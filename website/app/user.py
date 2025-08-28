# users.py
from uuid import uuid4
from flask import Blueprint, request, jsonify
from flask_security import SQLAlchemyUserDatastore, UserMixin, RoleMixin, login_user, login_required, logout_user
from flask_security.utils import hash_password
from flask_login import current_user
from app.db import DB

# Blueprint setup
user_bp = Blueprint('user_bp', __name__, url_prefix='/api/users')

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

@user_bp.route('/login', methods=['POST'])
def api_login():
    data = request.get_json()
    email = data.get('email')
    password = data.get('password')

    user = user_datastore.find_user(email=email)
    if user and user.verify_and_update_password(password):
        login_user(user)
        return jsonify({
            'loggedIn': True,
            'email': user.email,
            'roles': [role.name for role in user.roles]  # if using roles
        })
    return jsonify({'success': False}), 401

@user_bp.route('/check-auth', methods=['GET'])
@login_required
def auth_status():
    return jsonify({
        'loggedIn': True,
        'email': current_user.email,
        'first_name': current_user.first_name,
        'last_name': current_user.last_name,
        'roles': [role.name for role in current_user.roles]  # if using roles
    })

@user_bp.route('/logout', methods=['POST'])
@login_required
def logout():
    logout_user()
    return jsonify({'message': 'Logged out'}), 200

@user_bp.route('/list')
@login_required
def list_users():
    # Get pagination parameters from query string
    page = request.args.get('page', default=1, type=int)
    per_page = request.args.get('per_page', default=10, type=int)

    # Paginate query
    pagination = User.query.paginate(page=page, per_page=per_page, error_out=False)
    users = pagination.items

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

    return jsonify({
        "users": user_list,
        "total": pagination.total,
        "pages": pagination.pages,
        "current_page": pagination.page,
        "per_page": pagination.per_page,
        "has_next": pagination.has_next,
        "has_prev": pagination.has_prev
    })

@user_bp.route('/add', methods=['POST'])
@login_required
def add_user():
    data = request.get_json()
    first_name = data.get('first_name')
    last_name = data.get('last_name')
    email = data.get('email')
    password = data.get('password')
    
    # Check if email already exists
    existing_user = User.query.filter_by(email=email).first()
    if existing_user:
        return jsonify({"error": "Email already exists. Please use a different email address."}), 400
    
    # role_ids = data.get('roles', [])
    # Only can add normal role users for now
    role_ids = [1]

    roles = Role.query.filter(Role.id.in_(role_ids)).all()

    try:
        user_datastore.create_user(
            first_name=first_name,
            last_name=last_name,
            email=email,
            password=hash_password(password),
            roles=roles
        )
        DB.session.commit()
        return jsonify({"message": "User added successfully"}), 201
    except Exception as e:
        DB.session.rollback()
        return jsonify({"error": "Failed to create user. Please try again."}), 500

@user_bp.route('/<int:user_id>', methods=['PUT'])
@login_required
def edit_user(user_id):
    user = User.query.get_or_404(user_id)
    data = request.get_json()
    
    new_email = data.get('email', user.email)
    
    # Check if email is being changed and if the new email already exists
    if new_email != user.email:
        existing_user = User.query.filter_by(email=new_email).first()
        if existing_user:
            return jsonify({"error": "Email already exists. Please use a different email address."}), 400
    
    try:
        user.first_name = data.get('first_name', user.first_name)
        user.last_name = data.get('last_name', user.last_name)
        user.email = new_email
        
        # Update password if provided
        new_password = data.get('password')
        if new_password and new_password.strip():
            user.password = hash_password(new_password)
        
        DB.session.commit()
        return jsonify({"message": "User updated successfully"}), 200
    except Exception as e:
        DB.session.rollback()
        return jsonify({"error": "Failed to update user. Please try again."}), 500

@user_bp.route('/<int:user_id>', methods=['DELETE'])
@login_required
def delete_user(user_id):
    # Prevent users from deleting themselves
    if current_user.id == user_id:
        return jsonify({"error": "You cannot delete your own account."}), 400
    
    user = User.query.get_or_404(user_id)
    
    try:
        # Store user info for response
        user_email = user.email
        user_name = f"{user.first_name} {user.last_name}"
        
        DB.session.delete(user)
        DB.session.commit()
        
        return jsonify({
            "message": f"User {user_name} ({user_email}) has been deleted successfully."
        }), 200
    except Exception as e:
        DB.session.rollback()
        return jsonify({"error": "Failed to delete user. Please try again."}), 500

@user_bp.route('/change-password', methods=['PUT'])
@login_required
def change_password():
    """Change the current user's password"""
    data = request.get_json()
    current_password = data.get('current_password')
    new_password = data.get('new_password')
    
    # Validate required fields
    if not current_password:
        return jsonify({"error": "Current password is required."}), 400
    
    if not new_password:
        return jsonify({"error": "New password is required."}), 400
    
    # Validate minimum password length
    if len(new_password) < 6:
        return jsonify({"error": "New password must be at least 6 characters long."}), 400
    
    # Verify current password
    if not current_user.verify_and_update_password(current_password):
        return jsonify({"error": "Current password is incorrect."}), 400
    
    try:
        # Update password
        current_user.password = hash_password(new_password)
        DB.session.commit()
        
        return jsonify({"message": "Password changed successfully."}), 200
    except Exception as e:
        DB.session.rollback()
        return jsonify({"error": "Failed to change password. Please try again."}), 500

@user_bp.route('/profile', methods=['PUT'])
@login_required
def update_profile():
    """Update the current user's profile information"""
    data = request.get_json()
    
    new_email = data.get('email', current_user.email)
    first_name = data.get('first_name', current_user.first_name)
    last_name = data.get('last_name', current_user.last_name)
    
    # Validate required fields
    if not first_name or not first_name.strip():
        return jsonify({"error": "First name is required."}), 400
    
    if not last_name or not last_name.strip():
        return jsonify({"error": "Last name is required."}), 400
    
    if not new_email or not new_email.strip():
        return jsonify({"error": "Email is required."}), 400
    
    # Validate email format (basic validation)
    import re
    email_pattern = r'^[^\s@]+@[^\s@]+\.[^\s@]+$'
    if not re.match(email_pattern, new_email):
        return jsonify({"error": "Please enter a valid email address."}), 400
    
    # Check if email is being changed and if the new email already exists
    if new_email != current_user.email:
        existing_user = User.query.filter_by(email=new_email).first()
        if existing_user:
            return jsonify({"error": "Email already exists. Please use a different email address."}), 400
    
    try:
        # Update profile information
        current_user.first_name = first_name.strip()
        current_user.last_name = last_name.strip()
        current_user.email = new_email.strip()
        
        DB.session.commit()
        
        return jsonify({
            "message": "Profile updated successfully.",
            "user": {
                "first_name": current_user.first_name,
                "last_name": current_user.last_name,
                "email": current_user.email
            }
        }), 200
    except Exception as e:
        DB.session.rollback()
        return jsonify({"error": "Failed to update profile. Please try again."}), 500

    