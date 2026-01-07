import logging

logger = logging.getLogger(__name__)
import os
from uuid import uuid4
import re

from flask import Flask, Blueprint, request, jsonify
from flask_security import (
    UserMixin,
    RoleMixin,
    SQLAlchemyUserDatastore,
    login_user,
    logout_user,
    login_required,
    roles_required,
)
from flask_security.utils import hash_password
from flask_login import current_user
from sqlalchemy import inspect, select

user = Blueprint("user", __name__, url_prefix="/user")

from app.db import DB


class Roles(DB.Model, RoleMixin):
    """
    Defines the structure of the roles table, which defines types of users.
    """

    __tablename__ = "roles"

    id = DB.Column(DB.Integer, primary_key=True)
    name = DB.Column(DB.String(80), unique=True)
    description = DB.Column(DB.String(255))


# simple association table to link User(s) and Role(s)
roles_users = DB.Table(
    "roles_users",
    DB.Column("user_id", DB.Integer, DB.ForeignKey("users.id")),
    DB.Column("role_id", DB.Integer, DB.ForeignKey("roles.id")),
)


class Users(DB.Model, UserMixin):
    """
    Defines the structure of the users table.
    """

    __tablename__ = "users"

    id = DB.Column(DB.Integer, primary_key=True)
    first_name = DB.Column(DB.String(255), nullable=False)
    last_name = DB.Column(DB.String(255))
    email = DB.Column(DB.String(255), unique=True, nullable=False)
    password = DB.Column(DB.String(255), nullable=False)
    active = DB.Column(DB.Boolean())
    confirmed_at = DB.Column(DB.DateTime())
    fs_uniquifier = DB.Column(
        DB.String(64), unique=True, nullable=False, default=lambda: str(uuid4())
    )
    roles = DB.relationship(
        "Roles", secondary=roles_users, backref=DB.backref("users", lazy="dynamic")
    )
    subscribed = DB.Column(DB.Boolean(), default=False)
    subscription_expiry = DB.Column(DB.DateTime())


users = SQLAlchemyUserDatastore(DB, Users, Roles)


def create_admin(app: Flask):
    """
    Builds initial role data in the database and creates the admin user for basic functionality.
    """
    with app.app_context():
        inspector = inspect(DB.engine)
        if not inspector.has_table(Roles.__tablename__):
            return  # roles table not yet created

        admin_name = os.getenv("ADMIN_NAME", "admin")
        admin_email = os.getenv("ADMIN_EMAIL", "user@admin.dev")
        admin_password = os.getenv("ADMIN_PASSWORD", "password")

        # first ensure basic roles exist
        role_names = ["user", "superuser"]
        roles = {}
        for role_name in role_names:
            role = Roles.query.filter_by(name=role_name).first()
            if not role:
                role = Roles(name=role_name)
                DB.session.add(role)
                logger.info(f'creating new role "{role_name}"')
            roles[role_name] = role
        DB.session.commit()

        # then ensure the admin user exists
        if Users.query.filter_by(email=admin_email).first() is not None:
            return
        users.create_user(
            first_name=admin_name,
            email=admin_email,
            password=hash_password(admin_password),
            roles=[roles["user"], roles["superuser"]],
            # are the other columns of Users therefore not needed?
        )
        logger.info(f'creating admin user "{admin_email}"')
        DB.session.commit()


@user.route("/login", methods=["POST"])
def login():
    """
    API to verify user login requests, works in conjunction with AuthContext.tsx.
    """
    data = request.get_json()
    email = data.get("email")
    password = data.get("password")

    u = users.find_user(email=email)
    if u and u.verify_and_update_password(password):
        login_user(u)
        return jsonify(
            {
                "loggedIn": True,
                "email": u.email,
                "roles": [role.name for role in u.roles],
            }
        )
    return jsonify({"success": False}), 401


@user.route("/check-auth", methods=["GET"])
@login_required
def check_auth():
    """
    API to check for already logged-in users, works in conjunction with AuthContext.tsx.
    """
    return jsonify(
        {
            "loggedIn": True,
            "email": current_user.email,
            "first_name": current_user.first_name,
            "last_name": current_user.last_name,
            "roles": [role.name for role in current_user.roles],
        }
    )


@user.route("/logout", methods=["POST"])
@login_required
def logout():
    """
    API to perform user logout requests, works in conjunction with AuthContext.tsx.
    """
    logout_user()
    return jsonify({"success": "Logged out"}), 200


@user.route("/list", methods=["GET"])
@login_required
def list():
    """
    API to list users already existing in the database, works in conjunction with list.tsx.
    """
    page = request.args.get("page", default=1, type=int)
    per_page = request.args.get("per_page", default=10, type=int)

    pagination = Users.query.paginate(page=page, per_page=per_page, error_out=False)
    users = pagination.items

    user_list = [
        {
            "id": u.id,
            "first_name": u.first_name,
            "last_name": u.last_name,
            "email": u.email,
            "roles": [role.name for role in u.roles],
        }
        for u in users
    ]

    return jsonify(
        {
            "users": user_list,
            "total": pagination.total,
            "pages": pagination.pages,
            "current_page": pagination.page,
            "per_page": pagination.per_page,
            "has_next": pagination.has_next,
            "has_prev": pagination.has_prev,
        }
    )


