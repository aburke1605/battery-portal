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

@sock.route("/monitor")
def monitor(ws):
    while True:
        overlay_info = f"{time.strftime('%H:%M:%S')}\n\n"
        overlay_info += "| ESP32 client IDs:\n"
        esp_data = {}
        for esp in esp_clients:
            content = dict(esp)["content"]
            if content == None:
                # still registering
                continue
            content = json.loads(content)
            overlay_info += f"| *** {content['esp_id']}\n"
            esp_data[content.pop("esp_id")] = content
        overlay_info += f"\nlast updated: {time_of_last_update}"

        message = json.dumps({"overlay": overlay_info, "esps": json.dumps(esp_data)})
        ws.send(message)

        time.sleep(1)


esp_clients = set()
browser_clients = set()

def broadcast():
    with lock:
        esp_data = {}
        for esp in esp_clients:
            content = dict(esp)["content"]
            if content == None:
                # still registering
                continue
            content = json.loads(content)
            esp_data[content.pop("esp_id")] = content

        message = json.dumps(esp_data)

        for browser in set(browser_clients): # copy set to avoid modification issues
            try:
                browser.send(message) # broadcast to all browsers
            except Exception:
                browser_clients.discard(browser) # remove disconnected clients


@sock.route('/browser_ws')
def browser_ws(ws):
    with lock:
        browser_clients.add(ws)
    print(f"Browser connected: {ws}")
    print(f"Total browsers: {len(browser_clients)}")

    try:
        while True:
            if ws.receive() is None: # browser disconnected
                break

    except Exception as e:
        print(f"Browser WebSocket error: {e}")

    finally:
        with lock:
            browser_clients.discard(ws) # remove browser on disconnect
        print(f"Browser disconnected: {ws}")


@sock.route('/esp_ws')
def esp_ws(ws):
    meta_data = {"ws": ws, "content": None}

    try:
        while True:
            message = ws.receive()
            if message is None:  # esp disconnected
                break


            response = {
                "type": "response",
                "content": {}
            }

            try:
                data = json.loads(message)
                if data["type"] == "register":
                    with lock:
                        esp_clients.add(frozenset(meta_data.items()))
                        update_time()
                    print(f"ESP connected: {ws}")
                    print(f"Total ESPs: {len(esp_clients)}")
                else:
                    """
                    # TODO remove this in favour of data storage for later broadcast in `browser_ws()`
                    for c in browser_clients:
                        with lock:
                            c.send(json.dumps({"esp_id": data["esp_id"], **data["content"]}))
                    """
                    esp_clients.discard(frozenset(meta_data.items())) # remove old entry
                    meta_data["content"] = json.dumps({"esp_id": data["esp_id"], **data["content"]}) # update content
                    esp_clients.add(frozenset(meta_data.items())) # re-add updated data

                    broadcast() # send data to browsers

                response["type"] = "echo"
                response["content"] = message
            except json.JSONDecodeError:
                response["content"]["status"] = "error"
                response["content"]["message"] = "invalid json"

            ws.send(json.dumps(response))


    except Exception as e:
        print(f"ESP WebSocket error: {e}")


    finally:
        with lock:
            esp_clients.discard(frozenset(meta_data.items())) # remove esp on disconnect
            update_time()
        print(f"ESP disconnected: {ws}")

# TODO: add some code which periodically checks if ESP32s are still connected
