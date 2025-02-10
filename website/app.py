#!venv/bin/python
import os
import string
import random
import json
from threading import Lock

import requests
import urllib.parse

from flask import Flask, Response, render_template, request, jsonify, redirect, url_for, abort
from flask_sock import Sock

from flask_sqlalchemy import SQLAlchemy

from flask_security import Security, SQLAlchemyUserDatastore, UserMixin, RoleMixin, login_required, current_user
from flask_security.utils import hash_password

import flask_admin
from flask_admin.contrib import sqla
from flask_admin import helpers as admin_helpers

from wtforms import PasswordField

# Create Flask application
app = Flask(__name__)
app.config.from_pyfile('config.py')
sock = Sock(app)
db = SQLAlchemy(app)

# Define models
roles_users = db.Table(
    'roles_users',
    db.Column('user_id', db.Integer(), db.ForeignKey('user.id')),
    db.Column('role_id', db.Integer(), db.ForeignKey('role.id'))
)

class Role(db.Model, RoleMixin):
    id = db.Column(db.Integer(), primary_key=True)
    name = db.Column(db.String(80), unique=True)
    description = db.Column(db.String(255))

    def __str__(self):
        return self.name


class User(db.Model, UserMixin):
    id = db.Column(db.Integer, primary_key=True)
    first_name = db.Column(db.String(255), nullable=False)
    last_name = db.Column(db.String(255))
    email = db.Column(db.String(255), unique=True, nullable=False)
    password = db.Column(db.String(255), nullable=False)
    active = db.Column(db.Boolean())
    confirmed_at = db.Column(db.DateTime())
    fs_uniquifier = db.Column(db.String(64), unique=True, nullable=False, default=lambda: str(uuid4()))
    roles = db.relationship('Role', secondary=roles_users,
                            backref=db.backref('users', lazy='dynamic'))

    def __str__(self):
        return self.email

# Setup Flask-Security
user_datastore = SQLAlchemyUserDatastore(db, User, Role)
security = Security(app, user_datastore)

# Create customized model view class
class MyModelView(sqla.ModelView):
    def is_accessible(self):
        if not current_user.is_active or not current_user.is_authenticated:
            return False

        if current_user.has_role('superuser'):
            return True

        return False

    def _handle_view(self, name, **kwargs):
        """
        Override builtin _handle_view in order to redirect users when a view is not accessible.
        """
        if not self.is_accessible():
            if current_user.is_authenticated:
                # permission denied
                abort(403)
            else:
                # login
                return redirect(url_for('security.login', next=request.url))
    # can_edit = True
    edit_modal = True
    create_modal = True
    can_export = True
    can_view_details = True
    details_modal = True

class UserView(MyModelView):
    column_editable_list = ['email', 'first_name', 'last_name']
    column_searchable_list = column_editable_list
    column_exclude_list = ['password']
    #form_excluded_columns = column_exclude_list
    column_details_exclude_list = column_exclude_list
    column_filters = column_editable_list
    form_overrides = {
        'password': PasswordField
    }




@app.route('/')
def index():
    return render_template('Home/index.html')

@app.route('/admin/dashboard')
@login_required
def admin_index():
    return render_template('admin/dashboard.html')

@app.route('/admin/battery')
@login_required
def subpage():
    return render_template('admin/battery.html')


connected_esp_clients = []  # Keep track of connected WebSocket clients:  [{ "ws": ws, "ESP_ID": "esp32_1" }]
connected_browser_clients = []
lock = Lock()

def forward_request_to_esp32(endpoint, method="POST", ESP_ID=None):
    """
    Generic function to forward requests to the ESP32 via WebSocket.
    :param endpoint: ESP32 endpoint to forward the request to.
    :param method: HTTP method ("GET", "POST", etc.).
    :return: Response from the ESP32 WebSocket.
    """

    message = {
        "type": "request",
        "content": {
            "endpoint": endpoint,
            "method": method
        }
    }

    responses = []

    with lock:
        target_clients = [c for c in connected_esp_clients if not ESP_ID or c["ESP_ID"] == ESP_ID]

        if not target_clients:
            return 'No matching ESP32 connected', 404
        for client in target_clients:
            try:
                ws = client["ws"]
                if method == "POST":
                    message["content"]["data"] = request.form.to_dict()
                ws.send(json.dumps(message))

                response = ws.receive()
                if response:
                    responses.append({"ESP_ID": client["ESP_ID"], "response": response})

            except Exception as e:
                print(f"Error communicating with {client['ESP_ID']}: {e}")
                responses.append({"ESP_ID": client["ESP_ID"], "error": str(e)})

    return responses

@app.route('/')
def homepage():
    print('Request for home page received')
    return render_template('portal/homepage.html')

@app.route('/login')
def login():
    print('Request for login page received')
    return render_template('portal/login.html')

@app.route('/validate_login', methods=['POST'])
def validate_login():
    username = request.form.get("username")
    password = request.form.get("password")
    if username == "admin" and password == "1234":
        return redirect(url_for("display"), code=302)
    else:
        return "<p>Invalid username or password.</p>", 401


@app.route('/display')
def display():
    print('Request for display page received')
    return render_template('portal/display.html')

@app.route("/alert")
def alert():
    return render_template("portal/alert.html")

