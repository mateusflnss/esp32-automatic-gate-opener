#include "rolling_code.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "ROLLING_CODE";

/* Rolling code persisted in NVS */
static uint32_t rolling_code = 0;
static int64_t last_save_time = 0;
static int32_t last_saved_rolling_code = 0;

/* --------------------------------------------------------------------------
 * Rolling code persistence
 * -------------------------------------------------------------------------- */
static int32_t load_rolling_code(void) {
    nvs_handle_t nvs;
    uint32_t rolling_code = 0;

    if (nvs_open("sec", NVS_READWRITE, &nvs) != ESP_OK) {
        rolling_code = 0;
        return;
    }

    if (nvs_get_u32(nvs, "roll", &rolling_code) == ESP_ERR_NVS_NOT_FOUND) {
        rolling_code = 1;              // First boot initialization
        nvs_set_u32(nvs, "roll", rolling_code);
        nvs_commit(nvs);
    }

    nvs_close(nvs);
    ESP_LOGI(TAG, "Loaded rolling code: %lu", rolling_code);
    return rolling_code;
}

int32_t rolling_code_init( void ) {
    uint32_t rolling_code = load_rolling_code();
    last_save_time = esp_timer_get_time();
    last_saved_rolling_code = rolling_code;
    return rolling_code;
}

void rolling_code_save( int32_t rolling_code ) {
    nvs_handle_t nvs;
    if (nvs_open("sec", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u32(nvs, "roll", rolling_code);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "Saved rolling code: %lu", rolling_code);
    }
}

uint32_t rolling_code_get_and_increment( int32_t rolling_code ) {
    return ++rolling_code;
}

void rolling_code_periodic_save( int32_t rolling_code ) {
    uint64_t current_time = esp_timer_get_time();
    
    // Save rolling code every SAVE_ROLLING_CODE_DELAY_US if it has increased to decrease wear on NVS
    // Can get desynchronized if power loss before save though
    if ((current_time - last_save_time) > SAVE_ROLLING_CODE_DELAY_US) {
        if (rolling_code > last_saved_rolling_code) {
            last_saved_rolling_code = rolling_code;
            rolling_code_save(rolling_code);
            last_save_time = current_time;
        }
    }
}