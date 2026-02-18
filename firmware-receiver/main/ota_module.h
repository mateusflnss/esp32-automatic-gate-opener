#ifndef OTA_MODULE_H
#define OTA_MODULE_H

#include "ring_buffer.h"

void ota_setup(void);
void ota_teardown(void);
void http_server_setup(void);
void http_server_stop(void);

extern ringbuf_t ota_gpio_ringbuf;

#endif // OTA_MODULE_H
