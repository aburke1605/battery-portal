from flask import current_app, Blueprint, request, jsonify
from sqlalchemy import create_engine, text
from flask_login import login_required
import datetime
import logging
from flask_sqlalchemy import SQLAlchemy

logger = logging.getLogger(__name__)

db_bp = Blueprint('db_bp', __name__, url_prefix='/api/db')

_engine = None

# TODO REMOVE THIS
DB = SQLAlchemy()

def get_engine():
    global _engine
    if _engine is None:
        _engine = create_engine(current_app.config['SQLALCHEMY_DATABASE_URI'])
    return _engine

@db_bp.route('/query', methods=['POST'])
@login_required
def run_query():
    sql_query = request.get_json().get('query', '').strip()
    if not sql_query.lower().startswith('select'):
        return jsonify({'error': 'Only SELECT statements are allowed.'}), 400
    try:
        with get_engine().connect() as conn:
            result = conn.execute(text(sql_query))
            rows = [dict(row) for row in result.mappings()]  # <- real dicts now
            return jsonify({'result': rows})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

# TODO REMOVE THIS, and using ORM instead 
def update_db(esp_id: str, data: dict):
    try:
        with get_engine().connect() as conn:
            # Check if table exists
            result = conn.execute(text("""
                SHOW TABLES LIKE :table_name
            """), {'table_name': esp_id})

            if not result.fetchone():
                # Create the table if it doesn't exist
                conn.execute(text(f"""
                    CREATE TABLE `{esp_id}` (
                        timestamp TIMESTAMP PRIMARY KEY,
                        soc INT,
                        temperature FLOAT,
                        voltage FLOAT,
                        current FLOAT
                    )
                """))
                logger.info(f"Table `{esp_id}` created.")
            else:
                # Count rows
                count_result = conn.execute(text(f"""
                    SELECT COUNT(*) FROM `{esp_id}`
                """))
                n_rows = count_result.scalar()
            # Optional logic to prevent unnecessary inserts
            if abs(data["current"]) >= 0.1:
                now = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                conn.execute(text(f"""
                    INSERT INTO `{esp_id}` (timestamp, soc, temperature, voltage, current)
                    VALUES (:ts, :soc, :temp, :volt, :curr)
                """), {
                    'ts': now,
                    'soc': data['charge'],
                    'temp': data['temperature'],
                    'volt': data['voltage'],
                    'curr': data['current']
                })

    except Exception as e:
        logger.error(f"Database update failed: {e}")