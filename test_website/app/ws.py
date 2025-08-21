from flask import request
from flask_sock import Sock

from threading import Lock
import json

from app.db import update_battery_data, set_live_websocket

sock = Sock()
lock = Lock()


browser_clients = {}
@sock.route("/browser_ws")
def browser_ws(ws):
    """
        Handles server WebSocket connections which are initiated by browser clients, and subsequent client messages.
        Connections are requested in the battery list and the individual detail pages of the admin portal:
            * on individual detail pages, the `esp_id` of the battery in question is the key used to store the browser client.
            * on the battery list page, a random 32-byte id string is used to imitate a unique '`esp_id`' for each browser client.
    """

    esp_id = request.args.get("esp_id")
    """
    TODO: what if two browsers are viewing the same battery?
          the second will overwrite the dict entry of the first,
          meaning the first will not receive websocket updates
    """
    with lock:
        browser_clients[esp_id] = ws
    print("New Browser WebSocket client")
    try:
        # listen for client messages
        while True:
            message = ws.receive()
            if not message: continue
            """
            TODO: forward_to_esp32 code
            """
    except Exception as e:
        print(f"Browser WebSocket error: {e}")
    finally:
        with lock:
            del browser_clients[esp_id]
            print("Browser WebSocket disconnected")


esp_clients = {}
@sock.route("/esp_ws")
def esp_ws(ws):
    """
        Handles server WebSocket connections which are initiated by ESP32 clients, and subsequent client messages.
        Incoming messages always have the following format:
            [ {"esp_id":"bms_001", "content":{"Q": 1,"H": 1}}, {"esp_id":"bms_002","content":{"Q": 2,"H": 2}}, ... ]
        The database is updated according to new telemetry data before each list element is parsed in turn,
        updating the browser clients currently viewing the detail page for the `esp_id` battery unit.
    """

    print("New ESP32 WebSocket client")
    try:
        # listen for client messages
        while True:
            message = ws.receive()
            if not message: continue
            response = {"type": "response", "content": "error"} # default response content
            
            try:
                data_list = json.loads(message)
                if not data_list or len(data_list) == 0: continue
                with lock:
                    esp_clients[data_list[0]["esp_id"]] = { # the first entry in the list is the one making the websocket connection (the root)
                        "ws": ws,
                        "mesh_ids": [data["esp_id"] for data in data_list],
                    }
                    update_battery_data(data_list) # updates database
                for data in data_list:
                    broadcast(data) # updates browsers currently viewing `data["esp_id"]` detail page
                response["content"] = "OK"
            except Exception as e:
                print(f"ESP WebSocket error in while loop: {e}")

            broadcast() # indicate to all browser clients that (at least one) esp has connected / sent an update
            ws.send(json.dumps(response))
    except Exception as e:
        print(f"ESP WebSocket error: {e}")
    finally:
        with lock:
            for esp_id, info in list(esp_clients.items()):
                if info["ws"] == ws:
                    for mesh_id in info["mesh_ids"]:
                        set_live_websocket(mesh_id, False)
                    del esp_clients[esp_id]
                    print(f"ESP WebSocket with esp_id={esp_id} disconnected")
                    break
            broadcast() # indicate to all browser clients that the esp has disconnected


def broadcast(data: dict | None = None) -> None:
    """
    Used to send WebSocket messages to browser clients when any ESP32 WebSocket client updates.
    The message can either be a simple forwarding of telemetry data, or a trigger to update the status of a battery unit.
    """
    if data is not None:
        # default use case
        messages = [(data["esp_id"], data)]
    else:
        # if no data is passed, just send a status update instruction to all browser clients
        messages = [(esp_id, {"esp_id": esp_id, "type": "status_update"}) for esp_id in browser_clients.keys()]

    for esp_id, message in messages:
        ws = browser_clients.get(esp_id)
        if not ws: continue
        try:
            ws.send(json.dumps(message))
        except Exception as e:
            print(f"Browser WebSocket broadcast error: {e}")
            del browser_clients[esp_id]
