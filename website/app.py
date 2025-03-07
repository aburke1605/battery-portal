import mysql.connector

db = mysql.connector.connect(
    host="batteryportal-server.mysql.database.azure.com",
    user="hroolfgemh",
    password="Qaz123ws",
    database="batteryportal-database"
)

cursor = db.cursor()
cursor.execute("SHOW TABLES")
for table in cursor.fetchall():
    print(table)
