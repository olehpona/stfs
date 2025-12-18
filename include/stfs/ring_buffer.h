#pragma once
#include <cstdint>

struct RingBufferState
{
    uint64_t head_id;
    uint64_t tail_id;
    uint64_t count;
    uint64_t capacity;

    uint64_t get_next_block_id() const {
        return (tail_id + 1) % capacity;
    }
};