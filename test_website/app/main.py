from flask import Blueprint, render_template, request

main = Blueprint("main", __name__, url_prefix="/")

@main.route("/")
def index():
    return render_template("index.html")

@main.route("/info")
def info():
    esp_id = request.args.get("esp_id")
    return render_template("info.html", esp_id=esp_id)