#ifndef ESPNOW_CONFIG_H
#define ESPNOW_CONFIG_H

#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "event_processing.h"

#define SUPPORTED_PROTOCOL_VERSION 1

/* Packet format received over ESP-NOW */
typedef struct __attribute__((packed)) {
    uint8_t version;        // Protocol version
    uint32_t rolling_code;   // Monotonic counter for replay protection
    uint8_t  command;        // 0 = ping, 1 = bypass / force open if sent 2 to sender ota request
} espnow_data_t;

// Packet sent to receiver to trigger OTA update mode
typedef struct __attribute__((packed)) {
    uint8_t command;
} receiver_send_packet_t;


void receive_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
void espnow_setup(void);

extern QueueHandle_t rx_queue;

#endif // ESPNOW_CONFIG_H
