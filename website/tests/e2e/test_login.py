# tests/e2e/test_login.py
import os
import pytest
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC

BASE_URL = os.getenv("BASE_URL", "http://localhost")  # set in Docker compose.yml

# driver fixture is provided by tests/e2e/conftest.py

def test_login(driver):
    # go to login page
    driver.get(f"{BASE_URL}/#/login")

    # wait for form container (or any stable parent element)
    form = WebDriverWait(driver, 10).until(
        EC.presence_of_element_located((By.CSS_SELECTOR, "form"))
    )

    # grab elements
    email_input = form.find_element(By.CSS_SELECTOR, 'input[placeholder="info@gmail.com"]')
    password_input = form.find_element(By.CSS_SELECTOR, 'input[type="password"]')
    login_button = form.find_element(By.XPATH, '//button[contains(text(), "Sign in")]')

    # send login info
    email_input.send_keys("user@admin.dev")
    password_input.send_keys("password")
    login_button.click()

    # wait redirect after successful login
    WebDriverWait(driver, 5).until(
        EC.title_contains("Dashboard")
    )
