import os
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC

BASE_URL = os.getenv("BASE_URL", "http://localhost")


def test_home_button_exists(driver):
    driver.get(f"{BASE_URL}/#/")

    # Wait for a button containing text 'BMS Portal'
    button = WebDriverWait(driver, 10).until(
        EC.presence_of_element_located((By.XPATH, "//button[contains(., 'BMS Portal')]"))
    )

    assert button is not None

