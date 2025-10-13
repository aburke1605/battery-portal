from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import os

def test_home(selenium_driver, base_url, wait_timeout):
    selenium_driver.get(f"{base_url}/#/")
    
    # Hero heading
    heading = WebDriverWait(selenium_driver, wait_timeout).until(
        EC.presence_of_element_located((By.XPATH, "//h1[contains(., 'Battery Management System Portal')]"))
    )
    assert heading is not None

    # BMS Portal CTA (the visible text is inside a <span>)
    bms_portal = WebDriverWait(selenium_driver, wait_timeout).until(
        EC.presence_of_element_located((By.XPATH, "//a[.//span[contains(., 'BMS Portal')]]"))
    )
    assert bms_portal.get_attribute("href") is not None

    # Footer sanity (one link is enough)
    footer_link = WebDriverWait(selenium_driver, wait_timeout).until(
        EC.presence_of_element_located((By.XPATH, "//footer//a[contains(., 'HIGHESS')]"))
    )
    assert footer_link is not None