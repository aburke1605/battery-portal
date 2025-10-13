from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import os
import time

def test_batteries(selenium_driver, base_url, wait_timeout):
    # go to login page
    selenium_driver.get(f"{base_url}/#/batteries")
        
    # Map/List button
    batteries_button = WebDriverWait(selenium_driver, wait_timeout).until(
        EC.presence_of_element_located((By.XPATH, '//span[contains(text(), "Map")]'))
    )
    assert batteries_button is not None
    time.sleep(1)
    # Battert list data
    batteries_info = WebDriverWait(selenium_driver, wait_timeout).until(
        EC.presence_of_element_located((By.XPATH, '//h2[contains(text(), "bms_02")]'))
    )
    assert batteries_info is not None

