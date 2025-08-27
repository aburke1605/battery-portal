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

    browser_id = request.args.get("browser_id")
    esp_id = request.args.get("esp_id")
    with lock:
        browser_clients[browser_id] = {
            "ws": ws,
            "esp_id": esp_id
        }
    print("New Browser WebSocket client")
    try:
        # listen for client messages
        while True:
            message = ws.receive()
            if not message: continue
            data = json.loads(message)
            forward_to_esp(data)
    except Exception as e:
        print(f"Browser WebSocket error: {e}")
    finally:
        with lock:
            del browser_clients[browser_id]
            print("Browser WebSocket disconnected")


def forward_to_esp(data: dict) -> None:
    """
    Used to send WebSocket messages to esp clients which contain requests made by browser clients.
    Request messages look like:
        { "type":"request", "content":{"summary":"change-settings", "data":{"esp_id":"bms_001", "OTC_threshold":550, ...}} }
    Root and node esp_ids are appended to this for the case where the request is for a mesh node.
    """

    node_id = data["content"]["data"]["esp_id"]
    if not node_id:
        print("ESP32 WebSocket forward error: could not determine node esp_id for request")
        return

    for root_id, info in esp_clients.items():
        if node_id in info["mesh_ids"]:
            try:
                # append root and node ids for mesh groups
                data["root_id"] = root_id
                data["node_id"] = node_id
                # send as json array
                info["ws"].send(json.dumps([data]))
                return
            except Exception as e:
                print(f"ESP32 WebSocket forward error: {e}")


esp_clients = {}
@sock.route("/esp_ws")
def esp_ws(ws):
    """
        Handles server WebSocket connections which are initiated by ESP32 clients, and subsequent client messages.
        Incoming messages always have the following format:
            [ {"esp_id":"bms_001","content":{"Q": 1,"H": 1}}, {"esp_id":"bms_002","content":{"Q": 2,"H": 2}}, ... ]
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
                    update_browsers(data["esp_id"]) # updates browsers currently viewing `data["esp_id"]` detail page
                response["content"] = "OK"
            except Exception as e:
                print(f"ESP WebSocket error in while loop: {e}")

            update_browsers("LIST") # indicate to browser clients on ListPage that (at least one) esp has connected / sent an update
            ws.send(json.dumps(response))
    except Exception as e:
        print(f"ESP WebSocket error: {e}")
    finally:
        with lock:
            for esp_id, info in list(esp_clients.items()):
                if info["ws"] == ws:
                    for mesh_id in info["mesh_ids"]:
                        set_live_websocket(mesh_id, False)
                        update_browsers(mesh_id)
                    del esp_clients[esp_id]
                    print(f"ESP WebSocket with esp_id={esp_id} disconnected")
                    break
            update_browsers("LIST") # indicate to browser clients on ListPage that the esp has disconnected


def update_browsers(esp_id: str) -> None:
    """
    Used to send WebSocket messages to browser clients when any ESP32 WebSocket client updates.
    The message triggers the frontend to fetch data from the database through an api.
    """
    message = {"type": "status_update"}

    for browser_id, info in browser_clients.items():
        if info["esp_id"] == esp_id:
            ws = info.get("ws")
            if not ws: continue
            try:
                message["browser_id"] = browser_id
                message["esp_id"] = esp_id
                ws.send(json.dumps(message))
            except Exception as e:
                print(f"Browser WebSocket forward error: {e}")
                del browser_clients[browser_id]
