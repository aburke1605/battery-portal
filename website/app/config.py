import os

# create dummy secrey key so we can use sessions
SECRET_KEY = "123456790"

# environment variables for MySQL database
MYSQL_ADMIN_PASSWORD = os.getenv("MYSQL_ADMIN_PASSWORD", "password")
MYSQL_ADMIN_USERNAME = os.getenv("MYSQL_ADMIN_USERNAME", "root")
MYSQL_DATABASE_NAME = os.getenv("MYSQL_DATABASE_NAME", "battery_data")
MYSQL_PORT = os.getenv("MYSQL_PORT", 3306)
MYSQL_SERVER_HOST = os.getenv("MYSQL_SERVER_HOST", "localhost")
MYSQL_SSL_CA = (os.getenv("MYSQL_SSL_CA", None),)
MYSQL_SSL_DISABLED = os.getenv("MYSQL_SSL_DISABLED", "False") == "True"

SQLALCHEMY_DATABASE_URI = f"mysql+mysqlconnector://{MYSQL_ADMIN_USERNAME}:{MYSQL_ADMIN_PASSWORD}@{MYSQL_SERVER_HOST}:{MYSQL_PORT}/{MYSQL_DATABASE_NAME}"
SQLALCHEMY_ECHO = os.getenv("SQLALCHEMY_ECHO")

# flask_security config
SECURITY_PASSWORD_HASH = "pbkdf2_sha512"
SECURITY_PASSWORD_SALT = os.getenv(
    "SECURITY_PASSWORD_SALT", "ATGUOHAELKiubahiughaerGOJAEGj"
)
