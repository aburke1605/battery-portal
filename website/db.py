import mysql.connector
import time


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
            db.commit()
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
            db.commit()


        cursor.execute(f"           SELECT * FROM {esp_id}")
        table = cursor.fetchall()

    except mysql.connector.Error as err:
        print(f"Error: {err}")
