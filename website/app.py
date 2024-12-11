#!venv/bin/python
import os
import string
from datetime import time, datetime
import random
import matplotlib.pyplot as plt

from flask import Flask, url_for, redirect, render_template, request, abort
from flask_sqlalchemy import SQLAlchemy
from flask_security import Security, SQLAlchemyUserDatastore, \
    UserMixin, RoleMixin, login_required, current_user
from flask_security.utils import hash_password
import flask_admin
from flask_admin.contrib import sqla
from flask_admin import helpers as admin_helpers
from flask_socketio import SocketIO, emit
import time
from wtforms import PasswordField

# Create Flask application
app = Flask(__name__)
app.config['SECRET_KEY'] = 'd0ughball$'
app.config.from_pyfile('config.py')
db = SQLAlchemy(app)
socketio = SocketIO(app)

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

# this would be done in some other script
# whether that be on a regular basis or something?
fig, ax = plt.subplots()
xs = range(100)
ys = [random.randint(1, 50) for x in xs]
ax.plot(xs, ys)
plt.savefig('static/plots/plot.png')

data_store = {}

@app.route('/aodhan')
def index_aodhan():
    print('Request for index page received')
    Q = data_store["ESP32"]["charge"]
    V = data_store["ESP32"]["voltage"]
    I = data_store["ESP32"]["current"]
    T = data_store["ESP32"]["temperature"]

    return render_template('index.html')


@app.route('/data', methods=['POST'])
def receive_data():
    data = request.json
    if data:
        data_store["ESP32"] = data
        socketio.emit('update_data', data) # broadcast to websocket clients
        return {"status": "success", "data_received": data}, 200
    return {"status": "error", "message": "No data received"}, 400

@app.route('/admin/dashboard')
@login_required
def admin_index():
    return render_template('admin/dashboard.html')

@app.route('/admin/battery')
@login_required
def subpage():
    return render_template('admin/battery.html')

# Socket connect
@socketio.on('connect', namespace='/websocket')
def on_connect():
    print('Client connected')

# @socketio.on('connect')
# def handle_connect():
#     print('Client connected')

@socketio.on('disconnect', namespace='/websocket')
def on_disconnect():
    print('Client disconnected')

# Function to send data to the client every 5 seconds
def send_data():
    while True:
        print('send data')
        current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        socketio.emit('server_response', {'data': current_time},  namespace='/websocket')
        time.sleep(1)

# Start background thread to send data
socketio.start_background_task(send_data)

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
