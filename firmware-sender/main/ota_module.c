#include "ota_module.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_server.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "state_machine.h"
#include <string.h>

static const char *TAG = "OTA_MODULE";
static httpd_handle_t ota_http_server = NULL;

/* WiFi credentials for OTA mode */
static const char* OTA_SSID = "ESP32-OTA";
static const char* OTA_PASSWORD = "12345678";

/* OTA button pin */
static const uint8_t OTA_BUTTON_PIN_INPUT = 0;

/* Global OTA GPIO ring buffer */
ringbuf_t ota_gpio_ringbuf = {0};

/* Forward declaration of receive callback */
void receive_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);

static esp_err_t upload_page_handler(httpd_req_t *req) {
    const char* html = "<!DOCTYPE html><html><head><title>ESP32 OTA Update</title></head><body>"
        "<h2>ESP32 OTA Update</h2>"
        "<form method='POST' action='/update' enctype='multipart/form-data'>"
        "<input type='file' name='firmware' accept='.bin'><br><br>"
        "<input type='submit' value='Upload Firmware'>"
        "</form></body></html>";
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

static esp_err_t update_handler_func(httpd_req_t *req) {
    esp_ota_handle_t ota_handle = 0;
    const esp_partition_t *ota_partition = NULL;
    esp_err_t err;
    
    ESP_LOGI(TAG, "Starting OTA update...");
    
    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (ota_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition found");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x", 
             ota_partition->subtype, ota_partition->address);
    
    err = esp_ota_begin(ota_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    char boundary[128];
    size_t boundary_len = 0;
    
    // Get boundary from Content-Type header
    size_t hdr_len = httpd_req_get_hdr_value_len(req, "Content-Type");
    if (hdr_len > 0) {
        char *content_type = malloc(hdr_len + 1);
        httpd_req_get_hdr_value_str(req, "Content-Type", content_type, hdr_len + 1);
        
        char *boundary_start = strstr(content_type, "boundary=");
        if (boundary_start) {
            boundary_start += 9; // length of "boundary="
            strncpy(boundary, boundary_start, sizeof(boundary) - 1);
            boundary[sizeof(boundary) - 1] = '\0';
            boundary_len = strlen(boundary);
        }
        free(content_type);
    }
    
    char *buf = malloc(4096);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        esp_ota_abort(ota_handle);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    int total_received = 0;
    int binary_start = -1;
    bool headers_parsed = false;
    
    while (1) {
        int received = httpd_req_recv(req, buf, 4096);
        if (received <= 0) {
            break;
        }
        
        if (!headers_parsed) {
            // Look for the end of multipart headers (double CRLF)
            for (int i = 0; i < received - 3; i++) {
                if (buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n') {
                    binary_start = i + 4;
                    headers_parsed = true;
                    break;
                }
            }
            
            if (headers_parsed && binary_start < received) {
                // Write the binary data after headers
                int binary_len = received - binary_start;
                err = esp_ota_write(ota_handle, buf + binary_start, binary_len);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                    esp_ota_abort(ota_handle);
                    free(buf);
                    httpd_resp_send_500(req);
                    return ESP_FAIL;
                }
                total_received += binary_len;
            }
        } else {
            // Check if this chunk contains the boundary (end of file)
            if (boundary_len > 0) {
                char boundary_pattern[132];
                snprintf(boundary_pattern, sizeof(boundary_pattern), "\r\n--%s", boundary);
                
                char *boundary_pos = strstr(buf, boundary_pattern);
                if (boundary_pos) {
                    // Only write data up to the boundary
                    int data_len = boundary_pos - buf;
                    if (data_len > 0) {
                        err = esp_ota_write(ota_handle, buf, data_len);
                        if (err != ESP_OK) {
                            ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                            esp_ota_abort(ota_handle);
                            free(buf);
                            httpd_resp_send_500(req);
                            return ESP_FAIL;
                        }
                        total_received += data_len;
                    }
                    break; // End of file reached
                }
            }
            
            // Write the entire chunk
            err = esp_ota_write(ota_handle, buf, received);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                esp_ota_abort(ota_handle);
                free(buf);
                httpd_resp_send_500(req);
                return ESP_FAIL;
            }
            total_received += received;
        }
    }
    
    free(buf);
    
    ESP_LOGI(TAG, "Total binary data length: %d", total_received);
    
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    err = esp_ota_set_boot_partition(ota_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "OTA update successful! Rebooting...");
    httpd_resp_sendstr(req, "Update successful! Rebooting...");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    
    return ESP_OK;
}

void ota_setup(void) {
    esp_now_deinit();

    wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = OTA_SSID,
            .ssid_len = strlen(OTA_SSID),
            .password = OTA_PASSWORD,
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    esp_wifi_stop();
    esp_wifi_deinit();

    esp_wifi_init(&wifiCfg);
    esp_wifi_set_mode(WIFI_MODE_AP);

    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
    
    http_server_setup();
    set_state(STATE_IDLE);
}

void ota_teardown(void) {
    http_server_stop();
    
    esp_wifi_stop();
    esp_wifi_deinit();

    wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifiCfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    esp_now_init();
    esp_now_register_recv_cb(receive_cb);
    set_state(STATE_IDLE);
}

void http_server_setup(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    
    if (httpd_start(&ota_http_server, &config) == ESP_OK) {
        // Upload page handler
        httpd_uri_t upload_page = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = upload_page_handler
        };
        
        httpd_register_uri_handler(ota_http_server, &upload_page);
        
        // Firmware update handler
        httpd_uri_t update_handler = {
            .uri = "/update",
            .method = HTTP_POST,
            .handler = update_handler_func
        };
    
        httpd_register_uri_handler(ota_http_server, &update_handler);
        
        ESP_LOGI(TAG, "OTA HTTP server started on 192.168.4.1");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}

void http_server_stop(void) {
    if (ota_http_server) {
        httpd_stop(ota_http_server);
        ota_http_server = NULL;
        ESP_LOGI(TAG, "OTA HTTP server stopped");
    }
}