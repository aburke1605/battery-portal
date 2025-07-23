import os

# Create dummy secrey key so we can use sessions
SECRET_KEY = '123456790'

# Mysql database
MYSQL_HOST = os.getenv("AZURE_MYSQL_HOST", "localhost")
MYSQL_PORT = os.getenv("AZURE_MYSQL_PORT", 3306)
MYSQL_USER = os.getenv("AZURE_MYSQL_USER", "root")
MYSQL_PASSWORD = os.getenv("AZURE_MYSQL_PASSWORD", "password")
MYSQL_DATABASE = os.getenv("AZURE_MYSQL_NAME", "battery_data")

MYSQL_SSL_CART = os.getenv("AZURE_MYSQL_SSL_CA", None),
MYSQL_SSL_DISABLED =os.getenv("AZURE_MYSQL_SSL_DISABLED", "False") == "True"
SQLALCHEMY_DATABASE_URI = f"mysql+mysqlconnector://{MYSQL_USER}:{MYSQL_PASSWORD}@{MYSQL_HOST}:{MYSQL_PORT}/{MYSQL_DATABASE}"
SQLALCHEMY_ECHO = os.getenv("SQLALCHEMY_ECHO")

# Flask-Security config
SECURITY_URL_PREFIX = "/admin"
SECURITY_PASSWORD_HASH = "pbkdf2_sha512"
SECURITY_PASSWORD_SALT = "ATGUOHAELKiubahiughaerGOJAEGj"

# Flask-Security URLs, overridden because they don't put a / at the end
SECURITY_LOGIN_URL = "/login/"
SECURITY_LOGOUT_URL = "/logout/"
SECURITY_REGISTER_URL = "/register/"

SECURITY_POST_LOGIN_VIEW = "/admin"
SECURITY_POST_LOGOUT_VIEW = "/admin"
SECURITY_POST_REGISTER_VIEW = "/admin"

# Flask-Security features
SECURITY_REGISTERABLE = True
SECURITY_SEND_REGISTER_EMAIL = False
SQLALCHEMY_TRACK_MODIFICATIONS = False
