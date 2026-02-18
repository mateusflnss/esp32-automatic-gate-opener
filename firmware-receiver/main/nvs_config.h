#ifndef NVS_CONFIG_H
#define NVS_CONFIG_H

#include <stdint.h>

void load_expected_rolling_code(void);
void save_expected_rolling_code(void);

extern uint32_t last_saved_rolling_code;

#endif // NVS_CONFIG_H
