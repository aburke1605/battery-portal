from flask import render_template, request

from app import create_app

app = create_app()

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/info")
def info():
    esp_id = request.args.get("esp_id")
    return render_template("info.html", esp_id=esp_id)

if __name__ == "__main__":
    app.run(debug=True, ssl_context=("../website/cert/local_cert.pem", "../website/cert/local_key.pem"), host="0.0.0.0")