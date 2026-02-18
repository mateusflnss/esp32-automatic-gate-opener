#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "ring_buffer.h"

/* Shared global variables */
extern ringbuf_t gpio_ringbuf;
extern bool last_gate_state;
extern int64_t last_auto_open_time;
extern int64_t last_toggle_time;

/* Timing constants */
extern const int64_t AUTO_OPEN_COOLDOWN_US;
extern const int64_t TOGGLE_COOLDOWN_US;

#endif // MAIN_H