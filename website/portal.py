from flask import Blueprint, render_template, request, redirect, url_for
from flask_login import login_required
import urllib.parse
import json

from ws import lock, connected_esp_clients, update_time

portal = Blueprint('portal', __name__, url_prefix='/portal')
@portal.before_request
@login_required
def require_login():
    pass


def forward_request_to_esp32(endpoint, method="POST", esp_id=None):
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
        target_clients = connected_esp_clients if not esp_id else {esp_id: connected_esp_clients[esp_id]}

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
                            responses.append({"esp_id": ID, "response": response["content"]})
                        break

            except Exception as e:
                print(f"Error communicating with {ID}: {e}")
                responses.append({"esp_id": ID, "error": str(e)})

    return responses

@portal.route('/display')
def display():
    esp_id = request.args.get("esp_id")
    print('Request for display page received')
    return render_template('portal/display.html', prefix="/portal", esp_id=esp_id)

@portal.route("/alert")
def alert():
    return render_template("portal/alert.html", prefix="/portal")

@portal.route("/purge")
def purge():
    global connected_esp_clients
    connected_esp_clients.clear()
    update_time()

    return "", 204

@portal.route('/change')
def change():
    esp_id = request.args.get("esp_id")
    print('Request for change page received')
    return render_template('portal/change.html', prefix="/portal", esp_id=esp_id)

@portal.route('/validate_change', methods=['POST'])
def validate_change():
    forward_request_to_esp32("/validate_change", esp_id=list(connected_esp_clients.keys())[0])
    return "", 204

@portal.route('/reset', methods=['POST'])
def reset():
    esp_id = request.args.get("esp_id")
    responses = forward_request_to_esp32("/reset", esp_id=list(connected_esp_clients.keys())[0])
    for response in responses:
        if response["response"]["response"] == "success":
            return redirect("/change")
    return "", 204

@portal.route('/connect')
def connect():
    print('Request for connect page received')
    return render_template('portal/connect.html', prefix="/portal")

@portal.route('/eduroam')
def eduroam():
    print('Request for eduroam page received')
    return render_template('portal/eduroam.html', prefix="/portal")

@portal.route('/validate_connect', methods=['POST'])
def validate_connect():
    print("\n")
    esp_id = request.args.get("esp_id")
    print(f"esp_id: {esp_id}, hardcoded: {list(connected_esp_clients.keys())[0]}")
    print("\n")
    # TODO: support >1 ESP32
    responses = forward_request_to_esp32(f"/validate_connect{'?esp_id=eduroam' if esp_id=='eduroam' else ''}", esp_id=list(connected_esp_clients.keys())[0]) # TODO: fix this hardcoding
    for response in responses:
        if response["response"]["response"] == "already connected":
            message = "One or more ESP32s already connected to Wi-Fi"
            encoded_message = urllib.parse.quote(message)
            return redirect(f"/portal/alert?message={encoded_message}")

    return redirect("/display")


@portal.route('/nearby')
def nearby():
    esp_id = request.args.get("esp_id")
    print('Request for nearby page received')
    return render_template('portal/nearby.html', esp_id=esp_id)

@portal.route('/about')
def about():
    print('Request for about page received')
    return render_template('portal/about.html', prefix="/portal")

@portal.route('/device')
def device():
    print('Request for device page received')
    return render_template('portal/device.html', prefix="/portal")

@portal.route('/toggle', methods=['POST'])
def toggle():
    esp_id = request.args.get("esp_id")
    responses = forward_request_to_esp32("/toggle", esp_id=list(connected_esp_clients.keys())[0])
    for response in responses:
        if response["response"]["response"] == "led toggled":
            message = "One or more ESP32 LEDs toggled"
            encoded_message = urllib.parse.quote(message)
            return redirect(f"/portal/alert?message={encoded_message}")
    return "", 204