from flask import Blueprint, render_template, request

import mysql.connector
import time

import matplotlib.pyplot as plt
import io
import base64


local = False
if local:
    DB = mysql.connector.connect(
        host="localhost",
        user="root",
        password="password",
        database="battery_data",
    )
else:
    DB = mysql.connector.connect(
        host="batteryportal-server.mysql.database.azure.com",
        user="hroolfgemh",
        password="Qaz123ws",
        database="batteryportal-database",
        port=3306,
        ssl_ca="DigiCertGlobalRootCA.crt.pem",
        ssl_disabled=False
    )


cursor = DB.cursor()


def update_db(esp_id, data):

    table = """
        <----->
        EMPTY
        <----->
    """

    try:
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
            print("table already exists")


        if n_rows < 100:
            cursor.execute(f"""
                                    INSERT INTO {esp_id} (timestamp, soc, temperature, voltage, current)
                                    VALUES (FROM_UNIXTIME({time.time()}), {data['charge']}, {data['temperature']}, {data['voltage']}, {data['current']})
            """)
            DB.commit()


    except mysql.connector.Error as err:
        print(f"Error: {err}")


db = Blueprint('db', __name__, url_prefix='/db')
@db.route('/test')
def test():
    esp_id = request.args.get("esp_id")

    try:
        cursor.execute(f"           SELECT * FROM {esp_id}")
        table = cursor.fetchall()

    except mysql.connector.Error as err:
        table = ""
        print(f"Error: {err}")

    fig, ax = plt.subplots()
    cursor.execute(f"               SELECT timestamp FROM {esp_id}"); t = cursor.fetchall()
    cursor.execute(f"               SELECT soc FROM {esp_id}"); y = cursor.fetchall()
    ax.plot(t, y)
    ax.set_xlabel("timestamp")
    ax.set_ylabel("soc [%]")
    plt.gcf().autofmt_xdate()
    img = io.BytesIO()
    fig.savefig(img, format="png", bbox_inches='tight')
    # img.seek(0) # rewind the buffer (needed?)

    return render_template("test.html", table=table, plot_url=base64.b64encode(img.getvalue()).decode())
