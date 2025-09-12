from app import create_app
app = create_app()

if __name__ == "__main__":
    app.run(debug=True, ssl_context=("../website/cert/local_cert.pem", "../website/cert/local_key.pem"), host="0.0.0.0")
