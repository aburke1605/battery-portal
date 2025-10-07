import os
import pytest
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.chrome.options import Options


@pytest.fixture(scope="module")
def driver():
    options = Options()
    options.add_argument("--headless")
    options.add_argument("--no-sandbox")
    options.add_argument("--disable-dev-shm-usage")
    options.add_argument("--ignore-certificate-errors")
    options.add_argument("--window-size=1280,800")

    service_path = os.getenv("CHROMEDRIVER_PATH", "/usr/bin/google-chrome")
    service = Service(service_path)
    driver = webdriver.Chrome(options=options)  # Selenium Manager auto-downloads driver
    yield driver
    driver.quit()

# Common test configuration
@pytest.fixture(scope="session")
def base_url() -> str:
    return os.getenv("BASE_URL", "http://localhost:5000")


@pytest.fixture(scope="session")
def wait_timeout() -> int:
    return int(os.getenv("E2E_WAIT_TIMEOUT", "10"))

