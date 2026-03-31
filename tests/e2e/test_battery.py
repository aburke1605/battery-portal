from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import time


# Test battery list and detail page
def test_batteries_list_has_detail_link(selenium_driver, base_url, wait_timeout):
    # Navigate to batteries list page
    selenium_driver.get(f"{base_url}/#/batteries")
    # Find the battery container that includes the test battery title
    battery_container = WebDriverWait(selenium_driver, wait_timeout).until(
        EC.presence_of_element_located(
            (
                By.XPATH,
                '//div[.//h2[contains(normalize-space(), "998")]]',
            )
        )
    )
    # Verify a detail link exists under the same container
    detail_link = battery_container.find_element(
        By.XPATH,
        './/a[normalize-space()="Details"]',
    )
    assert detail_link is not None
    # Verify an online status span exists under the same container
    online_span = battery_container.find_element(
        By.XPATH,
        './/span[contains(translate(normalize-space(), "ONLINE", "online"), "online")]',
    )
    assert online_span is not None

    # Click the detail link and wait for the detail page to load
    detail_link.click()
    # Assert the detail page shows the expected heading and status
    heading = WebDriverWait(selenium_driver, wait_timeout).until(
        EC.presence_of_element_located(
            (
                By.XPATH,
                '//h2[contains(@class, "text-2xl") and normalize-space()="BMS ID: 998"]',
            )
        )
    )
    assert heading is not None

    status_span = WebDriverWait(selenium_driver, wait_timeout).until(
        EC.presence_of_element_located(
            (
                By.XPATH,
                '//span[contains(@class, "bg-green-100") and contains(@class, "text-green-800") and contains(translate(normalize-space(), "ONLINE", "online"), "online")]',
            )
        )
    )
    assert status_span is not None
