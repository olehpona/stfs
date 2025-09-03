#include <stfs/search_engine.h>

SearchEngine::SearchEngine(const Index& idx, const DeviceHead& hd): index(idx), head(hd) {} 

uint64_t SearchEngine::real_to_logical_index(uint64_t real_id){
    return (real_id - head.first_index_id + head.total_blocks) % head.total_blocks;
}

uint64_t SearchEngine::logical_to_real_index(uint64_t logical_id){
    return (head.first_index_id + logical_id) % head.total_blocks;
}

uint64_t SearchEngine::find_block_id_by_timestamp(uint64_t timestamp) {
    uint64_t left = 0;
    uint64_t right = head.total_blocks;
    while (left < right) {
        uint64_t mid = left + (right - left) / 2;
        uint64_t found_timestamp = index.get_index_entry(logical_to_real_index(mid)).timestamp;

        if (found_timestamp >= timestamp) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    return left;
}

std::vector<uint64_t> SearchEngine::find_blocks_id_by_timestamp_range(uint64_t start, uint64_t end) {
    uint64_t left = find_block_id_by_timestamp(start);
    uint64_t right = find_block_id_by_timestamp(end);

    std::vector<uint64_t> vec;

    for (uint64_t i = left; i< right; i++) {
        vec.push_back(i);
    }

    return vec;
}