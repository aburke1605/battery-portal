from flask import Blueprint, render_template, request, redirect, url_for
from flask_login import login_required
import urllib.parse
import json
import time

from ws import lock, response_lock, esp_clients, pending_responses, update_time

portal = Blueprint('portal', __name__, url_prefix='/portal')
@portal.before_request
@login_required
def require_login():
    pass


def forward_request_to_esp32(endpoint, method="POST", esp_id=None, delay=5):
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

    with lock:
        for esp in set(esp_clients):
            content = dict(esp)["content"]
            if content == None:
                # still registering
                continue

            try:
                content = json.loads(content)
                if content.get("esp_id") != esp_id:
                    # not the right one
                    continue

                if method == "POST":
                    message["content"]["data"] = request.form.to_dict()

                ws = dict(set(esp))["ws"]
                ws.send(json.dumps(message))

                start = time.time()
                while time.time() - start < delay: # seconds
                    with response_lock:
                        if esp_id in pending_responses.keys():
                            return pending_responses.pop(esp_id)
                    time.sleep(0.1)

                return {"esp_id": esp_id, "error": "Timeout waiting for ESP response"}

            except Exception as e:
                print(f"Error communicating with {esp_id}: {e}")
                return {"esp_id": esp_id, "error": str(e)}

@portal.route('/display')
def display():
    esp_id = request.args.get("esp_id")
    print('Request for display page received')
    return render_template('portal/display.html', prefix="/portal", esp_id=esp_id)

@portal.route("/purge")
def purge():
    global esp_clients
    esp_clients.clear()
    update_time()

    return "", 204

@portal.route('/change')
def change():
    esp_id = request.args.get("esp_id")
    print('Request for change page received')
    return render_template('portal/change.html', prefix="/portal", esp_id=esp_id)

@portal.route('/validate_change', methods=['POST'])
def validate_change():
    esp_id = request.args.get("esp_id")
    response = forward_request_to_esp32("/validate_change", esp_id=esp_id, delay=20)
    if response.get("status") == "success" and response["response"] == "success":
        return "", 204
    return "", 500

@portal.route('/reset', methods=['POST'])
def reset():
    esp_id = request.args.get("esp_id")
    response = forward_request_to_esp32("/reset", esp_id=esp_id)
    if response.get("status") == "success" and response["response"] == "success":
        return "", 204
    return "", 500

@portal.route('/connect')
def connect():
    esp_id = request.args.get("esp_id")
    print('Request for connect page received')
    return render_template('portal/connect.html', prefix="/portal", esp_id=esp_id)

@portal.route('/eduroam')
def eduroam():
    esp_id = request.args.get("esp_id")
    print('Request for eduroam page received')
    return render_template('portal/eduroam.html', prefix="/portal", esp_id=esp_id)

@portal.route('/validate_connect', methods=['POST'])
def validate_connect():
    esp_id = request.args.get("esp_id")
    use_eduroam = request.args.get("eduroam")
    response = forward_request_to_esp32(f"/validate_connect{'?eduroam=true' if use_eduroam else ''}", esp_id=esp_id, delay=15)
    if response.get("status") == "success" and response["response"] == "success":
        return "", 204
    return "", 500


@portal.route('/nearby')
def nearby():
    esp_id = request.args.get("esp_id")
    print('Request for nearby page received')
    return render_template('portal/nearby.html', prefix="/portal", esp_id=esp_id)

@portal.route('/about')
def about():
    esp_id = request.args.get("esp_id")
    print('Request for about page received')
    return render_template('portal/about.html', prefix="/portal", esp_id=esp_id)

@portal.route('/device')
def device():
    esp_id = request.args.get("esp_id")
    print('Request for device page received')
    return render_template('portal/device.html', prefix="/portal", esp_id=esp_id)

@portal.route('/toggle', methods=['POST'])
def toggle():
    esp_id = request.args.get("esp_id")
    response = forward_request_to_esp32("/toggle", esp_id=esp_id)
    if response.get("status") == "success" and response["response"] == "success":
        return "", 204
    return "", 500
