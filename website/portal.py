from flask import Blueprint, render_template, request, redirect, url_for
from flask_login import login_required
from flask_sock import Sock
import urllib.parse
from threading import Lock
import json
import time

portal = Blueprint('portal', __name__, url_prefix='/portal')
@portal.before_request
@login_required
def require_login():
    pass

sock = Sock()

time_of_last_update = None
def update_time():
    global time_of_last_update
    time_of_last_update = time.strftime('%H:%M:%S')
update_time()

connected_esp_clients = dict()
connected_browser_clients = dict()
lock = Lock()

def forward_request_to_esp32(endpoint, method="POST", id=None):
    """
    Generic function to forward requests to the ESP32 via WebSocket.
    :param endpoint: ESP32 endpoint to forward the request to.
    :param method: HTTP method ("GET", "POST", etc.).
    :return: Response from the ESP32 WebSocket.
    """

    message = {
        "type": "request",
        "content": {
            "endpoint": endpoint,
            "method": method
        }
    }

    responses = []

    with lock:
        target_clients = connected_esp_clients if not id else {id: connected_esp_clients[id]}

        if not target_clients:
            return 'No matching ESP32 connected', 404
        for ID, ws in target_clients.items():
            try:
                if method == "POST":
                    message["content"]["data"] = request.form.to_dict()
                ws.send(json.dumps(message))

                while True:
                    response = ws.receive()
                    if response:
                        response = json.loads(response)
                        if response["type"] == "response":
                            responses.append({"id": ID, "response": response["content"]})
                        break

            except Exception as e:
                print(f"Error communicating with {ID}: {e}")
                responses.append({"id": ID, "error": str(e)})

    return responses

@portal.route('/')
def homepage():
    print('Request for home page received')
    return render_template('portal/homepage.html')

@sock.route("/monitor")
def monitor(ws):
    while True:
        message = f"{time.strftime('%H:%M:%S')}\n\n"
        message += "| ESP32 client IDs:\n"
        for k in connected_esp_clients.keys():
            message += f"| *** {k}\n"
        message += f"\nlast updated: {time_of_last_update}"
        ws.send(message)
        time.sleep(1)

@portal.route('/login')
def login():
    print('Request for login page received')
    return render_template('portal/login.html')

@portal.route('/validate_login', methods=['POST'])
def validate_login():
    username = request.form.get("username")
    password = request.form.get("password")
    if username == "admin" and password == "1234":
        return redirect(url_for("display"), code=302)
    else:
        return "<p>Invalid username or password.</p>", 401


@portal.route('/display')
def display():
    print('Request for display page received')
    return render_template('portal/display.html')

@portal.route("/alert")
def alert():
    return render_template("portal/alert.html")

@sock.route('/ws')
def websocket(ws):
    global connected_esp_clients, connected_browser_clients
    metadata = {"ws": ws, "id": "unknown"}

    with lock:
        connected_browser_clients["unknown"] = ws # assume it's a browser client initially

    try:
        while True:
            # Receive a message from the client
            message = ws.receive()

            if message:
                response = {
                    "type": "response",
                    "content": {}
                }

                try:
                    data = json.loads(message)

                    if data["type"] == "register":
                        with lock:
                            metadata["id"] = data["id"]
                            # move the client to the connected clients set
                            connected_browser_clients.pop("unknown")
                            connected_esp_clients[data["id"]] = ws
                            update_time()
                            print(f"Client connected: {ws} with id: {data['id']}.")
                            print(f"Total clients: {len(connected_esp_clients.keys())}")
                    else:
                        with lock:
                            for id, client in connected_browser_clients.items():
                                try:
                                    client.send(json.dumps(data["content"]))
                                except Exception as e:
                                    print(f"Error sending to browser: {e}")
                                    connected_browser_clients.pop("unknown")

                    response["content"]["status"] = "success"
                    response["content"]["id"] = metadata["id"]

                except json.JSONDecodeError:
                    response["content"]["status"] = "error"
                    response["content"]["message"] = "invalid json"

                ws.send(json.dumps(response))

                response["type"] = "echo"
                response["content"] = message
                ws.send(json.dumps(response))
            else:
                break

    except Exception as e:
        print(f"WebSocket error: {e}")

    finally:
        with lock:
            if ws in connected_esp_clients.values():
                connected_esp_clients.pop(data["id"])
                print(f"Client disconnected. Total clients: {len(connected_esp_clients.keys())}")
            elif ws in connected_browser_clients.values():
                connected_browser_clients.pop("unknown")

# TODO: add some code which periodically checks if ESP32s are still connected

@portal.route('/change')
def change():
    print('Request for change page received')
    return render_template('portal/change.html')

@portal.route('/validate_change', methods=['POST'])
def validate_change():
    forward_request_to_esp32("/validate_change", id=list(connected_esp_clients.keys())[0])
    return "", 204

@portal.route('/reset', methods=['POST'])
def reset():
    id = request.args.get("id")
    responses = forward_request_to_esp32("/reset", id=list(connected_esp_clients.keys())[0])
    for response in responses:
        if response["response"]["response"] == "success":
            return redirect("/change")
    return "", 204

@portal.route('/connect')
def connect():
    print('Request for connect page received')
    return render_template('portal/connect.html')

@portal.route('/eduroam')
def eduroam():
    print('Request for eduroam page received')
    return render_template('portal/eduroam.html')

@portal.route('/validate_connect', methods=['POST'])
def validate_connect():
    print("\n")
    id = request.args.get("id")
    print(f"id: {id}, hardcoded: {list(connected_esp_clients.keys())[0]}")
    print("\n")
    responses = forward_request_to_esp32(f"/validate_connect{'?id=eduroam' if id=='eduroam' else ''}", id=list(connected_esp_clients.keys())[0]) # TODO: fix this hardcoding
    for response in responses:
        if response["response"]["response"] == "already connected":
            message = "One or more ESP32s already connected to Wi-Fi"
            encoded_message = urllib.parse.quote(message)
            return redirect(f"/portal/alert?message={encoded_message}")

    return redirect("/display")


@portal.route('/nearby')
def nearby():
    print('Request for nearby page received')
    return render_template('portal/nearby.html')

@portal.route('/about')
def about():
    print('Request for about page received')
    return render_template('portal/about.html')

@portal.route('/device')
def device():
    print('Request for device page received')
    return render_template('portal/device.html')

@portal.route('/toggle', methods=['POST'])
def toggle():
    id = request.args.get("id")
    responses = forward_request_to_esp32("/toggle", id=list(connected_esp_clients.keys())[0])
    for response in responses:
        if response["response"]["response"] == "led toggled":
            message = "One or more ESP32 LEDs toggled"
            encoded_message = urllib.parse.quote(message)
            return redirect(f"/portal/alert?message={encoded_message}")
    return "", 204