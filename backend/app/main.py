from flask import Blueprint, current_app

main = Blueprint("main", __name__, url_prefix="/")


@main.route("/", methods=["GET"])
def serve_react_frontend():
    return current_app.send_static_file("index.html")
