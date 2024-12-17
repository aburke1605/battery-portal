#!venv/bin/python
from flask import Flask, render_template, request, jsonify
from flask_sock import Sock

# Create Flask application
app = Flask(__name__, static_folder='static', template_folder='static')
app.config['SECRET_KEY'] = 'd0ughball$'
sock = Sock(app)


data_store = {}
connected_clients = set()  # Keep track of connected WebSocket clients

@app.route('/')
def index():
    print('Request for index page received')
    return render_template('display.html')


@app.route('/data', methods=['POST'])
def receive_data():
    data = request.json
    if data:
        data_store["ESP32"] = data

        # Send the data to all connected WebSocket clients
        for client in connected_clients.copy():
            try:
                client.send(jsonify(data).get_data(as_text=True))  # Send as a JSON string
            except Exception as e:
                print(f"Error sending data to a client: {e}")
                connected_clients.remove(client)  # Remove the client if sending fails

        return {"status": "success", "data_received": data}, 200
    return {"status": "error", "message": "No data received"}, 400

@sock.route('/ws')
def websocket(ws):
    # Add the client to the connected clients set
    connected_clients.add(ws)
    try:
        while True:
            # WebSocket server can listen for incoming messages if needed
            message = ws.receive()
            if message:
                print(f"Received from client: {message}")
    except Exception as e:
        print(f"WebSocket error: {e}")
    finally:
        # Clean up when the client disconnects
        connected_clients.remove(ws)

if __name__ == '__main__':
    # Start app
    app.run(debug=True)
