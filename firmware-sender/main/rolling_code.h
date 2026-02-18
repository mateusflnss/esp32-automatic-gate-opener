#ifndef ROLLING_CODE_H
#define ROLLING_CODE_H

#include <stdint.h>

#define SAVE_ROLLING_CODE_DELAY_US 21600000000ULL  // Save rolling code every 6 hours

/* Function declarations */
int32_t rolling_code_init();
void rolling_code_save(int32_t rolling_code);
uint32_t rolling_code_get_and_increment(int32_t rolling_code);
void rolling_code_periodic_save(int32_t rolling_code);

#endif // ROLLING_CODE_H