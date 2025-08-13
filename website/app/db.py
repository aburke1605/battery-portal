import os

import logging
logger = logging.getLogger(__name__)

from flask import Blueprint, render_template, request, jsonify
from flask_login import login_required
from flask_sqlalchemy import SQLAlchemy
from flask_security import roles_required

import mysql.connector
import datetime

db_bp = Blueprint('db_bp', __name__, url_prefix='/db')

# Create DB instance
DB = SQLAlchemy()

DB_CONFIG = {
    "host": os.getenv("AZURE_MYSQL_HOST", "localhost"),
    "user": os.getenv("AZURE_MYSQL_USER", "root"),
    "password": os.getenv("AZURE_MYSQL_PASSWORD", "password"),
    "database": os.getenv("AZURE_MYSQL_NAME", "battery_data"),
    "port": int(os.getenv("AZURE_MYSQL_PORT", 3306)),
    "ssl_ca": os.getenv("AZURE_MYSQL_SSL_CA", None),
    "ssl_disabled": os.getenv("AZURE_MYSQL_SSL_DISABLED", "False") == "True",
}

def update_db(esp_id, data):
    try:
        DB = mysql.connector.connect(**DB_CONFIG)
        cursor = DB.cursor()

        n_rows = 0

        cursor.execute(f"""
                                    SHOW TABLES LIKE '{esp_id}'
        """)
        if not cursor.fetchone():
            cursor.execute(f"""
                                    CREATE TABLE {esp_id} (
                                        timestamp TIMESTAMP PRIMARY KEY,
                                        soc INT,
                                        temperature FLOAT,
                                        voltage FLOAT,
                                        current FLOAT
                                    )
            """)
            DB.commit()
            logger.info("table created")

        else:
            cursor.execute(f"       SELECT COUNT(*) FROM {esp_id}")
            n_rows = cursor.fetchone()[0]

        if abs(data["I"]) >= 0.1: # and n_rows < 10000:
            cursor.execute(f"""
                                    INSERT INTO {esp_id} (timestamp, soc, temperature, voltage, current)
                                    VALUES ('{datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}', {data['Q']}, {data['aT']/10}, {data['V']/10}, {data['I']/10})
            """)
            DB.commit()


    except mysql.connector.Error as err:
        logger.error(err)


def execute_query(query):
    try:
        DB = mysql.connector.connect(**DB_CONFIG)
        cursor = DB.cursor(dictionary=True)

        cursor.execute(query)
        result = cursor.fetchall()

        cursor.close()
        DB.close()

        return result

    except Exception as e:
        return {"error": str(e)}

@db_bp.route("/query")
@login_required
def query():
    return render_template("db/query.html")

@db_bp.route("/execute_sql", methods=["POST"])
@roles_required('superuser')
def execute_sql():

    data = request.get_json()
    query = data.get("query")

    if not query:
        return jsonify({"error": "No query provided"}), 400

    verified = True if query[:6] == "ADMIN " else False
    dangerous_statements = ["drop", "delete", "update", "alter", "truncate"]
    if any(word in query.lower() for word in dangerous_statements):
        if not verified:
            return jsonify({"confirm": "Please re-enter the query to confirm execution."}), 403
        else:
            query = query[5:]

    result = execute_query(query)
    return jsonify(result)

@db_bp.route('/data')
def data():
    esp_id = request.args.get("esp_id")
    table = f"battery_data_{esp_id}"
    column = request.args.get("column")

    data = []
    try:
        DB = mysql.connector.connect(**DB_CONFIG)
        cursor = DB.cursor()

        cursor.execute(f"""
                                    SELECT * FROM (
                                        SELECT timestamp, {column}
                                        FROM {table}
                                        ORDER BY timestamp DESC
                                        LIMIT 250
                                    ) sub
                                    ORDER BY timestamp ASC;
        """)
        rows = cursor.fetchall()

        previous = None
        for row in rows[::-1]: # work from end

            # take only data from most recent date
            if row[0].date() != rows[-1][0].date():
                continue

            if previous is not None:
                if previous - row[0] > datetime.timedelta(minutes = 5):
                    break
            previous = row[0]

            data.append({"timestamp": row[0], column: row[1]})

        cursor.close()
        DB.close()

        # reverse it back
        data = data[::-1]

    except mysql.connector.Error as err:
        logger.error(err)

    return jsonify(data)
