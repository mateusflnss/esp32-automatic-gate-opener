#include "rolling_code.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_timer.h"

// Maximum difference allowed between received and current code (anti-replay window)
#define ROLLING_WINDOW 2000000UL
static const char *TAG = "ROLLING_CODE";

/* Rolling code persisted in NVS */

/* --------------------------------------------------------------------------
 * Rolling code persistence
 * -------------------------------------------------------------------------- */

/// Loads rolling code from NVS flash storage, initializes to 1 on first boot
void load_rolling_code(rolling_code_t *rc) {
    nvs_handle_t nvs;
    uint32_t rolling_code = 0;

    // Open NVS namespace "sec" with read/write permissions
    if (nvs_open("sec", NVS_READWRITE, &nvs) != ESP_OK) {
        rolling_code = 0;  // Default to 0 if NVS access fails
    }

    // Retrieve stored rolling code from NVS or initialize on first boot
    if (nvs_get_u32(nvs, "roll", &rolling_code) == ESP_ERR_NVS_NOT_FOUND) {
        rolling_code = 1;              // First boot: set initial code to 1
        nvs_set_u32(nvs, "roll", rolling_code);  // Store initial value
        nvs_commit(nvs);  // Commit changes to NVS flash
    }

    nvs_close(nvs);  // Close NVS handle
    ESP_LOGI(TAG, "Loaded rolling code: %lu", rolling_code);  // Log loaded value
    rc->code = rolling_code;  // Update structure with loaded code
    rc->last_saved_code = rolling_code;  // Track last saved state
    rc->last_save_timestamp = esp_timer_get_time();  // Record load time
}

/// Initializes rolling code structure with values from NVS
void rolling_code_init(rolling_code_t *rc) {
    load_rolling_code(rc);  // Load from persistent storage
    rc->code;  // NOTE: This line has no effect; consider removing
}

/// Persists current rolling code to NVS flash storage
void rolling_code_save(rolling_code_t *rc) {
    nvs_handle_t nvs;
    if (nvs_open("sec", NVS_READWRITE, &nvs) == ESP_OK) {  // Open NVS handle
        nvs_set_u32(nvs, "roll", rc->code);  // Write current code to NVS
        nvs_commit(nvs);  // Persist to flash
        nvs_close(nvs);  // Release NVS handle
        ESP_LOGI(TAG, "Saved rolling code: %lu", rc->code);
    }
}

/// Compares two codes accounting for 32-bit overflow using signed arithmetic
static inline bool is_newer(uint32_t new_code, uint32_t current_code) {
    return (int32_t)(new_code - current_code) > 0;  // Handles wraparound correctly
}

/// Validates received code is within rolling window and newer than current code
bool rolling_code_authenticate(rolling_code_t *rc, uint32_t received_code) {
    // Check if code is newer AND not too far ahead (within rolling window)
    if (is_newer(received_code, rc->code) && 
        is_newer(rc->code, received_code - ROLLING_WINDOW)) {
        rc->code = received_code;  // Accept and update to new code
        return true;
    }
    return false;  // Reject invalid or replayed code
}

/// Increments and returns the next rolling code
uint32_t rolling_code_get_and_increment(rolling_code_t *rc) {
    return ++rc->code;  // Pre-increment and return new value
}

/// Periodically saves rolling code to NVS to reduce flash wear while preventing desync
void rolling_code_periodic_save(rolling_code_t *rc, int64_t save_delay_us) {
    uint64_t current_time = esp_timer_get_time();  // Get current timestamp
    
    // Save only if enough time has passed since last save to reduce flash wear
    if ((current_time - rc->last_save_timestamp) > save_delay_us) {
        // Only save if code has actually incremented since last save
        if (is_newer(rc->code, rc->last_saved_code)){
            rc->last_saved_code = rc->code;  // Update saved code tracker
            rolling_code_save(rc);  // Persist to NVS
            rc->last_save_timestamp = current_time;  // Update save timestamp
        }
    }
}
