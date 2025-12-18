#include <stfs/raid.h>
#include <iostream>

std::vector<PhysicalLocation> Raid0::map_logical_to_physical(uint64_t block_id, const std::vector<DiskLayout> &disks_layout) const {
    uint64_t num_disks = disks_layout.size();

    uint8_t target_disk_id = block_id % num_disks;

    uint64_t stripe_row_index = block_id / num_disks;

    uint64_t physical_block = stripe_row_index + block_id;

    if (physical_block >= disks_layout[target_disk_id].total_blocks) {
        std::cerr << "Error: Address out of bounds!" << std::endl;
    }

    return std::vector<PhysicalLocation> {{ target_disk_id, physical_block }};
}

uint8_t Raid0::get_type() const { return 0; }