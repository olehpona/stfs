#include <stfs/search_engine.h>

uint64_t SearchEngine::logical_to_real_index(uint64_t logical_id, RingBufferState state) {
    return (state.head_id + logical_id) % state.capacity;
}

uint64_t SearchEngine::find_block_id_by_timestamp(uint64_t timestamp,  TimeStampFetcher& fetcher, RingBufferState state) {
    uint64_t left = 0;
    uint64_t right = state.count;
    while (left < right) {
        uint64_t mid = left + (right - left) / 2;
        uint64_t real_id = logical_to_real_index(mid, state);
        uint64_t found_timestamp = fetcher(real_id);
        if (found_timestamp == timestamp) {
            return logical_to_real_index(mid, state);
        }
        else if (found_timestamp > timestamp) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    return logical_to_real_index(left, state);
}
