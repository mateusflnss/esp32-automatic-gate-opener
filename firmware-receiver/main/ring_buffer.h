#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#define WINDOW_SIZE 32

typedef struct {
    bool samples[WINDOW_SIZE];
    uint8_t index;
    bool full;
} ringbuf_t;

void ringbuf_add_sample(ringbuf_t *rb, bool sample);
uint8_t ringbuf_count_high(ringbuf_t *rb);
bool ringbuf_is_majority_high(ringbuf_t *rb);

#endif // RING_BUFFER_H