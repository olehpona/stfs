#include <stfs/fs.h>
#include <stfs/crypto.h>

using namespace std;

Block::Block(uint64_t timestamp, uint64_t payload_size, uint8_t *payload) : timestamp(timestamp),
                                                                            payload_size(payload_size),
                                                                            payload(payload)
{
    crc32 = generate_CRC32(payload, payload_size);
}

bool Block::verifyPayload()
{
    return validate_CRC32(payload, crc32, payload_size);
}

FS::FS(vector<Device &> devices_vec, Index &index_ptr, RaidGovernor raid_governor) : devices(devices_vec),
                                                                                          index(index_ptr),
                                                                                          raid(raid_governor) {};

void FS::create_block(uint64_t timestamp, uint8_t *payload)
{
    FSHead fshead = devices[0].get().get_fs_head();
    DeviceHead devhead = devices[0].get().get_device_head();

    Block block(timestamp, fshead.block_payload_size, payload);

    uint64_t new_block_id = index.get_index_head().last_id + 1;

    vector<uint8_t> device_ids = raid.get_disks_by_block_id(new_block_id);

    for (uint8_t i : device_ids) {
        uint64_t block_addres = devhead.new_block_offset;
        index.update_index_entry(new_block_id, IndexEntry {
            .timestamp = timestamp,
            .data = IndexData {
                .device_id = i,
                .address = block_addres
            }
        });
        index.write_index_entry(new_block_id);
    }
    

}