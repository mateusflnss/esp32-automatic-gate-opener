#include "nvs_config.h"
#include "event_processing.h"
#include "nvs_flash.h"
#include "nvs.h"

uint32_t last_saved_rolling_code = 0; // Check if code changed before saving

void load_expected_rolling_code(void) {
    nvs_handle_t nvs;
    if (nvs_open("sec", NVS_READWRITE, &nvs) != ESP_OK) {
        expected_rolling_code = 0;
        return;
    }

    if (nvs_get_u32(nvs, "exp_roll", &expected_rolling_code) == ESP_ERR_NVS_NOT_FOUND) {
        expected_rolling_code = 1;
        nvs_set_u32(nvs, "exp_roll", expected_rolling_code);
        nvs_commit(nvs);
    }

    nvs_close(nvs);
}

void save_expected_rolling_code(void) {
    nvs_handle_t nvs;
    if (nvs_open("sec", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u32(nvs, "exp_roll", expected_rolling_code);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}
