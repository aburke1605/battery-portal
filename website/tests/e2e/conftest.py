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

    service_path = os.getenv("CHROMEDRIVER_PATH", "/usr/bin/chromedriver")
    service = Service(service_path)
    driver = webdriver.Chrome(service=service, options=options)
    yield driver
    driver.quit()


