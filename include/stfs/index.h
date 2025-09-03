#pragma once
#include <cstdint>
#include <iostream>
#include <functional>
#include <vector>
#include <stfs/const.h>
#include <stdexcept>
#include <stfs/storage_cluster.h>

class IndexError : public std::runtime_error
{
public:
    IndexError(const std::string &msg) : std::runtime_error(msg) {}
};

struct RawIndexEntry // static sized index entry for storing on device
{ 
    uint64_t timestamp = 0;
    uint8_t device_count = 1;
    uint8_t _padding[7]; // What a waste of 7 bytes for the sake of functionality.
    uint64_t local_ids[MAX_DEVICES];
    uint8_t devices_id[MAX_DEVICES];
    uint32_t crc32;

    bool is_valid() const;

    void update_crc();
};

struct IndexEntry // in memory index entry 
{
    uint64_t timestamp = 0;
    uint8_t device_count = 1;
    std::vector<uint64_t> local_ids;
    std::vector<uint8_t> devices_id;
};

class Index // index entres stored on device and loaded when its needed
{
private:
    StorageCluster& storage_cluster_;
    uint64_t index_size_ = 0;

    IndexEntry read_entry(uint64_t id) const;
    void write_entry(uint64_t id, const IndexEntry &entry);
    IndexEntry convert_from_raw(const RawIndexEntry &raw_entry) const;
    RawIndexEntry convert_to_raw(const IndexEntry &memory_entry) const;

public:
    Index(StorageCluster& storage_cluster, uint64_t index_size);
    IndexEntry get_entry(uint64_t id) const;
    void modify_entry(uint64_t id, const IndexEntry &new_entry);
};