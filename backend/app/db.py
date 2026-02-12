from uuid import uuid4
from datetime import datetime

from flask_sqlalchemy import SQLAlchemy
from flask_security import UserMixin, RoleMixin

DB = SQLAlchemy()


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
    fs_uniquifier = DB.Column(
        DB.String(64), unique=True, nullable=False, default=lambda: str(uuid4())
    )
    roles = DB.relationship(
        "Roles", secondary=roles_users, backref=DB.backref("users", lazy="dynamic")
    )
    subscribed = DB.Column(DB.Boolean(), default=False)
    subscription_expiry = DB.Column(DB.DateTime())
    batteries = DB.relationship(
        "BatteryInfo",
        back_populates="user",
        passive_deletes=True,
    )


class BatteryInfo(DB.Model):
    """
    Defines the structure of the battery_info table, which manages the individual battery_data tables.
    Must first create the database manually in mysql command prompt:
        $ mysql -u root -p
        mysql> create database battery_data;
    Then initiate with flask:
        $ flask db init
        $ flask db migrate -m "initiate"
        $ flask db upgrade
    Further changes made to the class here are then propagated with:
        $ flask db migrate -m "add/remove ___ column"
        $ flask db upgrade
    """

    __tablename__ = "battery_info"

    esp_id = DB.Column(DB.Integer, primary_key=True)
    root_id = DB.Column(DB.Integer, nullable=True)
    last_updated_time = DB.Column(
        DB.DateTime, default=datetime.now, onupdate=datetime.now, nullable=False
    )
    live_websocket = DB.Column(DB.Boolean, default=False, nullable=False)
    owner_id = DB.Column(
        DB.Integer,
        DB.ForeignKey("users.id", ondelete="SET NULL"),
        nullable=True,
    )
    user = DB.relationship(
        "Users",
        back_populates="batteries",
    )


class PredictionFeatures(DB.Model):
    __tablename__ = "prediction_features"
    timestamp = DB.Column(DB.DateTime, nullable=False, primary_key=True)

    # metadata
    esp_id = DB.Column(DB.Integer, nullable=False, primary_key=True)
    cycle_index = DB.Column(DB.Integer, nullable=False, primary_key=True)

    # cycle-based
    mean_temp_last_50_cycles = DB.Column(DB.Float, nullable=False)
    mean_DoD_last_50_cycles = DB.Column(DB.Float, nullable=False)
    capacity_Ah_last_50_cycles = DB.Column(DB.Float, nullable=False)
    capacity_slope_last_200_cycles = DB.Column(DB.Float, nullable=False)

    # time-based
    hours_soc_gt_90_last_7d = DB.Column(DB.Float, nullable=False)
    idle_hours_last_7d = DB.Column(DB.Float, nullable=False)

    # observable
    failure_within_7d = DB.Column(DB.Boolean, nullable=False, default=False)


class ModelMetadata(DB.Model):
    __tablename__ = "model_metadata"

    model_name = DB.Column(DB.String(255), primary_key=True)
    version = DB.Column(DB.Integer, primary_key=True)

    storage_uri = DB.Column(DB.String(255), primary_key=True)
    creation_timestamp = DB.Column(DB.DateTime, nullable=False)
    active = DB.Column(DB.Boolean, nullable=False)
