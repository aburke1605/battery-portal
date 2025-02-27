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
        overlay_info = f"{time.strftime('%H:%M:%S')}\n\n"
        overlay_info += "| ESP32 client IDs:\n"
        for k in connected_esp_clients.keys():
            overlay_info += f"| *** {k}\n"
        overlay_info += f"\nlast updated: {time_of_last_update}"

        message = json.dumps({"overlay": overlay_info, "esps": json.dumps(connected_esp_clients)})
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

                            # the value for now is empty
                            connected_esp_clients[data["id"]] = {}

                            update_time()
                            print(f"Client connected: {ws} with id: {data['id']}.")
                            print(f"Total clients: {len(connected_esp_clients.keys())}")
                    else:
                        with lock:
                            # save the BMS data in python memory
                            connected_esp_clients[data["id"]] = data["content"]

                            # forward the BMS data to browser ws clients
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
