import os
from flask import Blueprint, render_template, request, jsonify
from flask_login import login_required
import datetime

import mysql.connector
import time

import matplotlib.pyplot as plt
import io
import base64

db = Blueprint('db', __name__, url_prefix='/db')


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
            print("table created")

        else:
            cursor.execute(f"       SELECT COUNT(*) FROM {esp_id}")
            n_rows = cursor.fetchone()[0]

        if abs(data["current"]) >= 0.1: # and n_rows < 10000:
            cursor.execute(f"""
                                    INSERT INTO {esp_id} (timestamp, soc, temperature, voltage, current)
                                    VALUES (FROM_UNIXTIME({time.time()}), {data['charge']}, {data['temperature']}, {data['voltage']}, {data['current']})
            """)
            DB.commit()


    except mysql.connector.Error as err:
        print(f"Error: {err}")


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

@db.route("/query")
@login_required
def query():
    return render_template("db/query.html")

@db.route("/execute_sql", methods=["POST"])
@login_required
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

@db.route('/display')
def display():
    esp_id = request.args.get("esp_id")

    fig = plt.figure(figsize=(15, 11))

    try:
        DB = mysql.connector.connect(**DB_CONFIG)
        cursor = DB.cursor()

        cursor.execute(f"           SELECT * FROM {esp_id}")
        table = cursor.fetchall()

        cursor.execute(f"           SELECT timestamp FROM {esp_id}"); timestamp = cursor.fetchall()
        cursor.execute(f"           SELECT soc FROM {esp_id}"); soc = cursor.fetchall()
        cursor.execute(f"           SELECT temperature FROM {esp_id}"); temperature = cursor.fetchall()
        cursor.execute(f"           SELECT voltage FROM {esp_id}"); voltage = cursor.fetchall()
        cursor.execute(f"           SELECT current FROM {esp_id}"); current = cursor.fetchall()

        gs = fig.add_gridspec(2, 2, height_ratios=[1, 1], hspace=0, wspace=0.1) # middle row is used as a spacer and so not used
        ax1 = fig.add_subplot(gs[0, 0])
        ax2 = fig.add_subplot(gs[1, 0], sharex=ax1)
        ax3 = fig.add_subplot(gs[0, 1])
        ax4 = fig.add_subplot(gs[1, 1], sharex=ax3)
        ax2.set_xlabel("timestamp")
        ax4.set_xlabel("timestamp")

        ax1.scatter(timestamp, soc, marker=".")
        ax1.set_ylabel("soc [%]")

        ax2.scatter(timestamp, temperature, marker=".")
        ax2.set_ylabel("T [°C]")

        ax3.scatter(timestamp, voltage, marker=".")
        ax3.set_ylabel("V [V]")
        ax3.yaxis.set_label_position("right")
        ax3.yaxis.tick_right()

        ax4.scatter(timestamp, current, marker=".")
        ax4.set_ylabel("I [A]")
        ax4.yaxis.set_label_position("right")
        ax4.yaxis.tick_right()

        cursor.close()
        DB.close()

    except mysql.connector.Error as err:
        table = ""
        ax = fig.add_subplot()
        ax.text(0.5,0.5,"No data")

        print(f"Error: {err}")

    plt.gcf().autofmt_xdate()
    img = io.BytesIO()
    fig.savefig(img, format="png", bbox_inches='tight')
    # img.seek(0) # rewind the buffer (needed?)

    return render_template("db/display.html", table=table, plot_url=base64.b64encode(img.getvalue()).decode())

@db.route('/data')
def data():
    esp_id = request.args.get("esp_id")

    try:
        DB = mysql.connector.connect(**DB_CONFIG)
        cursor = DB.cursor()

        cursor.execute(f"           SELECT timestamp, soc FROM {esp_id} ORDER BY timestamp")
        rows = cursor.fetchall()
        previous = None
        data = [{"timestamp": [], "value": []}]
        print(type(rows), dir(rows))
        for row in rows.reverse(): # work from end
            # take only data from most recent date
            if row[0].date() != rows[-1][0].date():
                continue

            if previous is not None:
                if previous - datetime.timedelta(minutes = 5) > row[0].time():
                    continue
            previous = row[0].time()

            data["timestamp"].append(row[0])
            data["value"].append(row[1])

        # data = [{"timestamp": row[0], "value": row[1]} for row in cursor.fetchall()]

        cursor.close()
        DB.close()

        return jsonify(data)

    except mysql.connector.Error as err:
        print(f"Error: {err}")

        return jsonify({})