@user.route("/add", methods=["POST"])
def add():
    """
    API to add new user to the database, works in conjunction with list.tsx
    """
    data = request.get_json()
    first_name = data.get("first_name")
    last_name = data.get("last_name")
    email = data.get("email")
    password = data.get("password")
    # role_ids = data.get("roles", [])

    existing_user = Users.query.filter_by(email=email).first()
    if existing_user:
        return jsonify({"error": "A user with this email address already exists."}), 400

    role_ids = [1]  # can only add normal role users for now
    roles = Roles.query.filter(Roles.id.in_(role_ids)).all()

    try:
        users.create_user(
            first_name=first_name,
            last_name=last_name,
            email=email,
            password=hash_password(password),
            roles=roles,
        )
        DB.session.commit()
        return jsonify({"success": "User added successfully"}), 201
    except Exception as e:
        DB.session.rollback()
        return jsonify({"error": f"Failed to create user: {e}"}), 500


@user.route("/<int:user_id>", methods=["PUT"])
@roles_required("superuser")
def edit(user_id: int):
    """
    API to edit existing user in the database, works in conjunction with list.tsx
    """
    u = Users.query.get_or_404(user_id)

    data = request.get_json()
    new_email = data.get("email", u.email)

    if new_email != u.email:  # email address is being edited
        existing_user = Users.query.filter_by(email=new_email).first()
        if existing_user:
            return (
                jsonify({"error": "A user with this email address already exists."}),
                400,
            )

    try:
        u.first_name = data.get("first_name", u.first_name)
        u.last_name = data.get("last_name", u.last_name)
        u.email = new_email
        new_password = data.get("password")
        if (
            new_password
        ):  # and new_password.strip(): # TODO add function to check all user entered stuff
            u.password = hash_password(new_password)
        DB.session.commit()
        return jsonify({"success": "User updated successfully"}), 200
    except Exception as e:
        DB.session.rollback()
        return jsonify({"error": f"Failed to update user: {e}."}), 500


@user.route("/<int:user_id>", methods=["DELETE"])
@roles_required("superuser")
def delete(user_id: int):
    """
    API to delete existing user from the database, works in conjunction with list.tsx
    """
    if current_user.id == user_id:
        return jsonify({"error": "Cannot delete admin account."}), 400

    u = Users.query.get_or_404(user_id)
    try:
        DB.session.delete(u)
        DB.session.commit()
        return jsonify({"success": "User deleted successfully."}), 200
    except Exception as e:
        DB.session.rollback()
        return jsonify({"error": f"Failed to delete user: {e}."}), 500


@user.route("/change-password", methods=["PUT"])
@login_required
def change_password():
    """
    API to change the current user's password.
    """
    if Roles.query.filter_by(name="superuser").first() in current_user.roles:
        return jsonify({"error": "Cannot change admin account."}), 400

    data = request.get_json()
    current_password = data.get("current_password")
    new_password = data.get("new_password")

    if not current_password or not new_password:
        return jsonify({"error": "All password fields are required."}), 400

    if not current_user.verify_and_update_password(current_password.strip()):
        return jsonify({"error": "Current password is incorrect."}), 400

    if len(new_password) < 8:
        return (
            jsonify({"error": "New password must be at least 8 characters long."}),
            400,
        )

    try:
        current_user.password = hash_password(new_password.strip())
        DB.session.commit()
        return jsonify({"success": "Password changed successfully."}), 200
    except Exception as e:
        DB.session.rollback()
        return jsonify({"error": f"Failed to change password: {e}."}), 500


# TODO: merge these (above and below)
@user.route("/profile", methods=["PUT"])
@login_required
def profile():
    """
    API to update the current user's profile information.
    """
    if Roles.query.filter_by(name="superuser").first() in current_user.roles:
        return jsonify({"error": "Cannot change admin account."}), 400

    data = request.get_json()

    new_first_name = data.get("first_name", current_user.first_name)
    new_last_name = data.get("last_name", current_user.last_name)
    new_email = data.get("email", current_user.email)

    # basic email format validation
    if not re.match(r"^[^\s@]+@[^\s@]+\.[^\s@]+$", new_email):
        return jsonify({"error": "Please enter a valid email address."}), 400

    if new_email != current_user.email:
        existing_user = Users.query.filter_by(email=new_email).first()
        if existing_user:
            return (
                jsonify({"error": "A user with this email address already exists."}),
                400,
            )

    try:
        current_user.first_name = new_first_name.strip()
        current_user.last_name = new_last_name.strip()
        current_user.email = new_email.strip()
        DB.session.commit()
        return jsonify({"success": "Profile updated successfully."}), 200
    except Exception as e:
        DB.session.rollback()
        return jsonify({"error": f"Failed to update profile: {e}."}), 500


@user.route("/subscription", methods=["GET"])
@login_required
def subscription():
    email = request.args.get("email")
    response = {"status": "success", "subscribed": False, "expiry": None}

    users_table = DB.Table("users", DB.metadata, autoload_with=DB.engine)
    # fmt: off
    query = (
        select(
            users_table.c.subscribed,
            users_table.c.subscription_expiry,
        )
        .where(users_table.c.email == email)
    )
    # fmt: on
    rows = DB.session.execute(query).first()
    response["subscribed"] = rows[0]
    response["expiry"] = rows[1].date().isoformat()

    return response, 200
