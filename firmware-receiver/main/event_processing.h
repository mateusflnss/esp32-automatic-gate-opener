#ifndef EVENT_PROCESSING_H
#define EVENT_PROCESSING_H

#include <stdint.h>
#include <stdbool.h>

/* Command definitions */
#define CMD_PING       0
#define CMD_FORCE_OPEN 1

/* Event definitions */
typedef struct {
    uint8_t command;
    uint32_t rolling_code;
    uint8_t rssi;
    uint64_t timestamp_us;
} rx_event_t;

typedef enum {
    EVNT_RX_PACKET,
    EVNT_BUTTON_PRESS,
    EVNT_TIMEOUT,
} event_type_t;

typedef struct {
    event_type_t type;
    union {
        rx_event_t rx;
        uint8_t button_id;
    };
} event_t;

typedef struct {
    uint8_t rssi;
    int64_t timestamp_us;
} signal_data_t;

void process_event(const event_t *evnt);
void update_rssi_history(uint8_t current_rssi, int64_t timestamp_us);
bool is_getting_closer(void);

extern uint32_t expected_rolling_code;
extern int64_t last_rx_time;
extern signal_data_t signal_history[8];
extern int16_t signal_count;

#endif // EVENT_PROCESSING_H
