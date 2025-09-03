#pragma once
#include <iostream>
#include <cstdint>
#include <stfs/device.h>
#include <stfs/index.h>
#include <stfs/raid.h>
#include <vector>

struct Block {
    uint64_t timestamp;
    uint64_t payload_size;
    uint8_t* payload = nullptr;
    uint32_t crc32;

    Block(uint64_t timestamp, uint64_t payload_size, uint8_t* payload);
    bool verifyPayload();
};

class FS{
    private:
        std::vector<std::reference_wrapper<Device>> devices;
        RaidGovernor raid;
        Index& index;

        void writeBlock(Block block);
        Block readBlock();
    public:
        FS(std::vector<std::reference_wrapper<Device>> devices_vec, Index& index_ptr, RaidGovernor raid_governor);
        void  create_block(uint64_t timestamp, uint8_t *payload);
        void  add_block(Block block);
        Block get_block_by_id(uint64_t id);
        Block get_block_by_timestamp(uint64_t timestamp);
        void  edit_block_by_id(uint64_t id, Block block);
        void  edit_block_by_timestamp(uint64_t id, Block block);
        void  erase_block_payload_by_id(uint64_t id);
        void  erase_block_payload_by_timestamp(uint64_t id);
};