import logging
logger = logging.getLogger(__name__)

from flask_sock import Sock
from threading import Lock
import time
import json
from urllib.parse import parse_qs
from app.battery_http import update_database, update_structure
from app.battery_status import esp_clients, browser_clients

sock = Sock()
lock = Lock()

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
        while True:
            message = ws.receive()
            if not message:
                continue
            data_list = json.loads(message)
            # Record Ids for contructing the esp_clitnes
            if not data_list or len(data_list) == 0:
                logger.error("Received empty data list from ESP")
                continue
            ids = [data["id"] for data in data_list]
            master_id = ids[0]
            with lock:
                esp_clients[master_id] = {'ws': ws, 'ids': ids, 'last_updated': time.time()}
            # Forward the message to all browser clients
            for data in data_list:
                esp_id = data["id"]
                broadcast(esp_id, data["content"])
            # Update database
            with lock:
                update_database(data_list)
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
def check_online(app):
    with app.app_context():
        print("Checking online ESP clients...")
        # Check if esp_clients is offline
        now = time.time()
        timeout = 30  # 30 seconds timeout for ESP clients
        with lock:
            # Reconstruct esp_clients
            esp_group_ids = [v["ids"] for v in esp_clients.values()]
            update_structure(esp_group_ids)
            for esp_id, client in list(esp_clients.items()):
                print("last_updated:", now - client['last_updated'])
                if now - client['last_updated'] > timeout:
                    print(f"ESP group master_id {esp_id} is offline.")
                    del esp_clients[esp_id]
                    continue
