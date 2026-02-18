#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Module includes */
#include "espnow_comm.h"
#include "state_machine.h"
#include "rolling_code.h"
#include "button_handler.h"

static const char *TAG = "MAIN";
int64_t ota_auto_exit_timer = 0; // Time when OTA update mode should auto-exit if no activity

// sender device mac address: 3c:8a:1f:0c:18:00
// receiver device mac address: 3c:8a:1f:0b:e3:d8

/* --------------------------------------------------------------------------
 * System initialization
 * -------------------------------------------------------------------------- */
static void system_init(void) {
    /* Initialize NVS */
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    /* Wi-Fi setup (STA mode required for ESP-NOW) */
    wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifiCfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    ESP_LOGI(TAG, "System initialization complete");
}

/* --------------------------------------------------------------------------
 * Main application
 * -------------------------------------------------------------------------- */
void app_main(void) {
    /* System initialization */
    system_init();

    /* Check OTA rollback and mark app as valid if boot is successful */
    const esp_partition_t *running = esp_ota_get_running_partition();
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

    /* Initialize all modules */
    int32_t rolling_code = rolling_code_init();
    espnow_init_communication();
    button_handler_init();
    state_machine_init();

    /* Set up ESP-NOW link detection callback */
    espnow_set_link_detected_callback(state_machine_on_link_detected);

    ESP_LOGI(TAG, "Application startup complete");

    /* Main application loop */
    while (1) {
        /* Update button state */
        button_handler_update();

        /* Handle state transitions based on button */
        if (button_handler_is_bypass_active()) {
            state_machine_set_state(STATE_BYPASS);
        } else {
            if (state_machine_get_current_state() != STATE_IDLE) {
                state_machine_set_state(STATE_IDLE);
            }
        }

        /* Periodic rolling code save to reduce NVS wear */
        rolling_code_periodic_save(rolling_code);

        /* Run the state machine */
        state_machine_run();
        
        if (ota_update_mode) {
            ESP_LOGI(TAG, "Entering OTA update mode...");
            ota_setup();
            // The device will reboot after OTA, so we can break the loop here
            break;
            if (ota_auto_exit_timer == 0) {
                ota_auto_exit_timer = esp_get_time() + 300000000LL; // Auto exit after 5 minutes
            } else if (esp_get_time() > ota_auto_exit_timer) {
                ESP_LOGI(TAG, "Exiting OTA update mode due to inactivity...");
                ota_teardown();
                ota_update_mode = false;
                ota_auto_exit_timer = 0;
            }
        }
        

        /* Small delay for the main loop */
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
