#include "button_handler.h"
#include "ring_buffer.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "BUTTON_HANDLER";

static int64_t last_bypass_time = 0;
static ringbuf_t bypass_rb = {0};

/* --------------------------------------------------------------------------
 * Button handler initialization
 * -------------------------------------------------------------------------- */
void button_handler_init(void) {
    /* GPIO setup for bypass switch */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << INPUT_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    /* Fill ring buffer initially */
    for (int i = 0; i < WINDOW_SIZE; i++) {
        bool sample = (gpio_get_level(INPUT_PIN) == 0); // Active low
        ringbuf_add_sample(&bypass_rb, sample);
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    last_bypass_time = esp_timer_get_time();
    ESP_LOGI(TAG, "Button handler initialized");
}

/* --------------------------------------------------------------------------
 * Update button state and check for bypass condition
 * -------------------------------------------------------------------------- */
void button_handler_update(void) {
    ringbuf_add_sample(&bypass_rb, (gpio_get_level(INPUT_PIN) == 0)); // Active low
    
    if (!ringbuf_is_majority_high(&bypass_rb)) {
        last_bypass_time = esp_timer_get_time();
    }
}

/* --------------------------------------------------------------------------
 * Check if bypass is currently active
 * -------------------------------------------------------------------------- */
bool button_handler_is_bypass_active(void) {
    int64_t current_time = esp_timer_get_time();
    
    return ringbuf_is_majority_high(&bypass_rb) && 
           (current_time - last_bypass_time) < BYPASS_TIMEOUT_US;
}