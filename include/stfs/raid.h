#pragma once
#include <cstdint>
#include <vector>

struct DiskLayout
{
    uint8_t disk_id;
    uint64_t total_blocks;
};

struct PhysicalLocation {
    uint8_t disk_id;
    uint64_t block_id_on_disk;
};

class RaidGovernor
{
public:
    virtual std::vector<PhysicalLocation> map_logical_to_physical(uint64_t block_id, const std::vector<DiskLayout> &disks_layout) const = 0;
    virtual uint8_t get_type() const = 0;
    virtual ~RaidGovernor() = default;
};

class Raid0 : public RaidGovernor
{
public:
    std::vector<PhysicalLocation> map_logical_to_physical(uint64_t block_id, const std::vector<DiskLayout> &disks_layout) const override;
    uint8_t get_type() const override;
};