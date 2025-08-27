from flask import Blueprint, request, jsonify
from flask_security import UserMixin, RoleMixin, SQLAlchemyUserDatastore, login_user, login_required, logout_user
from flask_security.utils import hash_password
from flask_login import current_user

from uuid import uuid4

user = Blueprint("user", __name__, url_prefix="/api/user")

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
roles_users = DB.Table("roles_users",
    DB.Column("user_id", DB.Integer, DB.ForeignKey("users.id")),
    DB.Column("role_id", DB.Integer, DB.ForeignKey("roles.id"))
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
    fs_uniquifier = DB.Column(DB.String(64), unique=True, nullable=False, default=lambda: str(uuid4()))
    roles = DB.relationship("Roles", secondary=roles_users,
                            backref=DB.backref("users", lazy="dynamic"))

users = SQLAlchemyUserDatastore(DB, Users, Roles)

@user.route("/login", methods=["POST"])
def api_login():
    data = request.get_json()
    email = data.get("email")
    password = data.get("password")

    u = users.find_user(email=email)
    if u and u.verify_and_update_password(password):
        login_user(u)
        return jsonify({
            "loggedIn": True,
            "email": u.email,
            "roles": [role.name for role in u.roles]  # if using roles
        })
    return jsonify({"success": False}), 401

@user.route("/check-auth", methods=["GET"])
@login_required
def auth_status():
    return jsonify({
        "loggedIn": True,
        "email": current_user.email,
        "roles": [role.name for role in current_user.roles]  # if using roles
    })

@user.route("/logout", methods=["POST"])
@login_required
def logout():
    logout_user()
    return jsonify({"message": "Logged out"}), 200

@user.route("/list")
@login_required
def list_users():
    # Get pagination parameters from query string
    page = request.args.get("page", default=1, type=int)
    per_page = request.args.get("per_page", default=10, type=int)

    # Paginate query
    pagination = Users.query.paginate(page=page, per_page=per_page, error_out=False)
    users = pagination.items

    user_list = [
        {
            "id": u.id,
            "first_name": u.first_name,
            "last_name": u.last_name,
            "email": u.email,
            "roles": [role.name for role in u.roles]
        }
        for u in users
    ]

    return jsonify({
        "users": user_list,
        "total": pagination.total,
        "pages": pagination.pages,
        "current_page": pagination.page,
        "per_page": pagination.per_page,
        "has_next": pagination.has_next,
        "has_prev": pagination.has_prev
    })

@user.route("/add", methods=["POST"])
@login_required
def add_user():
    data = request.get_json()
    first_name = data.get("first_name")
    last_name = data.get("last_name")
    email = data.get("email")
    password = data.get("password")
    role_ids = data.get("roles", [])

    roles = Roles.query.filter(Roles.id.in_(role_ids)).all()

    users.create_user(
        first_name=first_name,
        last_name=last_name,
        email=email,
        password=hash_password(password),
        roles=roles
    )
    DB.session.commit()
    return jsonify({"message": "User added successfully"}), 201

@user.route("/edit/<int:user_id>", methods=["POST"])
@login_required
def edit_user(user_id):
    u = Users.query.get_or_404(user_id)
    data = request.get_json()

    u.first_name = data.get("first_name", u.first_name)
    u.last_name = data.get("last_name", u.last_name)
    u.email = data.get("email", u.email)
    role_ids = data.get("roles", [])
    u.roles = Roles.query.filter(Roles.id.in_(role_ids)).all()
    DB.session.commit()
    return jsonify({"message": "User updated successfully"})