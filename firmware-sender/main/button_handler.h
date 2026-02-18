#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <stdbool.h>

#define INPUT_PIN 4  // GPIO pin for bypass button
#define BYPASS_TIMEOUT_US 5000000ULL  // If bypass button held for 5s, ignore since using high beam from bike

/* Function declarations */
void button_handler_init(void);
bool button_handler_is_bypass_active(void);
void button_handler_update(void);

#endif // BUTTON_HANDLER_H