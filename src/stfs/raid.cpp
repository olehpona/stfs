#include <stfs/raid.h>

std::vector<PhysicalLocation> Raid0::map_logical_to_physical(uint64_t block_id, const std::vector<DiskLayout> &disks_layout) const {
    return std::vector<PhysicalLocation>();
}

uint8_t Raid0::get_type() const { return 0 }