@sock.route('/ws')
def websocket(ws):
    global connected_esp_clients, connected_browser_clients
    metadata = {"ws": ws, "ESP_ID": "unknown"}

    with lock:
        connected_browser_clients.append(ws) # assume it's a browser client initially

    try:
        while True:
            # Receive a message from the client
            message = ws.receive()

            if message:
                response = {
                    "type": "response",
                    "content": {}
                }
                print(f"Received data: {message}")

                try:
                    data = json.loads(message)
                    metadata["ESP_ID"] = data.get("ESP_ID", "unknown")
                    response["content"]["status"] = "success"
                    response["content"]["ESP_ID"] = metadata["ESP_ID"]

                    if data["content"] == "register":
                        with lock:
                            connected_browser_clients.pop()
                            # Add the client to the connected clients set
                            connected_esp_clients.append(metadata)
                            print(f"Client connected. Total clients: {len(connected_esp_clients)}")
                    else:
                        with lock:
                            for client in connected_browser_clients:
                                try:
                                    client.send(json.dumps(data["content"]))
                                except Exception as e:
                                    print(f"Error sending to browser: {e}")
                                    connected_browser_clients.remove(client)  # Remove the client if sending fails

                except json.JSONDecodeError:
                    response["content"]["status"] = "error"
                    response["content"]["message"] = "invalid json"

                ws.send(json.dumps(response))

                response["type"] = "echo"
                response["content"] = message
                ws.send(json.dumps(response))
            else:
                break

    except Exception as e:
        print(f"WebSocket error: {e}")

    finally:
        with lock:
            connected_esp_clients = [c for c in connected_esp_clients if c["ws"] != ws]
            print(f"Client disconnected. Total clients: {len(connected_esp_clients)}")

@app.route('/change')
def change():
    print('Request for change page received')
    return render_template('portal/change.html')

@app.route('/validate_change', methods=['POST'])
def validate_change():
    return forward_request_to_esp32("validate_change")

@app.route('/reset', methods=['POST'])
def reset():
    return forward_request_to_esp32("reset", allow_redirects=False)


@app.route('/connect')
def connect():
    print('Request for connect page received')
    return render_template('portal/connect.html')

@app.route('/validate_connect', methods=['POST'])
def validate_connect():
    ESP_ID = request.args.get("ESP_ID")
    responses = forward_request_to_esp32("validate_connect", ESP_ID=ESP_ID)

    response = json.loads(responses[0]["response"]) # TODO: check all responses?
    if response["content"]["response"] == "already connected":
        message = "Already connected to Wi-Fi"
        encoded_message = urllib.parse.quote(message)
        return redirect(f"/alert?message={encoded_message}")

    return redirect("/display")


@app.route('/nearby')
def nearby():
    print('Request for nearby page received')
    return render_template('portal/nearby.html')

@app.route('/about')
def about():
    print('Request for about page received')
    return render_template('portal/about.html')

@app.route('/device')
def device():
    print('Request for device page received')
    return render_template('portal/device.html')

@app.route('/toggle', methods=['GET'])
def toggle():
    return forward_request_to_esp32("toggle", method="GET")


# Create admin
admin = flask_admin.Admin(
    app,
    url = "/admin/dashboard",
)

# Add model views
admin.add_view(MyModelView(Role, db.session, menu_icon_type='fa', menu_icon_value='fa-server', name="Roles"))
admin.add_view(UserView(User, db.session, menu_icon_type='fa', menu_icon_value='fa-users', name="Users"))

# define a context processor for merging flask-admin's template context into the
# flask-security views.
@security.context_processor
def security_context_processor():
    return dict(
        admin_base_template=admin.base_template,
        admin_view=admin.index_view,
        h=admin_helpers,
        get_url=url_for
    )

def build_sample_db():
    """
    Populate a small db with some example entries.
    """
    with app.app_context():
        db.drop_all()
        db.create_all()

        user_role = Role(name='user')
        super_user_role = Role(name='superuser')
        db.session.add(user_role)
        db.session.add(super_user_role)
        db.session.commit()

        user_datastore.create_user(
            first_name='Admin',
            email='admin@test.com',
            password=hash_password('admin'),
            roles=[user_role, super_user_role]
        )
        first_names = [
            'Harry', 'Amelia', 'Oliver', 'Jack', 'Isabella', 'Charlie', 'Sophie', 'Mia',
            'Jacob', 'Thomas', 'Emily', 'Lily', 'Ava', 'Isla', 'Alfie', 'Olivia', 'Jessica',
            'Riley', 'William', 'James', 'Geoffrey', 'Lisa', 'Benjamin', 'Stacey', 'Lucy'
        ]
        last_names = [
            'Brown', 'Smith', 'Patel', 'Jones', 'Williams', 'Johnson', 'Taylor', 'Thomas',
            'Roberts', 'Khan', 'Lewis', 'Jackson', 'Clarke', 'James', 'Phillips', 'Wilson',
            'Ali', 'Mason', 'Mitchell', 'Rose', 'Davis', 'Davies', 'Rodriguez', 'Cox', 'Alexander'
        ]

        for i in range(len(first_names)):
            tmp_email = first_names[i].lower() + "." + last_names[i].lower() + "@example.com"
            tmp_pass = ''.join(random.choice(string.ascii_lowercase + string.digits) for i in range(10))
            user_datastore.create_user(
                first_name=first_names[i],
                last_name=last_names[i],
                email=tmp_email,
                password=hash_password(tmp_pass),
                roles=[user_role, ]
            )

        db.session.commit()
    return

# Init the table, only in dev env TODO
app_dir = os.path.realpath(os.path.dirname(__file__))
database_path = os.path.join(app_dir, app.config['DATABASE_FILE'])
if not os.path.exists(database_path):
    print("Database file not found. Creating tables and populating with sample data.")
    build_sample_db()


if __name__ == '__main__':
    # Start app
    app.run(debug=True)
