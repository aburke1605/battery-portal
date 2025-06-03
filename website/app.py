#!venv/bin/python
import sys
import logging
from flask import Flask, send_from_directory
from flask_security import Security
from ws import sock
from db import db_bp, DB
from user import user_bp, user_datastore
import flask_admin
from dotenv import load_dotenv
from flask_migrate import Migrate
from seed import seed_data
# Import user model for migrations
from user import User, Role
from battery import battery_bp, check_online_task


# Load environment variables from .env file will not overwrite system env vars
load_dotenv()

# Set up logging
logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    handlers=[
        logging.StreamHandler(sys.stdout)
    ]
)
annoying_logs = ["werkzeug", "passlib"]
for log in annoying_logs:
    logging.getLogger(log).setLevel(logging.WARNING)
logger = logging.getLogger(__name__)

# Create Flask application
app = Flask(__name__)
app.config.from_pyfile('config.py')
# Register blueprints
app.register_blueprint(db_bp)
app.register_blueprint(user_bp)
app.register_blueprint(battery_bp)
# Register websocket
sock.init_app(app)
# Db setup
DB.init_app(app)
# Setup Flask-Security
security = Security(app, user_datastore)
# Create flask-admin
admin = flask_admin.Admin(app)
# Setup flask-migrate
migrate = Migrate(app, DB)
# Basic route for the home page and React app, static files
@app.route('/')
def index():
    return send_from_directory('frontend/dist/', 'home.html')

@app.route('/admin')
def admin():
    return send_from_directory("frontend/dist", "index.html")

@app.route("/<path:path>")
def serve_react_static(path):
    return send_from_directory("frontend/dist", path)

# Start scheduled task
check_online_thread = check_online_task(app)
check_online_thread.start()
# Create the database and tables if they don't exist
# Create seed data
#seed_data(app)

if __name__ == '__main__':
    app.run(debug=True, ssl_context=("local_cert.pem", "local_key.pem"), host="0.0.0.0")
