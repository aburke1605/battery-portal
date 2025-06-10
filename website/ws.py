import logging
logger = logging.getLogger(__name__)

from flask_sock import Sock
from threading import Lock, Thread
import time
import json
from urllib.parse import parse_qs

from battery import update_from_esp

time_of_last_update = None
def update_time():
    global time_of_last_update
    time_of_last_update = time.strftime('%H:%M:%S')
update_time()

sock = Sock()
lock = Lock()
response_lock = Lock()

esp_clients = set()
browser_clients = {}
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

        for esp_id, browser in browser_clients.items(): # copy set to avoid modification issues
            print('broadcasting to browser')
            try:
                if not esp_data[esp_id]:
                    continue  # no data for this esp_id
                message = json.dumps({esp_id: esp_data[esp_id]})
                browser.send(message) # broadcast to all browsers
            except Exception:
                del browser_clients[esp_id] # remove disconnected clients

def forward_to_esp32(esp_id, message):
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


                ws = dict(set(esp))["ws"]
                ws.send(message)

                return {"esp_id": esp_id, "message": "request sent to esp32"}

            except Exception as e:
                logger.error(f"Error communicating with {esp_id}: {e}")
                return {"esp_id": esp_id, "error": str(e)}

# WS connection between browser client and flask webserver
# TODO get data from database first then connect to ws
@sock.route('/browser_ws')
def browser_ws(ws):
    query_string = ws.environ.get("QUERY_STRING", "")
    query = parse_qs(query_string)
    esp_id = query.get("esp_id", [None])[0]
    print(f"New connection: esp_id={esp_id}")

    with lock:
        browser_clients[esp_id] = ws

    try:
        while True:
            message = ws.receive()
            if message is None: # browser disconnected
                break
            try:
                data = json.loads(message)
                esp_id = data["content"]["data"]["esp_id"]
                forward_to_esp32(esp_id, message)

            except json.JSONDecodeError:
                logger.error("/browser_ws: invalid JSON")

    except Exception as e:
        logger.error(f"Browser WebSocket error: {e}")

    finally:
        with lock:
            del browser_clients[esp_id]  # remove browser client from dictionary

# WS connection between esp and flask webserver
# First object in array is the Master node
# [{
#     "type":"data",
#     "id":"esp1_master",
#     "content":{
#         "Q":88,"H":88,"V":41,"I":0,"aT":268,"iT":235,"BL":0,"BH":0,"CCT":0,"DCT":0,"CITL":0,"CITH":0
#     }
# }, {
#     "type":"data",
#     "id":"esp2_node",
#     "content":{
#         "Q":88,"H":88,"V":41,"I":0,"aT":268,"iT":235,"BL":0,"BH":0,"CCT":0,"DCT":0,"CITL":0,"CITH":0
#     }
# }]
@sock.route('/esp_ws')
def esp_ws(ws):
    print('esp_ws')
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
                data_list = json.loads(message)
                if not data_list:
                    break
                # TODO how to reconstruct node data structure?
                update_from_esp(data_list)
                response["type"] = "echo"
                response["content"] = message
                # Add each data to esp_clients for broadcasting
                for data in data_list:
                    esp_id = data["content"].get("esp_id", data["id"])
                    meta_data = {
                        "ws": ws,
                        "content":  json.dumps({"esp_id": esp_id, **data["content"]})
                    }
                    esp_clients.add(frozenset(meta_data.items())) # re-add updated data
                broadcast()
            except json.JSONDecodeError:
                response["content"]["status"] = "error"
                response["content"]["message"] = "invalid json"
            ws.send(json.dumps(response))

    except Exception as e:
        logger.error(f"ESP WebSocket error: {e}")

    finally:
        with lock:
            pass
            # TODO: remove esp from esp_clients 
            #esp_clients.discard(frozenset(meta_data.items())) # remove esp on disconnect
        logger.info(f"ESP disconnected. Total ESPs: {len(esp_clients)}")

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
                    logger.error(f"Error communicating with {esp_id}: {e}")

        time.sleep(20)

# thread = Thread(target=ping_esps, daemon=True)
# thread.start()
