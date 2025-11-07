import os

from flask import Blueprint, send_from_directory

main = Blueprint("main", __name__, url_prefix="/")

build_dir = os.path.join(os.path.dirname(__file__), "../../frontend/dist")


@main.route("/", methods=["GET"])
def serve_react_frontend():
    return send_from_directory(build_dir, "index.html")


@main.route("/<path:file>", methods=["GET"])
def serve_react_static(file: str):
    return send_from_directory(build_dir, file)
