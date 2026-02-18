#include "ring_buffer.h"

void ringbuf_add_sample(ringbuf_t *rb, bool sample) {
    rb->samples[rb->index] = sample;
    rb->index++;

    if (rb->index >= WINDOW_SIZE) {
        rb->index = 0;
        rb->full = true;
    }
}

uint8_t ringbuf_count_high(ringbuf_t *rb) {
    uint8_t count = 0;
    uint8_t limit = 0;
    if (rb->full) {
        limit = WINDOW_SIZE;
    } else {
        limit = rb->index;
    }

    for (uint8_t i = 0; i < limit; i++) {
        if (rb->samples[i]) {
            count++;
        }
    }

    return count;
}

bool ringbuf_is_majority_high(ringbuf_t *rb) {
    uint8_t total_count = 0;
    if (rb->full) {
        total_count = WINDOW_SIZE;
    } else {
        total_count = rb->index;
    }
    
    /* If there are no samples, there can be no majority high. */
    if (total_count == 0) {
        return false;
    }

    uint8_t high_count = ringbuf_count_high(rb);
    return (high_count > (total_count / 2));
}