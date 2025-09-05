from flask import Blueprint, send_from_directory, request, jsonify

main = Blueprint("main", __name__, url_prefix="/")

"""
@main.route("/", methods=["GET"])
def home():
    # return "<p>hello</p>"
    return send_from_directory("../frontend/dist/", "home.html")

@main.route("/admin", methods=["GET"])
def admin():
    return send_from_directory("../frontend/dist", "index.html")

@main.route("/<path:path>", methods=["GET"])
def serve_react_static(path: str):
    return send_from_directory("../frontend/dist", path)
"""

import mysql.connector
import os
DB_CONFIG = {
    "host": os.getenv("AZURE_MYSQL_HOST", "localhost"),
    "user": os.getenv("AZURE_MYSQL_USER", "root"),
    "password": os.getenv("AZURE_MYSQL_PASSWORD", "password"),
    "database": os.getenv("AZURE_MYSQL_NAME", "battery_data"),
    "port": int(os.getenv("AZURE_MYSQL_PORT", 3306)),
    "ssl_ca": os.getenv("AZURE_MYSQL_SSL_CA", None),
    "ssl_disabled": os.getenv("AZURE_MYSQL_SSL_DISABLED", "False") == "True",
}
@main.route("/api/db/test/execute_sql", methods=["POST"])
def test_execute_sql():

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

    result = test_execute_query(query)
    return jsonify(result)
def test_execute_query(query):
    try:
        DB = mysql.connector.connect(**DB_CONFIG)
        cursor = DB.cursor(dictionary=True)

        cursor.execute(query)
        result = cursor.fetchall()

        cursor.close()
        DB.close()

        print(type(result))
        print(result)
        return result

    except Exception as e:
        return {"error": str(e)}
@main.route("/")
def query():
    return send_from_directory("../templates/db/", "query.html")
