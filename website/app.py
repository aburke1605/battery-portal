#!venv/bin/python

import mysql.connector
from flask import Flask, render_template

import time

import matplotlib.pyplot as plt
import io
import base64


app = Flask(__name__)

local = False
if local:
    db = mysql.connector.connect(
        host="localhost",
        user="root",
        password="password",
        database="battery_data",
    )
else:
    db = mysql.connector.connect(
        host="batteryportal-server.mysql.database.azure.com",
        user="hroolfgemh",
        password="Qaz123ws",
        database="batteryportal-database",
        port=3306,
        ssl_ca="DigiCertGlobalRootCA.crt.pem",
        ssl_disabled=False
    )


cursor = db.cursor()

table = """
    <----->
     EMPTY
    <----->
"""

esp_id = "BMS_01"

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
        db.commit()
        print("table created")

    else:
        cursor.execute(f"       SELECT COUNT(*) FROM {esp_id}")
        n_rows = cursor.fetchone()[0]
        print("table already exists")


    while n_rows < 10:
        cursor.execute(f"""
                                INSERT INTO {esp_id} (timestamp, soc, temperature, voltage, current)
                                VALUES (FROM_UNIXTIME({time.time()}), {31}, {5.5}, {3.7}, {0.1})
        """)
        db.commit()

        n_rows += 1
        time.sleep(1)


    cursor.execute(f"           SELECT * FROM {esp_id}")
    table = cursor.fetchall()

except mysql.connector.Error as err:
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

@app.route('/')
def index():
    return render_template("test.html", table=table, plot_url=base64.b64encode(img.getvalue()).decode())
