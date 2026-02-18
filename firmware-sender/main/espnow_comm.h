#ifndef ESPNOW_COMM_H
#define ESPNOW_COMM_H

#include <stdint.h>
#include "esp_now.h"

#define FIRMWARE_VERSION 1

/* --------------------------------------------------------------------------
 * Packet format sent over ESP-NOW
 * Packed to guarantee deterministic layout over RF
 * -------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint8_t version;
    uint32_t rolling_code;   // Monotonic counter for replay protection
    uint8_t  command;        // 0 = ping, 1 = bypass / force open
} espnow_data_t;

// Packet received from sender to trigger OTA update mode
typedef struct __attribute__((packed)) {
    uint8_t command;
} receiver_send_packet_t;

/* Variables */
extern bool ota_update_mode;

/* Function declarations */
void espnow_init_communication(void);
void espnow_send_packet(uint8_t command, uint32_t rolling_code);
void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
void espnow_set_link_detected_callback(void (*callback)(void));

#endif // ESPNOW_COMM_H