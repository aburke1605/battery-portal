from flask import Blueprint, send_from_directory

main = Blueprint("main", __name__, url_prefix="/")

@main.route("/", methods=["GET"])
def home():
    return send_from_directory("../frontend/dist/", "home.html")

@main.route("/admin", methods=["GET"])
def admin():
    return send_from_directory("../frontend/dist", "index.html")

@main.route("/<path:path>", methods=["GET"])
def serve_react_static(path: str):
    return send_from_directory("../frontend/dist", path)
