from flask import Flask, Response, render_template, url_for, request
from flask_socketio import SocketIO, emit

import random
import matplotlib.pyplot as plt

app = Flask(__name__)
app.config['SECRET_KEY'] = 'd0ughball$'
socketio = SocketIO(app)

# this would be done in some other script
# whether that be on a regular basis or something?
fig, ax = plt.subplots()
xs = range(100)
ys = [random.randint(1, 50) for x in xs]
ax.plot(xs, ys)
plt.savefig('static/plots/plot.png')

data_store = {}

@app.route('/')
def index():
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

@socketio.on('connect')
def handle_connect():
    print('Client connected')


if __name__ == '__main__':
    app.run(debug=True)