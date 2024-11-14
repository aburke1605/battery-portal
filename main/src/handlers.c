#include "src/handlers.h"

#include "html/index_page.h"
#include "html/display_page.h"

// Function to handle HTTP GET requests
esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Function to parse parameters from POST data (simple key=value parsing)
void parse_post_data(const char *data, char *username, char *password) {
    // Search for "username=" and "password=" in the data
    const char *user_pos = strstr(data, "username=");
    const char *pass_pos = strstr(data, "password=");
    
    if (user_pos) {
        user_pos += strlen("username=");
        sscanf(user_pos, "%[^&]", username); // Copy up to '&' character
    }

    if (pass_pos) {
        pass_pos += strlen("password=");
        sscanf(pass_pos, "%[^&]", password); // Copy up to '&' character
    }
}

// Function to handle HTTP GET requests
esp_err_t display_handler(httpd_req_t *req) {
    // Define buffer to receive POST data
    char content[100];
    char username[50] = {0}; // Buffer for the username
    char password[50] = {0}; // Buffer for the password

    // Get the length of content in the request
    int content_len = req->content_len;
    if (content_len >= sizeof(content)) {
        httpd_resp_send_500(req); // Return error if content is too large
        return ESP_FAIL;
    }

    // Receive the data into the buffer
    int received = httpd_req_recv(req, content, content_len);
    if (received <= 0) {
        httpd_resp_send_500(req); // Error in receiving data
        return ESP_FAIL;
    }
    
    // Null-terminate the received data
    content[received] = '\0';

    // Parse the POST data for username and password
    parse_post_data(content, username, password);

    ESP_LOGI("handlers", "Received query string: %s", username);

    ESP_LOGI("handlers", "Received query string: %s", password);

    if (strcmp(username, "admin") == 0 && strcmp(password, "1234") == 0) 
    {
        httpd_resp_send(req, display_html, HTTPD_RESP_USE_STRLEN);
    } else {
        const char *error_html = "<html><body><h2>Login Failed</h2><p>Invalid username or password.</p></body></html>";
        httpd_resp_send(req, error_html, HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

esp_err_t image_get_handler(httpd_req_t *req) {
    // Path to the file in the SPIFFS partition
    const char *file_path = (const char *)req->user_ctx;
    FILE *file = fopen(file_path, "r");

    if (file == NULL) {
        ESP_LOGE("handlers", "Failed to open file: %s", file_path);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // Set the response type to indicate it's an image
    httpd_resp_set_type(req, "image/png");

    // Buffer for reading file chunks
    char buffer[1024];
    size_t read_bytes;

    // Read the file and send chunks
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, read_bytes) != ESP_OK) {
            fclose(file);
            ESP_LOGE("handlers", "Failed to send file chunk");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
    }

    // Close the file and send the final empty chunk to signal the end of the response
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}