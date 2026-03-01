#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

// Size of the ring buffer window
#define WINDOW_SIZE 32

// Ring buffer structure for storing boolean samples
typedef struct {
    bool samples[WINDOW_SIZE];  // Array of boolean samples
    uint8_t index;              // Current write position in the buffer
    bool full;                  // Flag indicating if buffer has wrapped around
} ringbuf_t;

// Add a new sample to the ring buffer
void ringbuf_add_sample(ringbuf_t *rb, bool sample);

// Count the number of high (true) samples in the buffer
uint8_t ringbuf_count_high(ringbuf_t *rb);

// Check if the majority of samples in the buffer are high
bool ringbuf_is_majority_high(ringbuf_t *rb);

#endif // RING_BUFFER_H