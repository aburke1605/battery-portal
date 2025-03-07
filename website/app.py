#!venv/bin/python

import mysql.connector
from flask import Flask, render_template

app = Flask(__name__)

db = mysql.connector.connect(
    host="batteryportal-server.mysql.database.azure.com",
    user="hroolfgemh",
    password="Qaz123ws",
    database="batteryportal-database"
)

cursor = db.cursor()
cursor.execute("SHOW TABLES")

n = 0
for table in cursor.fetchall():
    data = table
    n += 1

@app.route('/')
def index():
    return render_template("test.html", n=n, data=data)