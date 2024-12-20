#!venv/bin/python
from flask import Flask, render_template, request, jsonify, redirect
import requests
from flask_sock import Sock

# Create Flask application
app = Flask(__name__, static_folder='static', template_folder='static')
app.config['SECRET_KEY'] = 'd0ughball$'
sock = Sock(app)


data_store = {}
connected_clients = set()  # Keep track of connected WebSocket clients

@app.route('/')
def login():
    print('Request for login page received')
    return render_template('login.html')

@app.route('/validate_login', methods=['POST'])
def validate_login():
    # TODO: check user name and password is ok
    return redirect("/display", code=302)

@app.route('/display')
def display():
    print('Request for display page received')
    return render_template('display.html')

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

@app.route('/change')
def change():
    print('Request for change page received')
    return render_template('change.html')

@app.route('/validate_change', methods=['POST'])
def validate_change():
    # Replace this with the ESP32's IP or hostname
    ESP32_URL = "http://127.0.0.1:5001/update_battery" # this is set up with:
    # > netsh interface portproxy add v4tov4 listenaddress=0.0.0.0 listenport=5001 connectaddress=192.168.137.143 connectport=5000
    # (PowerShell)

    # Extract the configuration data from the client request
    config_data = request.json

    # # Forward the configuration data to the ESP32
    # try:
    #     response = requests.post(ESP32_URL, json=config_data)
    #     return jsonify({
    #         "status": "success",
    #         "esp_response": response.json()
    #     })
    # except requests.RequestException as e:
    #     return jsonify({"status": "error", "message": str(e)}), 500
    if not request.is_json:
        return {"error": "Content-Type must be application/json"}, 415

    try:
        data = request.get_json()
        print("Received configuration:", data)
        response = requests.post(ESP32_URL, data={'BL_voltage_threshold': 2000})

        if response.status_code == 200:
            return {"status": "success", "message": "Battery property updated successfully"}
        else:
            return {"status": "error", "message": "ESP32 update failed", "esp32_response": response.text}, 500
    except Exception as e:
        print(f"Error processing request: {e}")
        return {"error": "Failed to process request"}, 400

@app.route('/reset', methods=['POST'])
def reset():
    ESP32_URL = "http://192.168.137.219:5000/test"

    try:
        data = request.get_json()
        print("Received configuration:", data)
        response = requests.post(ESP32_URL, data={'test': 1})

        if response.status_code == 200:
            return {"status": "success", "message": "successful"}
        else:
            return {"status": "error", "message": "failed", "esp32_response": response.text}, 500

    except Exception as e:
        print(f"Error processing request: {e}")
        return {"error": "Failed to process request"}, 400


@app.route('/connect')
def connect():
    print('Request for connect page received')
    return render_template('connect.html')

@app.route('/validate_connect', methods=['POST'])
def validate_connect():
    # TODO: code this
    return redirect("/display", code=302)

@app.route('/nearby')
def nearby():
    print('Request for nearby page received')
    return render_template('nearby.html')

@app.route('/about')
def about():
    print('Request for about page received')
    return render_template('about.html')

@app.route('/device')
def device():
    print('Request for device page received')
    return render_template('device.html')

@app.route('/toggle')
def toggle():
    # TODO: code this
    return redirect("/display", code=302)

if __name__ == '__main__':
    # Start app
    app.run(debug=True)
