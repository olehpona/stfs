#pragma once
#include <vector>
#include <cstdint>

#define BLOCK_STATIC_SIZE 20

struct Block {
    uint64_t timestamp;
    uint64_t block_payload_size;
    std::vector<char> payload;
    uint32_t crc32;
    std::vector<char> serialize() const;
    size_t deserialize(const char* buffer);

    bool is_valid() const;
    void update_crc();
};