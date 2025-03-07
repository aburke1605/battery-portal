#!venv/bin/python

import mysql.connector
from flask import Flask, render_template

app = Flask(__name__)
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
cursor.execute("SHOW TABLES")
data = "none"
n = 0
for table in cursor.fetchall():
    data = table
    n += 1

if n == 0:
    cursor.execute("""
        CREATE TABLE users (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(255),
            email VARCHAR(255)
        )
    """)
    db.commit() # commit the transaction to save changes
    print("Table 'users' created.")

cursor.execute("SHOW TABLES")
for table in cursor.fetchall():
    data = table
    n += 1

@app.route('/')
def index():
    return render_template("test.html", n=n, data=data)