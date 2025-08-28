import sys
import logging

from flask import Flask
from flask_migrate import Migrate
from flask_security import Security
from flask_admin import Admin

from app.ws import sock
from app.main import main
from app.db import db, DB, BatteryInfo
from app.user import user, users, create_admin

def create_app():
    if not logging.getLogger().handlers: # only configure if root logger has no handlers (avoids duplicate setup under Gunicorn)
        logging.basicConfig(
            level=logging.INFO,
            format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
            handlers=[logging.StreamHandler(sys.stdout)],
        )
    
    app = Flask(__name__)

    app.config.from_pyfile("config.py")

    app.register_blueprint(main)

    app.register_blueprint(db)
    DB.init_app(app)
    Migrate(app, DB)

    app.register_blueprint(user)
    Security(app, users)
    Admin(app)
    create_admin(app)

    with app.app_context():
        # reset the status of all websockets existing in database on startup
        DB.session.query(BatteryInfo).filter_by(live_websocket=True).update({BatteryInfo.live_websocket: False})
        DB.session.commit()
    sock.init_app(app)

    return app