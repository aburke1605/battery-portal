from flask import request, jsonify, Blueprint
from sqlalchemy import Table, Column, DateTime, Integer, Float, String, Enum, insert, select
from sqlalchemy.exc import IntegrityError
from datetime import datetime
import enum

from db import DB

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
    lat = DB.Column(DB.Float, nullable=False)
    lon = DB.Column(DB.Float, nullable=False)
    parent_id = DB.Column(DB.String(64), DB.ForeignKey('battery_info.id'), nullable=True)
    children = DB.relationship('BatteryInfo', backref=DB.backref('parent', remote_side=[id]), lazy=True)

    def __repr__(self):
        return f'<BatteryInfo {self.id} - {self.name}>'

# Dynamic battery_data_<battery_id> table factory
def create_battery_data_table(battery_id, metadata=None):
    if metadata is None:
        metadata = DB.metadata

    table_name = f"battery_data_{battery_id.lower()}"
    return Table(
        table_name,
        metadata,
        Column('timestamp', DateTime, primary_key=True, default=datetime.utcnow, nullable=False),
        Column('soc', Integer, nullable=False),
        Column('temperature', Float, nullable=False),
        Column('voltage', Float, nullable=False),
        Column('current', Float, nullable=False),
    )

# API: Register a new battery
@battery_bp.route('/battery/register', methods=['POST'])
def register_battery():
    data = request.json
    try:
        battery = BatteryInfo(
            id=data['id'],
            name=data['name'],
            online_status=OnlineStatusEnum(data.get('online_status', 'offline')),
            lat=data['lat'],
            lon=data['lon'],
            parent_id=data.get('parent_id')
        )
        DB.session.add(battery)
        DB.session.commit()

        # Create dynamic battery_data table
        dynamic_table = create_battery_data_table(battery.id)
        dynamic_table.create(bind=DB.engine, checkfirst=True)

        return jsonify({'message': f'Battery {battery.id} registered successfully'}), 201
    except IntegrityError:
        DB.session.rollback()
        return jsonify({'error': 'Battery ID already exists'}), 409
    except Exception as e:
        DB.session.rollback()
        return jsonify({'error': str(e)}), 400

# API: Insert battery data into dynamic table
@battery_bp.route('/battery/<battery_id>/data', methods=['POST'])
def insert_battery_data(battery_id):
    data = request.json
    table = create_battery_data_table(battery_id)

    try:
        stmt = insert(table).values(
            timestamp=datetime.strptime(data['timestamp'], '%Y-%m-%d %H:%M:%S'),
            soc=data['soc'],
            temperature=data['temperature'],
            voltage=data['voltage'],
            current=data['current']
        )
        DB.session.execute(stmt)
        DB.session.commit()
        return jsonify({'message': 'Battery data inserted'}), 201
    except Exception as e:
        DB.session.rollback()
        return jsonify({'error': str(e)}), 400

# API: Query battery data
@battery_bp.route('/battery/<battery_id>/data', methods=['GET'])
def get_battery_data(battery_id):
    table = create_battery_data_table(battery_id)
    try:
        stmt = select(table).order_by(table.c.timestamp.desc()).limit(10)
        result = DB.session.execute(stmt).fetchall()
        records = [
            {
                'timestamp': row.timestamp.strftime('%Y-%m-%d %H:%M:%S'),
                'soc': row.soc,
                'temperature': row.temperature,
                'voltage': row.voltage,
                'current': row.current
            }
            for row in result
        ]
        return jsonify(records)
    except Exception as e:
        return jsonify({'error': str(e)}), 400
