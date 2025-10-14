from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import time

def test_user(selenium_driver, base_url, wait_timeout):
    selenium_driver.get(f"{base_url}/#/userlist")
    
    # Wait for and verify the table is present
    table = WebDriverWait(selenium_driver, wait_timeout).until(
        EC.presence_of_element_located((By.TAG_NAME, "table"))
    )
    assert table is not None
    
    # Find the row containing the admin user
    admin_row = WebDriverWait(selenium_driver, wait_timeout).until(
        EC.presence_of_element_located(
            (By.XPATH, "//tr[.//td[.//div[normalize-space()='admin']]]")
        )
    )
    assert admin_row is not None
