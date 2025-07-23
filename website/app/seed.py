from flask_security.utils import hash_password
from app.db import DB
from app.user import User, Role, user_datastore
import os
from flask_migrate import upgrade


# Build initial data for the database
def seed_data(app):
    with app.app_context():
        # Try create or update the tables first
        # Run database migrations
        print("Running database migrations...")
        upgrade()
        print("Database migrations completed.")
        # Get admin credentials from environment variables
        admin_name = os.getenv("ADMIN_NAME", "admin")
        admin_email = os.getenv("ADMIN_EMAIL", "user@admin.dev")
        admin_password = os.getenv("ADMIN_PASSWORD", "password")

        # Check for existing roles
        role_names = ["user", "superuser"]
        roles = {}
        for role_name in role_names:
            role = Role.query.filter_by(name=role_name).first()
            if not role:
                role = Role(name=role_name)
                DB.session.add(role)
                print(f"Created role '{role_name}'")
            else:
                print(f"Role '{role_name}' already exists. Skipping.")
            roles[role_name] = role
        
        DB.session.commit()

        # Check for existing admin user
        existing_admin = User.query.filter_by(email=admin_email).first()
        if existing_admin:
            print(f"Admin user with email '{admin_email}' already exists. Skipping seed.")
            return

        # Create the admin user
        admin_user = user_datastore.create_user(
            first_name=admin_name,
            email=admin_email,
            password=hash_password(admin_password),
            roles=[roles["user"], roles["superuser"]]
        )

        DB.session.commit()
        print(f"Admin user '{admin_email}' created successfully.")
