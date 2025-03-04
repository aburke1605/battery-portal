from flask import Blueprint, render_template, request, redirect, url_for
from flask_login import login_required
import urllib.parse
import json

from ws import lock, esp_clients, update_time

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

    with lock:
        for esp in set(esp_clients):
            content = dict(esp)["content"]
            if content == None:
                # still registering
                continue
            content = json.loads(content)
            if content["esp_id"] != esp_id:
                # not the right one
                continue

            ws = dict(set(esp))["ws"]

            try:
                if method == "POST":
                    message["content"]["data"] = request.form.to_dict()
                ws.send(json.dumps(message))

                while True:
                    response = ws.receive()
                    if response:
                        response = json.loads(response)
                        # TODO
                        # TODO
                        if response["type"] == "response":
                            return response["content"]
                        # TODO
                        # TODO
                        break

            except Exception as e:
                print(f"Error communicating with {esp_id}: {e}")
                return {"esp_id": esp_id, "error": str(e)}

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
    esp_id = request.args.get("esp_id")
    response = forward_request_to_esp32("/validate_change", esp_id=esp_id)
    # TODO
    # TODO
    # if response["content"] == "success":
    #     pass
    # TODO
    # TODO
    return "", 204

@portal.route('/reset', methods=['POST'])
def reset():
    esp_id = request.args.get("esp_id")
    response = forward_request_to_esp32("/reset", esp_id=esp_id)
    # TODO
    # TODO
    # if response["content"] == "success":
    #     return redirect(f"/change?esp_id={esp_id}")
    # TODO
    # TODO
    return redirect(f"/portal/change?esp_id={esp_id}")

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
    esp_id = request.args.get("esp_id")
    response = forward_request_to_esp32("/validate_connect", esp_id=esp_id)
    # TODO
    # TODO
    # if response["content"] == "already connected":
    #     message = "One or more ESP32s already connected to Wi-Fi"
    #     encoded_message = urllib.parse.quote(message)
    #     return redirect(f"/portal/alert?message={encoded_message}")
    # TODO
    # TODO
    return redirect(f"/portal/display?esp_id={esp_id}")


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
    response = forward_request_to_esp32("/toggle", esp_id=esp_id)
    # TODO
    # TODO
    # if response["content"] == "led toggled":
    #     message = "One or more ESP32 LEDs toggled"
    #     encoded_message = urllib.parse.quote(message)
    #     return redirect(f"/portal/alert?message={encoded_message}")
    # TODO
    # TODO
    return "", 204
