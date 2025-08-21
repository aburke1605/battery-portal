from flask_sock import Sock

from threading import Lock
import json
from urllib.parse import parse_qs

from app.db import update_battery_data, set_live_websocket

sock = Sock()
lock = Lock()


browser_clients = {}
@sock.route("/browser_ws")
def browser_ws(ws):
    esp_id = parse_qs(ws.environ.get("QUERY_STRING", "")).get("esp_id", [None])[0]
    with lock:
        browser_clients[esp_id] = ws
    try:
        while True:
            message = ws.receive()
            if not message: continue
            print(message)
    except Exception as e:
        print(f"Browser WebSocket error: {e}")
    finally:
        with lock:
            del browser_clients[esp_id]
            print("Browser WebSocket disconnected")


esp_clients = {}
@sock.route("/esp_ws")
def esp_ws(ws):
    try:
        while True:
            message = ws.receive()
            if not message: continue
            
            try:
                data_list = json.loads(message)
                with lock:
                    esp_clients[data_list[0]["esp_id"]] = { # the first entry in the list is the one making the websocket connection
                        "ws": ws,
                        "mesh_ids": [data["esp_id"] for data in data_list],
                    }
                    update_battery_data(data_list)
                for data in data_list:
                    broadcast(data)
            except Exception as e:
                print(f"ESP WebSocket error in while loop: {e}")

            broadcast() # indicate to browser clients that (at least one) esp has connected / sent an update
            ws.send(json.dumps({"type": "response", "content": "OK"}))
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
            broadcast() # indicate to browser clients that the esp has disconnected


def broadcast(data: dict | None = None) -> None:
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
