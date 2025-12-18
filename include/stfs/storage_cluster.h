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
#include <stfs/ring_buffer.h>

#define CLUSTER_HEAD_SIZE 120
#define CLUSTER_STATE_SIZE 36

class ClusterError : public std::runtime_error
{
public:
    ClusterError(const std::string &msg) : std::runtime_error(msg) {};
};

struct ClusterHead // imutable fs meta block
{
    std::array<char, 5> magic = {'S', 'T', 'F', 'S', '\0'};           // "STFS"
    uint32_t version = 1;                                             // fs version
    uint8_t num_of_disks = 1;                                         // total number of disks in claster
    uint8_t raid_type = 0;                                            // claster raid type
    char mime[25] = "application/octet-stream";                       // mime type of block payload on claster

    uint64_t block_payload_size = 1;                                  // customizeble block size ( per claster )
    uint64_t total_blocks = 0;                                        // total blocks on claster
    uint64_t device_head_offset = CLUSTER_HEAD_SIZE;                // where device head is on device
    uint64_t cluster_state_offset = CLUSTER_HEAD_SIZE + DEVICE_HEAD_SIZE; // where cluster state is on device
    uint64_t index_offset = CLUSTER_HEAD_SIZE + DEVICE_HEAD_SIZE + CLUSTER_STATE_SIZE; // where index is on device
    uint64_t data_offset = 0;                                         // where data begins

    // reserved 32 bytes for future
    uint64_t reserve1 = 0;
    uint64_t reserve2 = 0;
    uint64_t reserve3 = 0;
    uint64_t reserve4 = 0;

    uint32_t crc32;

    std::vector<char> serialize() const;
    size_t deserialize(const char* buffer);

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

    std::vector<char> serialize() const;
    size_t deserialize(const char* buffer);

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

struct PhysicalAddress
{
    uint8_t disk_id;
    uint64_t offset;
};



struct ClusterStructsSizes
{
    uint64_t total_block_size;
    uint64_t index_entry_size;
    uint64_t transaction_size;
};

class StorageCluster
{
private:
    std::map<uint8_t, std::unique_ptr<Device>> devices_;
    std::unique_ptr<RaidGovernor> raid_governor_;
    uint64_t index_entry_size_ = 0;
    uint64_t total_block_size_ = 0;
    uint64_t transaction_size_ = 0;
    ClusterHead head_;
    ClusterState state_;

    std::unique_ptr<char[]> read_and_verify_mirrored_data(
        const std::vector<PhysicalAddress> &addresses,
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
    explicit StorageCluster(std::unique_ptr<RaidGovernor> governor, const ClusterStructsSizes &sizes);

    void format_cluster(const std::vector<DeviceFormatBlueprint> &blueprints, uint64_t block_payload_size);
    void open_cluster(const std::vector<DeviceOpenBlueprint> &blueprints);

    void write_next_block(const char *data);
    void write_index_block(uint64_t id, const char *data);
    void write_transaction_block(const char *data);

    RingBufferState get_ring_buffer_state() const;

    const ClusterState& get_state() const;
    const ClusterHead& get_head() const;
    void update_state(ClusterState state);

    std::unique_ptr<char[]> read_block(uint64_t id, DataValidator validator);
    std::unique_ptr<char[]> read_index_block(uint64_t id, DataValidator validator);
    std::unique_ptr<char[]> read_transaction_block(DataValidator validator);
};
