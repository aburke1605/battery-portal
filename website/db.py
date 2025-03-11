from flask import Blueprint, render_template, request, jsonify

import mysql.connector
import time

import matplotlib.pyplot as plt
import io
import base64


local = True
if local:
    DB_CONFIG = {
        "host": "localhost",
        "user": "root",
        "password": "password",
        "database": "battery_data",
    }
else:
    DB_CONFIG = {
        "host": "batteryportal-server.mysql.database.azure.com",
        "user": "hroolfgemh",
        "password": "Qaz123ws",
        "database": "batteryportal-database",
        "port": 3306,
        "ssl_ca": "DigiCertGlobalRootCA.crt.pem",
        "ssl_disabled": False
    }


def update_db(esp_id, data):

    table = """
        <----->
        EMPTY
        <----->
    """

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

        if n_rows < 100:
            cursor.execute(f"""
                                    INSERT INTO {esp_id} (timestamp, soc, temperature, voltage, current)
                                    VALUES (FROM_UNIXTIME({time.time()}), {data['charge']}, {data['temperature']}, {data['voltage']}, {data['current']})
            """)
            DB.commit()


    except mysql.connector.Error as err:
        print(f"Error: {err}")


db = Blueprint('db', __name__, url_prefix='/db')
@db.route('/display')
def display():
    esp_id = request.args.get("esp_id")

    fig, ax = plt.subplots()

    try:
        DB = mysql.connector.connect(**DB_CONFIG)
        cursor = DB.cursor()

        cursor.execute(f"           SELECT * FROM {esp_id}")
        table = cursor.fetchall()

        cursor.execute(f"           SELECT timestamp FROM {esp_id}"); t = cursor.fetchall()
        cursor.execute(f"           SELECT soc FROM {esp_id}"); y = cursor.fetchall()

        ax.plot(t, y)
        ax.set_xlabel("timestamp")
        ax.set_ylabel("soc [%]")

        cursor.close()
        DB.close()

    except mysql.connector.Error as err:
        table = ""
        ax.text(0.5,0.5,"No data")

        print(f"Error: {err}")

    plt.gcf().autofmt_xdate()
    img = io.BytesIO()
    fig.savefig(img, format="png", bbox_inches='tight')
    # img.seek(0) # rewind the buffer (needed?)

    return render_template("db/display.html", table=table, plot_url=base64.b64encode(img.getvalue()).decode())

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
def query():
    return render_template("db/query.html")  # Serve the HTML page

@db.route("/execute_sql", methods=["POST"])
def execute_sql():
    data = request.get_json()
    query = data.get("query")

    if not query:
        return jsonify({"error": "No query provided"}), 400

    # Security: Restrict dangerous queries
    forbidden_statements = ["drop", "delete", "update", "alter", "truncate"]
    if any(word in query.lower() for word in forbidden_statements):
        return jsonify({"error": "This query type is not allowed"}), 403

    result = execute_query(query)
    return jsonify(result)
