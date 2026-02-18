#include "espnow_config.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "ESPNOW";

QueueHandle_t rx_queue;

void receive_cb(const esp_now_recv_info_t *recv_info,
                const uint8_t *data,
                int len) {
    if (len != sizeof(espnow_data_t)) {
        ESP_LOGW(TAG, "Invalid packet size: %d", len);
        return;
    }
    if (((espnow_data_t *)data)->version != SUPPORTED_PROTOCOL_VERSION) {
        ESP_LOGW(TAG, "Unsupported protocol version: %d",
                 ((espnow_data_t *)data)->version);
        return;
    }
    rx_event_t evnt = {
        .command = ((espnow_data_t *)data)->command,
        .rolling_code = ((espnow_data_t *)data)->rolling_code,
        .rssi = recv_info->rx_ctrl->rssi,
        .timestamp_us = esp_timer_get_time(),
    };
    xQueueSendFromISR(rx_queue, &evnt, NULL);
}

void espnow_setup(void) {
    /* Create event queue */
    rx_queue = xQueueCreate(8, sizeof(rx_event_t));
    if (rx_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        return;
    }

    /* ESP-NOW setup */
    esp_now_init();
    esp_now_register_recv_cb(receive_cb);
}

void espnow_send_packet(uint8_t command){
    receiver_send_packet_t pkt = {
        .command = command
    };
    //TODO GET SENDER MAC
    esp_now_send(NULL, (uint8_t *)&pkt, sizeof(pkt)); // Broadcast to sender
}