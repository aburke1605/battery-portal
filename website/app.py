#!venv/bin/python

import mysql.connector
from flask import Flask
import time

app = Flask(__name__)

db = mysql.connector.connect(
    host="batteryportal-server.mysql.database.azure.com",
    user="hroolfgemh",
    password="Qaz123ws",
    database="batteryportal-database"
)
cursor = db.cursor()

@app.route('/')
def index():
    while True:
        cursor.execute("SHOW TABLES")
        for table in cursor.fetchall():
            print(table)
        
        time.sleep(5)
