#include "ring_buffer.h"

/**
 * Add a sample to the ring buffer at the current index and advance the pointer.
 * When the buffer reaches capacity, wrap around to the beginning and mark as full.
 * 
 * @param rb Pointer to the ring buffer structure
 * @param sample Boolean value to store
 */
void ringbuf_add_sample(ringbuf_t *rb, bool sample) {
    rb->samples[rb->index] = sample;
    rb->index++;

    if (rb->index >= WINDOW_SIZE) {
        rb->index = 0;
        rb->full = true;
    }
}

/**
 * Count the number of high (true) samples in the ring buffer.
 * 
 * @param rb Pointer to the ring buffer structure
 * @return Number of high samples
 */
uint8_t ringbuf_count_high(ringbuf_t *rb) {
    uint8_t count = 0;
    /* Determine the number of valid samples: all if full, otherwise up to current index */
    uint8_t limit = rb->full ? WINDOW_SIZE : rb->index;

    for (uint8_t i = 0; i < limit; i++) {
        if (rb->samples[i]) {
            count++;
        }
    }

    return count;
}

/**
 * Determine if the majority of samples in the buffer are high.
 * 
 * @param rb Pointer to the ring buffer structure
 * @return True if more than half the samples are high, false otherwise
 */
bool ringbuf_is_majority_high(ringbuf_t *rb) {
    /* Get total number of valid samples */
    uint8_t total_count = rb->full ? WINDOW_SIZE : rb->index;
    
    if (total_count == 0) {
        return false;
    }

    uint8_t high_count = ringbuf_count_high(rb);
    return (high_count > (total_count / 2));
}