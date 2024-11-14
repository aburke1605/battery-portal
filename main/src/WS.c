#include "include/WS.h"
#include "include/handlers.h"

/*
 * async send function, which we put into the httpd work queue
 */
void ws_async_send(void *arg) {
    static const char * data = "Async data";
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req) {
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    if (resp_arg == NULL) {
        return ESP_ERR_NO_MEM;
    }
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    esp_err_t ret = httpd_queue_work(handle, ws_async_send, resp_arg);
    if (ret != ESP_OK) {
        free(resp_arg);
    }
    return ret;
}

/*
 * This handler echos back the received ws data
 * and triggers an async send if certain message received
 */
esp_err_t websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI("WS", "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE("WS", "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI("WS", "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE("WS", "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE("WS", "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI("WS", "Got packet with message: %s", ws_pkt.payload);
    }
    ESP_LOGI("WS", "Packet type: %d", ws_pkt.type);
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT &&
        strcmp((char*)ws_pkt.payload,"Trigger async") == 0) {
        free(buf);
        return trigger_async_send(req->handle, req);
    }

    ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE("WS", "httpd_ws_send_frame failed with %d", ret);
    }
    free(buf);
    return ret;
}

// Test websocket
esp_err_t test_websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI("WS", "Handshake done, new connection opened");
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    /* Get the frame length */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE("WS", "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI("WS", "Frame len is %d", ws_pkt.len);

    /* Receive payload if available */
    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE("WS", "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE("WS", "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI("WS", "Received message: %s", ws_pkt.payload);
    }

    // if (ws_pkt.type == HTTPD_WS_TYPE_TEXT && strcmp((char *)ws_pkt.payload, "Trigger async") == 0) {
    //     free(buf);
    //     return trigger_async_send(req->handle, req);
    // }

    // /* Send echo message back */
    // ret = httpd_ws_send_frame(req, &ws_pkt);
    // if (ret != ESP_OK) {
    //     ESP_LOGE("WS", "httpd_ws_send_frame failed with %d", ret);
    // }

    // free(buf);

    char *response_msg = "Hello from ESP32!";
    httpd_ws_frame_t ws_response;
    memset(&ws_response, 0, sizeof(httpd_ws_frame_t));
    ws_response.type = HTTPD_WS_TYPE_TEXT;
    ws_response.payload = (uint8_t *)response_msg;
    ws_response.len = strlen(response_msg);

    ret = httpd_ws_send_frame(req, &ws_response);
    if (ret != ESP_OK) {
        ESP_LOGE("WS", "Failed to send message to client with error: %d", ret);
    } else {
        ESP_LOGI("WS", "Message sent to client: %s", response_msg);
    }

    free(buf);

    return ret;
}

/* Function to periodically send a message */
void websocket_send_task(void *arg) {
    httpd_handle_t server = (httpd_handle_t)arg;
    httpd_ws_frame_t ws_pkt;
    char *message = "Hello from ESP32";

    while (true) {
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        ws_pkt.payload = (uint8_t *)message;
        ws_pkt.len = strlen(message);

        /* Send the message to all connected clients */
        for (int fd = 0; fd < CONFIG_MAX_CLIENTS; fd++) {
            esp_err_t ret = httpd_ws_send_frame_async(server, fd, &ws_pkt);
            if (ret == ESP_OK) {
                ESP_LOGI("WS", "Sent message to client %d", fd);
            } else {
                ESP_LOGE("WS", "Failed to send message to client %d, error %d", fd, ret);
            }
        }

        /* Wait for 5 seconds */
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// Start HTTP server
httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16; // Increase this number as needed

    // Start the httpd server
    ESP_LOGI("WS", "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI("WS", "Registering URI handlers");
        httpd_register_uri_handler(server, &ws);  // Register WebSocket URI

        // Register ios detect page
        httpd_uri_t hotspot_detect_get_ios = {
            .uri       = "/hotspot-detect.html",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &hotspot_detect_get_ios);

        // Register android detect page
        httpd_uri_t hotspot_detect_get_android = {
            .uri       = "/generate_204",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &hotspot_detect_get_android);

        // Register windows detect page
        httpd_uri_t hotspot_detect_get_windows = {
            .uri       = "/connecttest.txt",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &hotspot_detect_get_windows);

        httpd_uri_t hotspot_detect_get_windows2 = {
            .uri       = "/favicon.ico",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &hotspot_detect_get_windows2);

        httpd_uri_t hotspot_detect_get_windows3 = {
            .uri       = "/redirect",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &hotspot_detect_get_windows3);

        // Display page
        httpd_uri_t display_page_get = {
            .uri       = "/display",
            .method    = HTTP_POST,
            .handler   = display_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &display_page_get);

        // Add a handler for serving the image
        httpd_uri_t image_uri = {
            .uri       = "/image/aceon.png",
            .method    = HTTP_GET,
            .handler   = image_get_handler, // Function to read and send the image
            .user_ctx  = "/storage/aceon.png" // File path as user context
        };
        httpd_register_uri_handler(server, &image_uri);

        httpd_uri_t image_uri_2 = {
            .uri       = "/image/aceon2.png",
            .method    = HTTP_GET,
            .handler   = image_get_handler, // Function to read and send the image
            .user_ctx  = "/storage/aceon2.png" // File path as user context
        };
        httpd_register_uri_handler(server, &image_uri_2);

        return server;
    }

    ESP_LOGI("WS", "Error starting server!");
    return NULL;
}