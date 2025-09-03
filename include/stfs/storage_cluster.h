#pragma once
#include <iostream>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <stdexcept>
#include <functional>
#include <stfs/crypto.h>
#include <stfs/raid.h>
#include <stfs/device.h>

class ClusterError : public std::runtime_error
{
public:
    ClusterError(const std::string &msg) : std::runtime_error(msg) {};
};

struct ClusterHead // imutable fs meta block
{
    std::array<char, 4> magic = {'S', 'T', 'F', 'S'};                 // "STFS"
    uint32_t version = 1;                                             // fs version
    uint64_t block_payload_size = 1;                                  // customizeble block size ( per claster )
    uint64_t total_blocks = 0;                                        // total blocks on claster
    uint64_t device_head_offset = sizeof(ClusterHead);                // Max blocks on claster
    uint64_t index_offset = sizeof(ClusterHead) + sizeof(DeviceHead); // where index is on device
    char mime[25] = "application/octet-stream";                       // mime type of block payload on claster

    uint8_t num_of_disks = 1; // total number of disks in claster
    uint8_t raid_type = 0;    // claster raid type

    // reserved 32 bytes for future
    uint64_t reserve1 = 0;
    uint64_t reserve2 = 0;
    uint64_t reserve3 = 0;
    uint64_t reserve4 = 0;

    uint32_t crc32;

    bool is_valid() const;

    void update_crc();
};

struct ClusterState
{
    uint64_t head_logical_block_id = 0;
    uint64_t tail_logical_block_id = 0;
    uint64_t valid_block_count = 0;
    uint64_t total_writes_count = 0;
    uint32_t crc32;

    bool is_valid() const;

    void update_crc();
};

using DeviceFormatter = std::function<std::unique_ptr<Device>(const std::string &, uint64_t device_head_offset, uint8_t device_id)>;

using DeviceOpener = std::function<std::unique_ptr<Device>(const std::string &, uint64_t device_head_offset)>;

using DataValidator = std::function<bool(const char *data, size_t size)>;

struct DeviceFormatBlueprint
{
    std::string path;
    DeviceFormatter formater;
};

struct DeviceOpenBlueprint
{
    std::string path;
    DeviceOpener opener;
};

struct BlockPlacement
{
    std::vector<uint8_t> devices;
    std::vector<uint64_t> ids;
};

struct ClusterConfig
{
    uint64_t block_payload_size;
    uint64_t total_block_size;
    uint64_t index_entry_size;
};

class StorageCluster
{
private:
    std::map<uint8_t, std::unique_ptr<Device>> devices_;
    std::unique_ptr<RaidGovernor> raid_governor_;
    uint64_t index_entry_size_ = 0;
    uint64_t total_block_size_ = 0;
    ClusterHead head_;
    ClusterState state_;

    std::unique_ptr<char[]> read_and_verify_mirrored_data(
        const std::vector<uint8_t> &device_ids,
        uint64_t offset,
        size_t size,
        const DataValidator &is_valid);

    void read_and_verify_heads();
    void read_and_verify_states();

    void write_head_to_all_devices();
    void write_state_to_all_devices();

    void mirrored_write(size_t address, const char *data, size_t size);
    void write(uint8_t device_id, size_t address, const char *data, size_t size);
    std::unique_ptr<char[]> read(uint8_t device_id, size_t address, size_t size);

public:
    explicit StorageCluster(std::unique_ptr<RaidGovernor> governor);

    void format_cluster(const std::vector<DeviceFormatBlueprint> &blueprints, const ClusterConfig &config);
    void open_cluster(const std::vector<DeviceOpenBlueprint> &blueprints);

    BlockPlacement write_next_block(const char *data);
    void write_index_block(uint64_t id, const char *data);

    uint64_t get_total_blocks_count() const;
    std::unique_ptr<char[]> read_block(uint64_t id, DataValidator validator);
    std::unique_ptr<char[]> read_index_block(uint64_t id, DataValidator validator);
};
