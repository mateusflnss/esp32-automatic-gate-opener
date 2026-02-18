#include "state_machine.h"
#include "main.h"
#include "gpio_config.h"
#include "ring_buffer.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define OTA_COOLDOWN_US 1000000LL // 1 seconds cooldown for reciever OTA sent packets


static const char *TAG = "STATE_MACHINE";
static const bool server_mode = false; // Set to true to enable OTA server mode
static int64_t ota_cooldown = 0; // Cooldown timer for OTA button


/* Current system state */
static State current_state = STATE_IDLE;

/* Forward declarations */
void state_idle(void);
void state_open(void);
void state_toggle(void);
void state_sender_ota(void);

typedef void (*state_func_t)(void);

/* State dispatch table */
static const state_func_t state_table[STATE_COUNT] = {
    [STATE_IDLE]        = state_idle,
    [STATE_OPEN]        = state_open,
    [STATE_TOGGLE]      = state_toggle,
    [STATE_SENDER_OTA]  = state_sender_ota, 
};

void state_idle(void) {
    /* Nothing to do until a packet arrives */
}

void state_open(void) {
    /* Drive GPIO to open gate */
    if (ringbuf_is_majority_high(&gpio_ringbuf)) {
        gpio_set_level(GATE_CMD_PIN_OUT, 1);
    } else {
        gpio_set_level(GATE_CMD_PIN_OUT, 0);
        state_machine_set_state(STATE_IDLE);
        last_auto_open_time = esp_timer_get_time();
    }
}

void state_toggle(void) {
    /* Drive GPIO to close gate */
    if ((esp_timer_get_time() - last_toggle_time) < TOGGLE_COOLDOWN_US) {
        state_machine_set_state(STATE_IDLE);
        return;
    }
    
    if (ringbuf_is_majority_high(&gpio_ringbuf) == last_gate_state) {
        gpio_set_level(GATE_CMD_PIN_OUT, 1);
    } else {
        gpio_set_level(GATE_CMD_PIN_OUT, 0);
        state_machine_set_state(STATE_IDLE);
        last_toggle_time = esp_timer_get_time();
    }
}


void state_machine_init(void) {
    current_state = STATE_IDLE;
    ESP_LOGI(TAG, "State machine initialized");
}

void state_machine_run(void) {
    if (current_state < STATE_COUNT) {
        state_table[current_state]();
    }
}

void state_machine_set_state(State new_state) {
    if (new_state < STATE_COUNT && new_state != current_state) {
        current_state = new_state;
        ESP_LOGI(TAG, "State changed to %d", new_state);
    }
}

void state_sender_ota(void) {
    int64_t now = esp_timer_get_time();
    if (now - ota_cooldown < OTA_COOLDOWN_US) {
        espnow_send_packet(2, rolling_code_get_and_increment());  // Sender OTA request
        ota_cooldown = now;
    }
}

State state_machine_get_current_state(void) {
    return current_state;
}
