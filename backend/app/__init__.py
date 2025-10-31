import os
import sys
import logging
from dotenv import load_dotenv

from flask import Flask, Blueprint
from flask_migrate import Migrate
from flask_security import Security
from sqlalchemy import inspect

from app.db import db, DB, BatteryInfo
from app.user import user, users, create_admin
from app.ws import ws


def create_app():
    if (
        not logging.getLogger().handlers
    ):  # only configure if root logger has no handlers (avoids duplicate setup under Gunicorn)
        logging.basicConfig(
            level=logging.INFO,
            format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
            handlers=[logging.StreamHandler(sys.stdout)],
        )
    logging.getLogger("werkzeug").setLevel(logging.WARNING)

    load_dotenv()

    app = Flask(__name__)

    app.config.from_pyfile("config.py")

    # first register main page URLs
    if os.getenv("FLASK_ENV") == "development":
        # nginx handles frontend service in production
        from app.main import main

        app.register_blueprint(main)

    # then eveything else at /api
    api = Blueprint("api", __name__, url_prefix="/api")

    api.register_blueprint(db)
    DB.init_app(app)
    Migrate(app, DB)

    api.register_blueprint(user)
    Security(app, users)

    create_admin(app)

    with app.app_context():
        inspector = inspect(DB.engine)
        if inspector.has_table(BatteryInfo.__tablename__):
            columns = [
                col["name"] for col in inspector.get_columns(BatteryInfo.__tablename__)
            ]
            if "live_websocket" in columns:
                # reset the status of all websockets existing in database on startup
                DB.session.query(BatteryInfo).filter_by(live_websocket=True).update(
                    {BatteryInfo.live_websocket: False}
                )
                DB.session.commit()
    api.register_blueprint(ws)

    # finally, register eveything at /api with main app
    app.register_blueprint(api)

    return app
