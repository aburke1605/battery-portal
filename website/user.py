# users.py
import os
import string
import random
from uuid import uuid4

from flask import Blueprint, request, jsonify
from flask_security import SQLAlchemyUserDatastore, UserMixin, RoleMixin, login_required
from flask_security.utils import hash_password
from wtforms import PasswordField

from db import DB

# Blueprint setup
user_bp = Blueprint('user_bp', __name__, url_prefix='/users')

# Models and Security Setup
roles_users = DB.Table(
    'roles_users',
    DB.Column('user_id', DB.Integer(), DB.ForeignKey('user.id')),
    DB.Column('role_id', DB.Integer(), DB.ForeignKey('role.id'))
)

class Role(DB.Model, RoleMixin):
    id = DB.Column(DB.Integer(), primary_key=True)
    name = DB.Column(DB.String(80), unique=True)
    description = DB.Column(DB.String(255))

    def __str__(self):
        return self.name

class User(DB.Model, UserMixin):
    id = DB.Column(DB.Integer, primary_key=True)
    first_name = DB.Column(DB.String(255), nullable=False)
    last_name = DB.Column(DB.String(255))
    email = DB.Column(DB.String(255), unique=True, nullable=False)
    password = DB.Column(DB.String(255), nullable=False)
    active = DB.Column(DB.Boolean())
    confirmed_at = DB.Column(DB.DateTime())
    fs_uniquifier = DB.Column(DB.String(64), unique=True, nullable=False, default=lambda: str(uuid4()))
    roles = DB.relationship('Role', secondary=roles_users,
                            backref=DB.backref('users', lazy='dynamic'))
    def __str__(self):
        return self.email

user_datastore = SQLAlchemyUserDatastore(DB, User, Role)

@user_bp.route('/list')
@login_required
def list_users():
    users = User.query.all()
    user_list = [
        {
            "id": user.id,
            "first_name": user.first_name,
            "last_name": user.last_name,
            "email": user.email,
            "roles": [role.name for role in user.roles]
        }
        for user in users
    ]
    return jsonify(user_list)

@user_bp.route('/add', methods=['POST'])
@login_required
def add_user():
    data = request.get_json()
    first_name = data.get('first_name')
    last_name = data.get('last_name')
    email = data.get('email')
    password = data.get('password')
    role_ids = data.get('roles', [])

    roles = Role.query.filter(Role.id.in_(role_ids)).all()

    user_datastore.create_user(
        first_name=first_name,
        last_name=last_name,
        email=email,
        password=hash_password(password),
        roles=roles
    )
    DB.session.commit()
    return jsonify({"message": "User added successfully"}), 201

@user_bp.route('/edit/<int:user_id>', methods=['POST'])
@login_required
def edit_user(user_id):
    user = User.query.get_or_404(user_id)
    data = request.get_json()

    user.first_name = data.get('first_name', user.first_name)
    user.last_name = data.get('last_name', user.last_name)
    user.email = data.get('email', user.email)
    role_ids = data.get('roles', [])
    user.roles = Role.query.filter(Role.id.in_(role_ids)).all()
    DB.session.commit()
    return jsonify({"message": "User updated successfully"})

# DB init helper
def build_sample_db(app):
    with app.app_context():
        DB.drop_all()
        DB.create_all()

        user_role = Role(name='user')
        super_user_role = Role(name='superuser')
        DB.session.add(user_role)
        DB.session.add(super_user_role)
        DB.session.commit()

        user = os.getenv("AZURE_MYSQL_USER", "user")
        password = os.getenv("AZURE_MYSQL_PASSWORD", "password")
        user_datastore.create_user(
            first_name=user,
            email=f'{user}@admin.dev',
            password=hash_password(password),
            roles=[user_role, super_user_role]
        )

        first_names = ['Harry', 'Amelia', 'Oliver', 'Jack', 'Isabella']
        last_names = ['Brown', 'Smith', 'Patel', 'Jones', 'Williams']

        for i in range(len(first_names)):
            tmp_email = f"{first_names[i].lower()}.{last_names[i].lower()}@example.com"
            tmp_pass = ''.join(random.choice(string.ascii_lowercase + string.digits) for _ in range(10))
            user_datastore.create_user(
                first_name=first_names[i],
                last_name=last_names[i],
                email=tmp_email,
                password=hash_password(tmp_pass),
                roles=[user_role]
            )

        DB.session.commit()