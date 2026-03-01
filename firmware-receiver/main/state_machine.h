#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>

/* --------------------------------------------------------------------------
 * State machine definitions
 * -------------------------------------------------------------------------- */
typedef enum {
    STATE_IDLE,     // default state, waiting for events
    STATE_OPEN,       // Open gate 
    STATE_TOGGLE,     // Close gate
    STATE_SENDER_OTA, // Handle sender OTA updates
    STATE_COUNT,    // Number of states (not a real state)
} State;

/* Function declarations */
void state_machine_init(void);
void state_machine_run(void);
void state_machine_set_state(State new_state);
State state_machine_get_current_state(void);

#endif // STATE_MACHINE_H
