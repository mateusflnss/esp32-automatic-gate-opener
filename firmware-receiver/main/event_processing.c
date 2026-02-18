#include "event_processing.h"
#include "state_machine.h"
#include "main.h"
#include "ring_buffer.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>

/* Timing variables */
int64_t last_rx_time = 0; // Timestamp of last packet

/* Signal tracking */
signal_data_t signal_history[8] = {0};
int16_t signal_index = 0; // Circular buffer index
int16_t signal_count = 0; // Number of valid entries in buffer

/* Rolling code state */
uint32_t expected_rolling_code = 0;

bool is_getting_closer(void) {
    signed int lower_average = 0;
    signed int higher_average = 0;
    signed int total_delay_us = 0;
    signed int64_t current_time_us = esp_timer_get_time();
    
    /* First half vs second half comparison */
    for (int i = 0; i < 4; i++) {
        lower_average  += signal_history[i].rssi;
        higher_average += signal_history[i + 4].rssi;
        total_delay_us += (current_time_us - signal_history[i].timestamp_us) + (current_time_us - signal_history[i+4].timestamp_us);
    }

    bool getting_closer = higher_average > lower_average;
    bool signals_us_recent = total_delay_us < 3000000; //3 seconds
    
    return (signals_us_recent && getting_closer);
}

void update_rssi_history(uint8_t current_rssi, int64_t timestamp_us) {
    signal_history[signal_index].rssi = current_rssi;
    signal_history[signal_index].timestamp_us = timestamp_us;
    
    signal_index++;
    if (signal_index >= 8) {  // Use actual array size
        signal_index = 0; // Wrap around for circular buffer
    }
    if (signal_count < 8) {
        signal_count++;
    }
}

void process_event(const event_t *evnt) {
    switch (evnt->type) {
        case EVNT_RX_PACKET:
            if (evnt->rx.rolling_code <= expected_rolling_code) {
                return;
            }
            
            expected_rolling_code = evnt->rx.rolling_code;

            if (evnt->rx.command == CMD_FORCE_OPEN) {
                state_machine_set_state(STATE_TOGGLE);
                last_gate_state = ringbuf_is_majority_high(&gpio_ringbuf);
            } else {
                if ((last_rx_time + 300000) < evnt->rx.timestamp_us) {
                    memset(signal_history, 0, sizeof(signal_history));
                    signal_index = 0;
                }
                update_rssi_history(evnt->rx.rssi, evnt->rx.timestamp_us);
                last_rx_time = evnt->rx.timestamp_us;

                if (signal_count >= 8) { // if buffer has enough samples, check for proximity
                    if (is_getting_closer() && (esp_timer_get_time() - last_auto_open_time) > AUTO_OPEN_COOLDOWN_US) {
                        state_machine_set_state(STATE_OPEN);
                    }
                }
            }
            break;

        default:
            break;
    }
}
