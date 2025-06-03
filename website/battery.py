from flask import request, jsonify, Blueprint
from sqlalchemy import Table, Column, DateTime, Integer, Float, insert, select, inspect, and_
from sqlalchemy.exc import IntegrityError
from datetime import datetime, timedelta
import time
import enum
from threading import Thread
from db import DB
from collections import defaultdict

# Blueprint setup
battery_bp = Blueprint('battery_bp', __name__, url_prefix='/api/battery')

# Enum for battery status
class OnlineStatusEnum(enum.Enum):
    online = "online"
    offline = "offline"

# BatteryInfo table
class BatteryInfo(DB.Model):
    __tablename__ = 'battery_info'

    id = DB.Column(DB.String(64), primary_key=True)
    name = DB.Column(DB.String(100), nullable=False)
    online_status = DB.Column(DB.Enum(OnlineStatusEnum), nullable=False, default=OnlineStatusEnum.offline)
    last_updated_time = DB.Column(DB.DateTime, default=datetime.now, onupdate=datetime.now, nullable=False)
    lat = DB.Column(DB.Float, nullable=True)
    lon = DB.Column(DB.Float, nullable=True)
    parent_id = DB.Column(DB.String(64), DB.ForeignKey('battery_info.id'), nullable=True)
    children = DB.relationship('BatteryInfo', backref=DB.backref('parent', remote_side=[id]), lazy=True)

    def __repr__(self):
        return f'<BatteryInfo {self.id} - {self.name}>'

def update_from_esp(data_list):
    # First object in array is the master node
    if not data_list or data_list is None:
        return False
    parent_id = 0
    for i, content_data in enumerate(data_list):
        battery_id = content_data['id']
        data = content_data['content']
        # Set parent_id for non-master nodes
        if i == 0:
            parent_id = battery_id
        try:
            # Try to get existing record
            battery = DB.session.get(BatteryInfo, battery_id)
            if battery:
                # Update existing record
                battery.name = battery_id
                battery.online_status = OnlineStatusEnum.online
                battery.parent_id = None if i == 0 else parent_id
                battery.last_updated_time = datetime.now()
                print(f"Updated existing battery with ID: {battery_id}")
            else:
                # Insert new record
                battery = BatteryInfo(
                    id=battery_id,
                    name=battery_id,
                    online_status=OnlineStatusEnum.online,
                    parent_id=None if i == 0 else parent_id,
                    last_updated_time = datetime.now()
                )
                DB.session.add(battery)
                print(f"Inserted new battery with ID: {battery_id}")
            # Commit the transaction
            DB.session.commit()
        except Exception as e:
            DB.session.rollback()
            print(f"Error processing battery ID {battery_id}: {e}")
        # Dynamic create battery data table
        data_table = create_battery_data_table(battery_id)
        try:
            stmt = insert(data_table).values(
                timestamp=datetime.now(),
                soc=data['Q'],
                temperature=data['aT']/10,
                voltage=data['V']/10,
                current=data['I']/10
            )
            DB.session.execute(stmt)
            DB.session.commit()
            print(f"Inserted new battery_data with ID: {battery_id}")
        except Exception as e:
            DB.session.rollback()
            print(f"Error processing battery_data ID {battery_id}: {e}")

# Dynamic battery_data_<battery_id> table factory
def create_battery_data_table(battery_id, metadata=None):
    if metadata is None:
        metadata = DB.metadata

    table_name = f"battery_data_{battery_id.lower()}"
    # Check if table exists
    inspector = inspect(DB.engine)
    if not inspector.has_table(table_name):
        data_table = Table(
            table_name,
            metadata,
            Column('id', Integer, primary_key=True, autoincrement=True),  # Auto-incremented PK
            Column('timestamp', DateTime, nullable=False, default=datetime.now),
            Column('soc', Integer, nullable=False),
            Column('temperature', Float, nullable=False),
            Column('voltage', Float, nullable=False),
            Column('current', Float, nullable=False),
        )
        data_table.create(bind=DB.engine)
        print(f"Created table: {table_name}")
    else:
        print(f"Existed table: {table_name}")
        data_table = Table(table_name, metadata, autoload_with=DB.engine)

    return data_table

def build_battery_tree(batteries):
    node_map = {}
    children_map = defaultdict(list)

    # First, create dicts of battery info and collect children
    for b in batteries:
        node_map[b.id] = {
            'id': b.id,
            'name': b.name,
            'online_status': b.online_status.value,
            'last_updated_time': b.last_updated_time.isoformat(),
            'lat': b.lat,
            'lon': b.lon,
            'capacity': '80',
            'voltage': '20',
            'parent_id': b.parent_id,
            'status': 'active' if b.online_status == OnlineStatusEnum.online else 'inactive',
            'children': []
        }
        if b.parent_id:
            children_map[b.parent_id].append(b.id)

    # Link children to parents
    for parent_id, child_ids in children_map.items():
        if parent_id in node_map:
            for child_id in child_ids:
                node_map[parent_id]['children'].append(node_map[child_id])

    # Extract roots (top-level batteries with no parent)
    tree = [node for node in node_map.values() if node['parent_id'] is None]
    return tree

@battery_bp.route('/list', methods=['GET'])
def get_batteries():
    batteries = DB.session.query(BatteryInfo).all()
    battery_tree = build_battery_tree(batteries)
    return jsonify(battery_tree)

# Check online status
def check_online_task(app):
    def run():
        with app.app_context():
            print("Started check_online task.")
            while True:
                try:
                    cutoff_time = datetime.now() - timedelta(minutes=1)
                    print(cutoff_time)
                    stale_batteries = DB.session.query(BatteryInfo).filter(
                        and_(
                            BatteryInfo.online_status == OnlineStatusEnum.online,
                            BatteryInfo.last_updated_time < cutoff_time
                        )
                    ).all()
                    print(stale_batteries)
                    for battery in stale_batteries:
                        print(f"[Monitor] Setting '{battery.id}' offline")
                        battery.online_status = OnlineStatusEnum.offline

                    if stale_batteries:
                        DB.session.commit()

                except Exception as e:
                    DB.session.rollback()
                    print(f"[Monitor] Error: {e}")

                time.sleep(60)

    return Thread(target=run, daemon=True)