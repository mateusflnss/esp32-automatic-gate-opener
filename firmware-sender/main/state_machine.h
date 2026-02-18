#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>

/* --------------------------------------------------------------------------
 * State machine definitions
 * -------------------------------------------------------------------------- */
typedef enum {
    STATE_IDLE,       // Slow ping
    STATE_DETECTS,    // Faster ping (link detected)
    STATE_BYPASS,     // Force open while button held
    STATE_COUNT       // Number of states (not a real state)
} State;

/* Function declarations */
void state_machine_init(void);
void state_machine_run(void);
void state_machine_set_state(State new_state);
State state_machine_get_current_state(void);
void state_machine_on_link_detected(void);

#endif // STATE_MACHINE_H