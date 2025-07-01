from flask import jsonify, Blueprint
from sqlalchemy import Table, Column, DateTime, Integer, Float, insert, select, inspect, and_
from datetime import datetime
import enum
from app.db import DB
from collections import defaultdict
from app.battery_status import esp_clients, browser_clients

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
    temperature = DB.Column(DB.Float, nullable=True)
    voltage = DB.Column(DB.Float, nullable=True)
    last_updated_time = DB.Column(DB.DateTime, default=datetime.now, onupdate=datetime.now, nullable=False)
    lat = DB.Column(DB.Float, nullable=True)
    lon = DB.Column(DB.Float, nullable=True)
    parent_id = DB.Column(DB.String(64), nullable=True)

    def __repr__(self):
        return f'<BatteryInfo {self.id} - {self.name}>'

# Reset parent_id for all batteries
def reset_structure():
    all_batteries = DB.session.query(BatteryInfo).all()
    for b in all_batteries:
        b.parent_id = None
    DB.session.commit()

# Update the structure of batteries based on esp_client_ids
def update_structure(esp_client_ids):
    if not esp_clients or len(esp_client_ids) == 0:
        return False
    # Restructure parent-child relationships based on esp_client
    parent_id = 0
    for i, esp_id in esp_client_ids:
        parent_id = esp_id if i == 0 else 0
        battery = DB.session.get(BatteryInfo, esp_id)
        if battery:
            battery.parent_id = 0 if i == 0 else parent_id
            battery.last_updated_time = datetime.now()
        DB.session.commit()

# Update the database with new battery data
def update_database(data_list):
    # First object in array is the master node
    if not data_list or len(data_list) == 0:
        return False
    for _, content_data in enumerate(data_list):
        esp_id = content_data['id']
        data = content_data['content']
        try:
            # Try to get existing record
            battery = DB.session.get(BatteryInfo, esp_id)
            if battery:
                # Update existing record
                battery.name=esp_id
                battery.temperature=data['aT']/10
                battery.voltage=data['V']/10
                battery.last_updated_time = datetime.now()
                print(f"Updated existing battery with ID: {esp_id}")
                print(battery)
            else:
                # Insert new record
                battery = BatteryInfo(
                    id=esp_id,
                    name=esp_id,
                    temperature=data['aT']/10,
                    voltage=data['V']/10,
                    last_updated_time = datetime.now()
                )
                DB.session.add(battery)
                print(f"Inserted new battery with ID: {esp_id}")
            # Commit the transaction
            DB.session.commit()
        except Exception as e:
            DB.session.rollback()
            print(f"Error processing battery ID {esp_id}: {e}")
        # Dynamic create battery data table
        data_table = create_battery_data_table(esp_id)
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
            print(f"Inserted new battery_data with ID: {esp_id}")
        except Exception as e:
            DB.session.rollback()
            print(f"Error processing battery_data ID {esp_id}: {e}")

# Dynamic battery_data_<esp_id> table factory
def create_battery_data_table(esp_id, metadata=None):
    if metadata is None:
        metadata = DB.metadata

    table_name = f"battery_data_{esp_id.lower()}"
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

# Structure the battery tree for UI display
def build_battery_tree(batteries):
    node_map = {}
    children_map = defaultdict(list)
    # Get all online batteries
    online_esp_ids = [esp_id for client in esp_clients.values() for esp_id in client['ids']]
    # Create dicts of battery info and collect children
    for b in batteries:
        node_map[b.id] = {
            'id': b.id,
            'name': b.name,
            'online_status': OnlineStatusEnum.online if b.id in online_esp_ids else OnlineStatusEnum.offline,
            'last_updated_time': b.last_updated_time.isoformat(),
            'lat': b.lat,
            'lon': b.lon,
            'temperature': b.temperature,
            'voltage': b.voltage,
            'parent_id': b.parent_id,
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