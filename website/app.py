#!venv/bin/python
from datetime import time, datetime

from flask import Flask, render_template, request
from flask_sqlalchemy import SQLAlchemy
from flask_socketio import SocketIO
import time

# Create Flask application
app = Flask(__name__)
app.config['SECRET_KEY'] = 'd0ughball$'
socketio = SocketIO(app)


@app.route('/')
def index():
    return render_template('Home/index.html')

data_store = {}

@app.route('/live')
def index_live():
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


# Socket connect
@socketio.on('connect', namespace='/websocket')
def on_connect():
    print('Client connected')

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
        
if __name__ == '__main__':
    # Start app
    app.run(debug=True)
