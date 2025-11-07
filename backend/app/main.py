import os

from flask import Blueprint, send_from_directory

main = Blueprint("main", __name__, url_prefix="/")

import logging

logger = logging.getLogger(__name__)
logger.info(
    f"os.path.dirname(__file__):                           {os.path.dirname(__file__)}"
)
logger.info(
    f"os.listdir(f'{{os.path.dirname(__file__)}}/../..'):    {os.listdir(f'{os.path.dirname(__file__)}/../..')}"
)
logger.info(
    f"os.listdir(f'{{os.path.dirname(__file__)}}/..'):       {os.listdir(f'{os.path.dirname(__file__)}/..')}"
)
logger.info(
    f"os.listdir(f'{{os.path.dirname(__file__)}}'):          {os.listdir(f'{os.path.dirname(__file__)}')}"
)

build_dir = os.path.join(os.path.dirname(__file__), "../../frontend/dist")


@main.route("/", methods=["GET"])
def serve_react_frontend():
    return send_from_directory(build_dir, "index.html")


@main.route("/<path:file>", methods=["GET"])
def serve_react_static(file: str):
    return send_from_directory(build_dir, file)
