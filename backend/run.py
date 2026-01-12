from app import create_app
from app.user import create_admin

app = create_app()


@app.cli.command("seed-admin")
def seed_admin():
    create_admin(app)


if __name__ == "__main__":
    app.run(
        debug=True,
        ssl_context=("cert/local_cert.pem", "cert/local_key.pem"),
        host="0.0.0.0",
        port=8000,
    )
