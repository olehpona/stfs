#pragma once
#include <cstdint>
#include <iostream>
#include <functional>
#include <vector>
#include <stfs/const.h>
#include <stdexcept>
#include <stfs/storage_cluster.h>
#include <stfs/search_engine.h>
class IndexError : public std::runtime_error
{
public:
    IndexError(const std::string &msg) : std::runtime_error(msg) {}
};

#define INDEX_ENTRY_SIZE 12

struct IndexEntry 
{ 
    uint64_t timestamp = 0;
    uint32_t crc32;

    std::vector<char> serialize() const;
    size_t deserialize(const char* buffer);

    bool is_valid() const;
    void update_crc();
};

class Index // index entres stored on device and loaded when its needed
{
private:
    StorageCluster& storage_cluster_;
    uint64_t index_size_ = 0;

    IndexEntry read_entry(uint64_t id) const;
    void write_entry(uint64_t id, const IndexEntry &entry);

public:
    Index(StorageCluster& storage_cluster, uint64_t index_size);
    IndexEntry get_entry(uint64_t id) const;
    uint64_t find_entry_id_by_timestamp(uint64_t timestamp) const;
    void modify_entry(uint64_t id, const IndexEntry &new_entry);
};