#include <stfs/raid.h>
#include <iostream>

std::vector<PhysicalLocation> Raid0::map_logical_to_physical(uint64_t block_id, const std::vector<DiskLayout> &disks_layout) const {
    uint64_t num_disks = disks_layout.size();

    if (num_disks == 0) {
        std::cerr << "Error: No disks in RAID layout!" << std::endl;
        return {};
    }

    uint8_t target_disk_id = block_id % num_disks;

    uint64_t physical_block = block_id / num_disks;

    if (physical_block >= disks_layout[target_disk_id].total_blocks) {
        std::cerr << "Error: Address out of bounds on disk " << (int)target_disk_id 
                  << " (Request: " << physical_block 
                  << ", Max: " << disks_layout[target_disk_id].total_blocks << ")" << std::endl;
        return {};
    }

    return std::vector<PhysicalLocation> {{ target_disk_id, physical_block }};
}

uint8_t Raid0::get_type() const { return 0; }