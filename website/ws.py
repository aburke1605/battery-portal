from flask_sock import Sock
from threading import Lock, Thread
import time
import json

from db import update_db

time_of_last_update = None
def update_time():
    global time_of_last_update
    time_of_last_update = time.strftime('%H:%M:%S')
update_time()

sock = Sock()
lock = Lock()
response_lock = Lock()

esp_clients = set()
browser_clients = set()
pending_responses = {}

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

def forward_to_esp32(endpoint, esp_id, data):
    message = {
        "type": "request",
        "content": {
            "endpoint": endpoint,
            "method": "POST"
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

                message["content"]["data"] = data

                ws = dict(set(esp))["ws"]
                ws.send(json.dumps(message))

                return {"esp_id": esp_id, "message": "request sent to esp32"}

            except Exception as e:
                print(f"Error communicating with {esp_id}: {e}")
                return {"esp_id": esp_id, "error": str(e)}

@sock.route('/browser_ws')
def browser_ws(ws):
    with lock:
        browser_clients.add(ws)
    print(f"Browser connected: {ws}")
    print(f"Total browsers: {len(browser_clients)}")

    try:
        while True:
            message = ws.receive()
            if message is None: # browser disconnected
                break
            try:
                data = json.loads(message)
                esp_id = data["id"] # TODO change to esp_id everywhere
                endpoint = data.pop("endpoint")
                forward_to_esp32(endpoint, esp_id, data)

            except json.JSONDecodeError:
                print("/browser_ws: invalid JSON")

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
                elif data["type"] == "response":
                    # should go to forward_request_to_esp32() or ping_esps() instead
                    with response_lock:
                        pending_responses[data["esp_id"]] = data["content"]
                    continue
                else:
                    esp_clients.discard(frozenset(meta_data.items())) # remove old entry
                    meta_data["content"] = json.dumps({"esp_id": data["esp_id"], **data["content"]}) # update content
                    esp_clients.add(frozenset(meta_data.items())) # re-add updated data

                    broadcast() # send data to browsers

                    update_db(data["esp_id"], data["content"]) # update server db

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

def ping_esps(delay=5):
    while True:
        message = {
            "type": "query",
            "content": "are you still there?",
        }

        with lock:
            for esp in set(esp_clients):
                content = dict(esp)["content"]
                if content == None:
                    # still registering
                    continue

                try:
                    content = json.loads(content)
                    esp_id = content.get("esp_id")

                    ws = dict(set(esp))["ws"]
                    ws.send(json.dumps(message))

                    start = time.time()
                    response = False
                    while not response and time.time() - start < delay: # seconds
                        with response_lock:
                            if esp_id in pending_responses.keys():
                                if pending_responses.pop(esp_id)["response"] == "yes":
                                    response = True
                        time.sleep(0.1)

                    if not response:
                        esp_clients.discard(frozenset(esp))
                        update_time()

                except Exception as e:
                    print(f"Error communicating with {esp_id}: {e}")

        time.sleep(20)

thread = Thread(target=ping_esps, daemon=True)
thread.start()
