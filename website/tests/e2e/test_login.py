# tests/e2e/test_login.py
import os
import pytest
import time
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC


def test_login(selenium_driver, base_url, wait_timeout):
    # go to login page
    selenium_driver.get(f"{base_url}/#/login")

    # wait for form container (or any stable parent element)
    form = WebDriverWait(selenium_driver, wait_timeout).until(
        EC.presence_of_element_located((By.CSS_SELECTOR, "form"))
    )
    assert form is not None

    # grab elements
    email_input = form.find_element(By.CSS_SELECTOR, 'input[placeholder="info@gmail.com"]')
    password_input = form.find_element(By.CSS_SELECTOR, 'input[type="password"]')
    login_button = form.find_element(By.XPATH, '//button[contains(text(), "Sign in")]')
    assert email_input is not None
    assert password_input is not None
    assert login_button is not None

    # send login info
    email_input.send_keys("user@admin.dev")
    password_input.send_keys("password")
    login_button.click()

    # wait redirect after successful login
    dashboard_link = WebDriverWait(selenium_driver, wait_timeout).until(        
        EC.title_contains("Dashboard")
    )
    assert dashboard_link is not None

    time.sleep(2)
    selenium_driver.refresh()

    