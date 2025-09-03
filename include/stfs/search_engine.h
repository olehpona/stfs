#pragma once
#include <cstdint>
#include <vector>
#include <stfs/index.h>
#include <stfs/device.h>

class SearchEngine {
    private:
        const Index &index;
        const DeviceHead &head;

        uint64_t real_to_logical_index(uint64_t real_id);
        uint64_t logical_to_real_index(uint64_t logical_id);

    public:
        SearchEngine(const Index& idx, const DeviceHead& hd);
        uint64_t find_block_id_by_timestamp(uint64_t timestamp);
        std::vector<uint64_t> find_blocks_id_by_timestamp_range(uint64_t start, uint64_t end);
};