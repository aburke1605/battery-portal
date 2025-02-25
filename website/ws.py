from flask_sock import Sock
from threading import Lock
import time
import json

time_of_last_update = None
def update_time():
    global time_of_last_update
    time_of_last_update = time.strftime('%H:%M:%S')
update_time()

sock = Sock()
lock = Lock()
connected_esp_clients = dict()
connected_browser_clients = dict()

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
