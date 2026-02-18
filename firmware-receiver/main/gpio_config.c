#include "gpio_config.h"
#include "driver/gpio.h"

void gpio_setup(void) {
    /* Output GPIO configuration */
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << GATE_CMD_PIN_OUT),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    /* Input GPIO configuration for gate status */
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << GATE_STATUS_PIN_INPUT);
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
    
    /* Input GPIO configuration for OTA button */
    io_conf.pin_bit_mask = (1ULL << OTA_BUTTON_PIN_INPUT);
    gpio_config(&io_conf);
    /* Input GPIO configuration for sender OTA status */
    io_conf.pin_bit_mask = (1ULL << SENDER_OTA_PIN_INPUT);
    gpio_config(&io_conf);
}
