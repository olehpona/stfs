#include <stfs/index.h>
#include <algorithm>

RawIndexEntry Index::convert_to_raw(const IndexEntry& memory_entry) const {
    RawIndexEntry raw_entry;

    raw_entry.timestamp = memory_entry.timestamp;
    raw_entry.device_count = memory_entry.device_count;

    size_t addresses_to_copy = std::min((size_t)MAX_DEVICES, memory_entry.local_ids.size());
    size_t addresses_bytes = addresses_to_copy * sizeof(uint64_t);
    std::memcpy(raw_entry.local_ids, memory_entry.local_ids.data(), addresses_bytes);

    size_t devices_id_to_copy = std::min((size_t)MAX_DEVICES, memory_entry.devices_id.size());
    size_t devices_id_bytes = devices_id_to_copy * sizeof(uint8_t);
    std::memcpy(raw_entry.devices_id, memory_entry.devices_id.data(), devices_id_bytes);

    return raw_entry;
}

void Index::write_entry(uint64_t id, const IndexEntry& entry)
{
    const RawIndexEntry& raw_entry = convert_to_raw(entry);

    storage_cluster_.write_index_block(id, reinterpret_cast<const char*>(&raw_entry));
}

IndexEntry Index::convert_from_raw(const RawIndexEntry& raw_entry) const {
    IndexEntry memory_entry;
    memory_entry.timestamp = raw_entry.timestamp;
    memory_entry.device_count = raw_entry.device_count;

    size_t count = raw_entry.device_count;
    if (count > MAX_DEVICES) {
        count = MAX_DEVICES;
    }

    memory_entry.local_ids.assign(
        raw_entry.local_ids,
        raw_entry.local_ids + count
    );

    memory_entry.devices_id.assign(
        raw_entry.devices_id,
        raw_entry.devices_id + count
    );

    return memory_entry;
}

IndexEntry Index::read_entry(uint64_t id) const
{
    RawIndexEntry raw_entry;
    std::unique_ptr<char[]> buffer(storage_cluster_.read_index_block(id));
    std::memcpy(&raw_entry, buffer.get(), sizeof(RawIndexEntry));

    return convert_from_raw(raw_entry);
}

Index::Index(StorageCluster& storage_cluster, uint64_t index_size) : storage_cluster_(storage_cluster), index_size_(index_size){}


IndexEntry Index::get_entry(uint64_t id) const {
    if (id >= index_size_) {
        throw IndexError("Id " + std::to_string(id) + " out of bound.");
    }
    return read_entry(id);
}

void Index::modify_entry(uint64_t id, const IndexEntry& new_entry)
{
    if (id >= index_size_) {
        throw IndexError("Id " + std::to_string(id) + " out of bound.");
    }

    write_entry(id, new_entry);
}
