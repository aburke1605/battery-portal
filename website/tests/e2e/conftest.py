import os
import json
import threading
import time
import socket
import contextlib
import multiprocessing as mp
import random
import pytest
from urllib.parse import urlparse, urlunparse, urlencode
from selenium import webdriver
from selenium.webdriver.chrome.options import Options
from websocket import create_connection
from datetime import datetime

"""
Setup the test environment for the e2e tests, all these are setup in a session scope.
1. Start the Flask server in a background process.
2. Start the Selenium chrome driver in a background process.
3. Start the ESP32 websocket telemetry in a background process.
TODO:
- Run in CI/CD pipeline
"""

# Start the Flask app in a background process for the duration of the test session.
@pytest.fixture(scope="session", autouse=True)
def server():
    proc = mp.Process(target=_run_flask_server, daemon=True)
    proc.start()
    ok = _wait_for_port("127.0.0.1", 5000, timeout=30)
    if not ok:
        proc.terminate()
        raise RuntimeError("Server failed to start on 127.0.0.1:5000")
    yield
    proc.terminate()
    proc.join(timeout=5)

# Start the Selenium chrome driver in a background process for the duration of the test session.
@pytest.fixture(scope="session", autouse=True)
def selenium_driver():
    options = Options()
    #options.add_argument("--headless")
    options.add_argument("--no-sandbox")
    options.add_argument("--disable-dev-shm-usage")
    options.add_argument("--ignore-certificate-errors")
    options.add_argument("--window-size=1280,800")
    # Selenium Manager auto-downloads driver
    driver = webdriver.Chrome(options=options)
    yield driver
    driver.quit()

# Start the ESP32 websocket telemetry in a background process for the duration of the test session.
@pytest.fixture(scope="session", autouse=True)
def esp_ws_telemetry(base_url, server):
    parsed = urlparse(base_url)
    scheme = "wss" if parsed.scheme == "https" else "ws"
    ws_url = urlunparse((scheme, parsed.netloc, "/api/esp_ws", "", "", ""))
    stop_event = threading.Event()
    print(f"[esp_ws] connecting to {ws_url}")
    try:
        ws = create_connection(ws_url, timeout=5)
    except Exception as e:
        raise RuntimeError("Failed to connect to ESP32 websocket telemetry")
    def send_loop():
        if ws is None:
            print("[esp_ws] failed to connect; telemetry disabled")
            return
        print("[esp_ws] connected")
        tick = 0
        while not stop_event.is_set():
            # Generate random payload data
            tick += 1
            now_dt = datetime.now()
            now = now_dt.isoformat()
            d_val = int(f"{now_dt.day:02d}{now_dt.month:02d}{now_dt.year % 100:02d}")
            t_val = float(f"{now_dt.hour:02d}{now_dt.minute:02d}{now_dt.second:02d}.{now_dt.microsecond // 1000:03d}")
            payload = [
                {
                    "esp_id": "bms_02",
                    "content": {
                        "timestamp": now,
                        "d": d_val,
                        "t": t_val,
                        "lat": 37.7749 + random.uniform(-0.01, 0.01),
                        "lon": -122.4194 + random.uniform(-0.01, 0.01),
                        "Q": tick,
                        "H": tick,
                        "V": random.uniform(115, 125),           # deci-Volts (stored V = V/10)
                        "V1": random.uniform(32, 34),            # deci-Volts per cell
                        "V2": random.uniform(32, 34),
                        "V3": random.uniform(32, 34),
                        "V4": random.uniform(32, 34),
                        "I": random.uniform(-100, 100),          # deci-Amps (stored I = I/10)
                        "I1": random.uniform(-25, 25),
                        "I2": random.uniform(-25, 25),
                        "I3": random.uniform(-25, 25),
                        "I4": random.uniform(-25, 25),
                        "aT": random.uniform(200, 300),          # deci-°C (stored T = T/10)
                        "cT": random.uniform(200, 300),
                        "T1": random.uniform(200, 300),
                        "T2": random.uniform(200, 300),
                        "T3": random.uniform(200, 300),
                        "T4": random.uniform(200, 300),
                        "OTC": random.randint(0, 100),
                        "wifi": random.choice([0, 1])
                    }
                },
                {
                    "esp_id": "bms_01", 
                    "content": {
                        "timestamp": now,
                        "d": d_val,
                        "t": t_val,
                        "lat": 37.7749 + random.uniform(-0.01, 0.01),
                        "lon": -122.4194 + random.uniform(-0.01, 0.01),
                        "Q": tick + 1,
                        "H": tick + 1,
                        "V": random.uniform(115, 125),           # deci-Volts (stored V = V/10)
                        "V1": random.uniform(32, 34),            # deci-Volts per cell
                        "V2": random.uniform(32, 34),
                        "V3": random.uniform(32, 34),
                        "V4": random.uniform(32, 34),
                        "I": random.uniform(-100, 100),          # deci-Amps (stored I = I/10)
                        "I1": random.uniform(-25, 25),
                        "I2": random.uniform(-25, 25),
                        "I3": random.uniform(-25, 25),
                        "I4": random.uniform(-25, 25),
                        "aT": random.uniform(200, 300),          # deci-°C (stored T = T/10)
                        "cT": random.uniform(200, 300),
                        "T1": random.uniform(200, 300),
                        "T2": random.uniform(200, 300),
                        "T3": random.uniform(200, 300),
                        "T4": random.uniform(200, 300),
                        "OTC": random.randint(0, 100),
                        "wifi": random.choice([0, 1])
                    }
                }
            ]
            with open("./message.txt", "a") as f:
                f.write(json.dumps(payload))
            try:
                ws.send(json.dumps(payload))
                # Best-effort read to clear server responses; ignore content
                try:
                    ws.settimeout(0.1)
                    _ = ws.recv()
                except Exception:
                    pass
                finally:
                    ws.settimeout(None)
            except Exception:
                # If send fails, stop trying for this session
                break
            time.sleep(1)
    thread = threading.Thread(target=send_loop, daemon=True)
    thread.start()
    # Make the WS available to tests
    yield
    stop_event.set()
    thread.join(timeout=2)
    try:
        if ws is not None:
            ws.close()
    except Exception:
        pass

# Common test configuration
@pytest.fixture(scope="session")
def base_url() -> str:
    return os.getenv("BASE_URL", "http://localhost:5000")

@pytest.fixture(scope="session")
def wait_timeout() -> int:
    return int(os.getenv("E2E_WAIT_TIMEOUT", "10"))

# Start Flask app for the whole test session
def _run_flask_server():
    os.environ.setdefault("FLASK_ENV", "development")
    # Use HTTP for tests to avoid certificate issues
    from app import create_app
    app = create_app()
    app.run(host="127.0.0.1", port=5000, debug=False, ssl_context=None, threaded=True)

# Wait for the server port to be open
def _wait_for_port(host: str, port: int, timeout: int = 30) -> bool:
    start = time.time()
    while time.time() - start < timeout:
        with contextlib.closing(socket.socket()) as s:
            s.settimeout(0.5)
            try:
                s.connect((host, port))
                return True
            except Exception:
                time.sleep(0.2)
    return False

# Enforce test execution order: home -> login -> battery within this e2e suite
def pytest_collection_modifyitems(config, items):
    priority = {
        "test_home.py": 0,
        "test_login.py": 1,
        "test_battery.py": 2,
    }
    def sort_key(item):
        # pytest>=8: item.path is a pathlib.Path
        filename = os.path.basename(str(getattr(item, "path", getattr(item, "fspath", ""))))
        return (priority.get(filename, 100), item.nodeid)
    items.sort(key=sort_key)