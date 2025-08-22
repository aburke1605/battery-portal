from flask import Flask
from flask_migrate import Migrate

from app.db import db, DB, battery_info
from app.ws import sock

def create_app():
    
    app = Flask(__name__)

    app.config.from_pyfile("config.py")

    app.register_blueprint(db)
    DB.init_app(app)
    Migrate(app, DB)

    with app.app_context():
        # reset the status of all websockets existing in database on startup
        DB.session.query(battery_info).filter_by(live_websocket=True).update({battery_info.live_websocket: False})
        DB.session.commit()
    sock.init_app(app)

    return app