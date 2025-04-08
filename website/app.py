#!venv/bin/python
import os
from flask import Flask, render_template, send_from_directory
from flask_security import login_required, Security
from portal import portal
from ws import sock
from db import db_bp
from user import user_bp, user_datastore
from db import DB
from user import build_sample_db
import flask_admin

# Create Flask application
app = Flask(__name__)
app.config.from_pyfile('config.py')
app.config['SQLALCHEMY_ECHO'] = False
# Register blueprints
app.register_blueprint(portal)
app.register_blueprint(db_bp)
app.register_blueprint(user_bp)
# Register websocket
sock.init_app(app)
# Db setup
DB.init_app(app)
# Setup Flask-Security
security = Security(app, user_datastore)
# Create flask-admin
admin = flask_admin.Admin(app)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/admin/dashboard')
@login_required
def admin_index():
    return render_template('admin/dashboard.html')

@app.route('/admin/battery')
@login_required
def subpage():
    return render_template('admin/battery.html')

@app.route('/admin')
@login_required
def new_admin():
    return send_from_directory("frontend/dist", "index.html")

@app.route("/<path:path>")
def serve_react_static(path):
    return send_from_directory("frontend/dist", path)


# Init the table, only in dev env TODO
app_dir = os.path.realpath(os.path.dirname(__file__))
database_path = os.path.join(app_dir, app.config['DATABASE_FILE'])
if not os.path.exists(database_path):
    print("Database file not found. Creating tables and populating with sample data.")
    build_sample_db(app)

if __name__ == '__main__':
    app.run(debug=True, ssl_context=("local_cert.pem", "local_key.pem"), host="0.0.0.0")
