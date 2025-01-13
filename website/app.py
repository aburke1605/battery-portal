#!venv/bin/python
from flask import Flask, render_template, request, jsonify, redirect, url_for
import requests
from flask_sock import Sock

# Create Flask application
app = Flask(__name__, static_folder='static', template_folder='static')
app.config['SECRET_KEY'] = 'd0ughball$'
sock = Sock(app)


# TODO: need to automatically determine this:
ESP_IP = "192.168.0.28"
def forward_request_to_esp32(endpoint, method="POST", allow_redirects=True):
    """
    Generic function to forward requests to the ESP32.
    :param endpoint: ESP32 endpoint to forward the request to.
    :param method: HTTP method ("GET", "POST", etc.).
    :param allow_redirects: Whether to allow redirects from the ESP32.
    :return: Response from the ESP32.
    """
    ESP32_URL = f"http://{ESP_IP}/{endpoint}"
    try:
        # Handle GET and POST requests
        if method == "POST":
            form_data = request.form.to_dict()  # Collect form data for POST
            response = requests.post(ESP32_URL, data=form_data, allow_redirects=allow_redirects)
        elif method == "GET":
            response = requests.get(ESP32_URL, allow_redirects=allow_redirects)
        else:
            return jsonify({'error': 'Unsupported HTTP method'}), 400

        # Handle redirect responses (if any)
        if response.status_code == 302 and not allow_redirects:
            redirect_url = response.headers.get("Location", "/")
            return redirect(redirect_url, code=302)

        # Return the ESP32's response to the client
        return response.text, response.status_code
    except requests.exceptions.RequestException as e:
        return jsonify({'error': f"Failed to communicate with ESP32: {str(e)}"}), 500

data_store = {}
connected_clients = set()  # Keep track of connected WebSocket clients

@app.route('/')
def homepage():
    print('Request for home page received')
    return render_template('portal/homepage.html')

@app.route('/login')
def login():
    print('Request for login page received')
    return render_template('portal/login.html')

@app.route('/validate_login', methods=['POST'])
def validate_login():
    username = request.form.get("username")
    password = request.form.get("password")
    if username == "admin" and password == "1234":
        return redirect(url_for("display"), code=302)
    else:
        return "<p>Invalid username or password.</p>", 401


@app.route('/display')
def display():
    print('Request for display page received')
    return render_template('portal/display.html')

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
    return render_template('portal/change.html')

@app.route('/validate_change', methods=['POST'])
def validate_change():
    return forward_request_to_esp32("validate_change")

@app.route('/reset', methods=['POST'])
def reset():
    return forward_request_to_esp32("reset", allow_redirects=False)


@app.route('/connect')
def connect():
    print('Request for connect page received')
    return render_template('portal/connect.html')

@app.route('/validate_connect', methods=['POST'])
def validate_connect():
    return forward_request_to_esp32("validate_connect")

@app.route('/nearby')
def nearby():
    print('Request for nearby page received')
    return render_template('portal/nearby.html')

@app.route('/about')
def about():
    print('Request for about page received')
    return render_template('portal/about.html')

@app.route('/device')
def device():
    print('Request for device page received')
    return render_template('portal/device.html')

@app.route('/toggle', methods=['GET'])
def toggle():
    return forward_request_to_esp32("toggle", method="GET")

if __name__ == '__main__':
    # Start app
    app.run(debug=True)
