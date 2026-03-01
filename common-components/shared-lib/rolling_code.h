#ifndef ROLLING_CODE_H
#define ROLLING_CODE_H

#include <stdint.h>

typedef struct {
    uint32_t code;
    uint32_t last_saved_code;
    int64_t last_save_timestamp; 
} rolling_code_t;

/* Function declarations */
void rolling_code_init(rolling_code_t *rc);
void rolling_code_save(rolling_code_t *rc);
uint32_t rolling_code_get_and_increment(rolling_code_t *rc);
bool rolling_code_authenticate(rolling_code_t *rc, uint32_t received_code);
void rolling_code_periodic_save(rolling_code_t *rc, int64_t save_delay_us);


#endif // ROLLING_CODE_H