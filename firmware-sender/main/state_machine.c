#include "state_machine.h"
#include "espnow_comm.h"
#include "rolling_code.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "STATE_MACHINE";

typedef void (*state_func_t)(void);

static State current_state = STATE_IDLE;

/* Forward declarations */
static void state_idle(void);
static void state_detects(void);
static void state_bypass(void);

/* State dispatch table */
static const state_func_t state_table[STATE_COUNT] = {
    [STATE_IDLE]    = state_idle,
    [STATE_DETECTS] = state_detects,
    [STATE_BYPASS]  = state_bypass,
};

/* --------------------------------------------------------------------------
 * State implementations
 * Each state performs one iteration and returns to the main loop for event handling
 * -------------------------------------------------------------------------- */

static void state_idle(void) {
    espnow_send_packet(0, rolling_code_get_and_increment());  // Ping
    vTaskDelay(pdMS_TO_TICKS(1000));   // 1 Hz
}

static void state_detects(void) {
    espnow_send_packet(0, rolling_code_get_and_increment());  // Ping
    vTaskDelay(pdMS_TO_TICKS(250));    // 4 Hz
}

static void state_bypass(void) {
    espnow_send_packet(1, rolling_code_get_and_increment());  // Force open
    vTaskDelay(pdMS_TO_TICKS(250));    // 4 Hz
}

/* --------------------------------------------------------------------------
 * Public functions
 * -------------------------------------------------------------------------- */

void state_machine_init(void) {
    current_state = STATE_IDLE;
    ESP_LOGI(TAG, "State machine initialized");
}

/* Execute current state */
void state_machine_run(void) {
    if (current_state < STATE_COUNT) {
        state_table[current_state]();
    }
}

/* Transition helper */
void state_machine_set_state(State new_state) {
    if (new_state < STATE_COUNT && new_state != current_state) {
        current_state = new_state;
        ESP_LOGI(TAG, "State changed to %d", new_state);
    }
}

State state_machine_get_current_state(void) {
    return current_state;
}

void state_machine_on_link_detected(void) {
    if (current_state == STATE_IDLE) {
        state_machine_set_state(STATE_DETECTS);
    }
}


