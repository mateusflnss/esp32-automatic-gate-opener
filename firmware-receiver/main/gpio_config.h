#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#include <stdint.h>

/* GPIO pin definitions */
static const uint8_t GATE_CMD_PIN_OUT = 2;       // GPIO2 for gate command output
static const uint8_t GATE_STATUS_PIN_INPUT = 4;  // GPIO4 for gate status input
static const uint8_t OTA_BUTTON_PIN_INPUT = 0;   // GPIO0 for OTA button input
static const uint8_t SENDER_OTA_PIN_INPUT = 15; // GPIO15 for OTA status input

void gpio_setup(void);

#endif // GPIO_CONFIG_H
