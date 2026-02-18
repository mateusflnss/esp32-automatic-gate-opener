#include <stdint.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"

#include "main.h"
#include "ring_buffer.h"
#include "ota_module.h"
#include "state_machine.h"
#include "event_processing.h"
#include "gpio_config.h"
#include "nvs_config.h"
#include "espnow_config.h"

static const char *TAG = "RECEIVER";

/* Flash write timing */
static const int64_t FLASH_WRITE_DELAY_US = 43200000000LL; // 12 hours
static int64_t last_flash_write_time = 0; // Last NVS write time

/* Global GPIO ring buffers */
ringbuf_t gpio_ringbuf = {0};
ringbuf_t sender_ota_gpio_ringbuf = {0};

/* Shared variables defined here */
bool last_gate_state = false;
int64_t last_auto_open_time = 0;
int64_t last_toggle_time = 0;

/* Timing constants */
const int64_t AUTO_OPEN_COOLDOWN_US = 120000000LL; // 2 minutes cooldown in microseconds  
const int64_t TOGGLE_COOLDOWN_US = 5000000LL; // 5 seconds cooldown in microseconds
static int64_t ota_cooldown = 0; // Cooldown timer for OTA button



/* Main application entry point */
void app_main(void) {
    /* Initialize NVS */
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    /* Check OTA rollback and mark app as valid if boot is successful */
    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    /* Check if this is the first boot after an OTA update */
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "First boot after OTA update detected");
            
            /* Perform basic system checks here if needed */
            // You could add checks for critical peripherals, network connectivity, etc.
            
            /* Mark the app as valid and cancel rollback */
            if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
                ESP_LOGI(TAG, "App marked as valid, rollback canceled");
            } else {
                ESP_LOGE(TAG, "Failed to mark app as valid");
            }
        } else if (ota_state == ESP_OTA_IMG_VALID) {
            ESP_LOGI(TAG, "Running from a valid OTA partition");
        } else if (ota_state == ESP_OTA_IMG_INVALID) {
            ESP_LOGW(TAG, "Running from an invalid OTA partition");
        }
    }

    /* Wi-Fi setup (STA mode required for ESP-NOW) */
    wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifiCfg);
    ESP_ERROR_CHECK(esp_wifi_init(&wifiCfg));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    /* Setup modules */
    gpio_setup();
    espnow_setup();
    state_machine_init();

    /* Load rolling code once at boot */
    load_expected_rolling_code();

    ESP_LOGI(TAG, "Receiver initialized, expected rolling code: %lu", expected_rolling_code);

    bool ota_update_mode = false;
    
    // Fill OTA ring buffer with initial samples
    for (int i = 0; i < WINDOW_SIZE; i++) {
        ringbuf_add_sample(&ota_gpio_ringbuf, gpio_get_level(OTA_BUTTON_PIN_INPUT));
        
    }

    /* Main loop */
    while (1) {
        ringbuf_add_sample(&gpio_ringbuf, gpio_get_level(GATE_STATUS_PIN_INPUT));
        ringbuf_add_sample(&ota_gpio_ringbuf, gpio_get_level(OTA_BUTTON_PIN_INPUT));
        
        bool ota_pressed = ringbuf_is_majority_high(&ota_gpio_ringbuf);
        if (ota_pressed && !ota_update_mode && (esp_timer_get_time() > ota_cooldown)) {
            ESP_LOGI(TAG, "OTA button pressed, entering OTA update mode...");
            ota_update_mode = true;
            ota_setup();
            ota_cooldown = esp_timer_get_time() + 5000000; // 5 seconds cooldown
        }
        
        if(ota_pressed && ota_update_mode && (esp_timer_get_time() > ota_cooldown)) {
            ESP_LOGI(TAG, "Exiting OTA update mode...");
            ota_update_mode = false;
            ota_teardown();
            ota_cooldown = esp_timer_get_time() + 5000000; // 5 seconds cooldown
        }


        event_t evnt = {.type = EVNT_RX_PACKET};
        if (xQueueReceive(rx_queue, &evnt.rx, 0) == pdTRUE) {
            process_event(&evnt);
        }
        
        if(ringbuf_is_majority_high(&ota_gpio_ringbuf)){
            state_machine_set_state(STATE_SENDER_OTA);
        }else{
            if(state_machine_get_state() == STATE_SENDER_OTA){
                state_machine_set_state(STATE_IDLE);
            }
        }

        state_machine_run();
        vTaskDelay(pdMS_TO_TICKS(5));
        
        rolling_code_periodic_save(expected_rolling_code);
    }
}
