#pragma once
#include <cstdint>
#include <functional>
#include <stfs/ring_buffer.h>

using TimeStampFetcher = std::function<uint64_t(uint64_t)>;

class SearchEngine {
    private:
        static uint64_t logical_to_real_index(uint64_t logical_id, RingBufferState state);
    public:
        static uint64_t find_block_id_by_timestamp(uint64_t timestamp, TimeStampFetcher& fetcher, RingBufferState state);
};