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

esp_clients = {}
browser_clients = {}
pending_responses = {}

# Broadcast to all browser clients
def broadcast(esp_id, data):
    if not esp_id or not data:
        return
    if esp_id in browser_clients: # copy set to avoid modification issues
        print('Broadcasting to browser', esp_id)
        try:
            ws_browser = browser_clients[esp_id]
            message = json.dumps({esp_id: data})
            ws_browser.send(message) # broadcast to all browsers
        except Exception:
            with lock:
                del browser_clients[esp_id] # remove disconnected clients

# TODO how to forwarrd to mutiple esp clients?
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
            if message is None:
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
            del browser_clients[esp_id]

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
    print('A new ESP connected', ws)
    try:
        # TODO Consider the edge cases expection
        while True:
            message = ws.receive()
            data_list = json.loads(message)
            # Record Ids for contructing the esp_clitnes
            # TODO: handle the case when data_list is empty
            ids = [data["id"] for data in data_list]
            master_id = ids[0]
            esp_clients[master_id] = {'ws': ws, 'ids': ids, 'last_updated': time.time()}
            # Forward the message to all browser clients
            for data in data_list:
                esp_id = data["id"]
                esp_clients[esp_id] = {'ws': ws, 'content': data["content"]}
                broadcast(esp_id, data["content"])
            # Update database
            update_from_esp(data_list)
            response = {
                "type": "response",
                "content": "Ok"
            }
            ws.send(json.dumps(response))
    except Exception as e:
        logger.error(f"ESP WebSocket error: {e}")
    finally:
        logger.info(f"ESP disconnected. Total ESPs: {len(esp_clients)}")

# Check structure of esp_clients and handle offline ESPs
def ping_esps(delay=5):
    while True:
        message = {
            "type": "query",
            "content": "are you still there?",
        }
        print("Pinging ESPs...")
        print(esp_clients)
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

thread = Thread(target=ping_esps, daemon=True)
thread.start()
