#include "espnow_comm.h"
#include "esp_log.h"
#include <string.h>



static const char *TAG = "ESPNOW_COMM";
static int64_t last_ota_command_time = 0; // Track last OTA command sent to avoid duplicates
static int16_t ota_command_received_count = 0; // Count OTA commands received in a short period
static const int OTA_COMMAND_TRESHOLD = 5; // Number of OTA commands to confirm sender's intent

/* Receiver MAC address (fixed peer) */
static uint8_t receiver_mac[6] = {0x3C, 0x8A, 0x1F, 0x0B, 0xE3, 0xD8};

/* Callback function pointer for link detection */
static void (*link_detected_callback)(void) = NULL;

/* --------------------------------------------------------------------------
 * ESP-NOW send callback
 * Used ONLY as a heuristic for link presence
 * -------------------------------------------------------------------------- */
void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS && link_detected_callback) {
        link_detected_callback();
    }
}

void receive_cb(const esp_now_recv_info_t *recv_info,
                const uint8_t *data,
                int len) {
    if (len != sizeof(espnow_data_t)) {
        ESP_LOGW(TAG, "Invalid packet size: %d", len);
        return;
    }
    if (((espnow_data_t *)data)->version != FIRMWARE_VERSION) {
        ESP_LOGW(TAG, "Unsupported protocol version: %d",
                 ((espnow_data_t *)data)->version);
        return;
    }
    int64_t time = esp_get_time();
    if (((espnow_data_t *)data)->command == 2) { // Sender OTA request
        // Multi sample OTA button state to avoid false triggers
        if (time - last_ota_command_time > 5000000LL) { // max 5 seconds between commands to count as one OTA request
            last_ota_command_time = time;
            ota_command_received_count = 1;
        } else {
            ota_command_received_count++;
            if (ota_command_received_count >= OTA_COMMAND_TRESHOLD) {
                ESP_LOGI(TAG, "Received sender OTA request");
                ota_update_mode = true;
            }
            last_ota_command_time = time; // Update last command time on every command to reset the count if commands are spaced out
        }
    } 
}



/* --------------------------------------------------------------------------
 * Initialize ESP-NOW communication
 * -------------------------------------------------------------------------- */
void espnow_init_communication(void) {
    /* ESP-NOW setup */
    esp_now_init();
    esp_now_register_send_cb(espnow_send_cb);

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, receiver_mac, 6);
    peer.channel = 1;
    peer.encrypt = false;   // Enable later with proper keys
    esp_now_add_peer(&peer);

    ESP_LOGI(TAG, "ESP-NOW communication initialized");
}

/* --------------------------------------------------------------------------
 * Packet transmission
 * -------------------------------------------------------------------------- */
void espnow_send_packet(uint8_t command, uint32_t rolling_code) {
    espnow_data_t pkt = {
        .version      = FIRMWARE_VERSION,
        .rolling_code = rolling_code,
        .command      = command
    };

    esp_now_send(receiver_mac, (uint8_t *)&pkt, sizeof(pkt));
}

/* --------------------------------------------------------------------------
 * Set callback for link detection
 * -------------------------------------------------------------------------- */
void espnow_set_link_detected_callback(void (*callback)(void)) {
    link_detected_callback = callback;
}