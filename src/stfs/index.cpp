#include <stfs/index.h>
#include <stfs/serelization.h>
#include <algorithm>

std::vector<char> IndexEntry::serialize() const {
    std::vector<char> buffer;
    buffer.resize(INDEX_ENTRY_SIZE);

    char *ptr = buffer.data();

    SERIALIZE_FIELD(ptr, timestamp, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, crc32, uint32_t, serializeU32);
    return buffer;
}

size_t IndexEntry::deserialize(const char* buffer) {
    const char* start = buffer;

    DESERIALIZE_FIELD(buffer, timestamp, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, crc32, uint32_t, deserializeU32);

    return buffer-start;
}

void IndexEntry::update_crc()
{
    this->crc32 = 0;
    auto serialized_data = serialize(); 
    this->crc32 = generate_CRC32(
        reinterpret_cast<const uint8_t *>(serialized_data.data()),
        serialized_data.size());
}

bool IndexEntry::is_valid() const
{
    IndexEntry self_copy = *this;
    self_copy.crc32 = 0;
    auto serialized_data = self_copy.serialize(); 
    uint32_t calculated_crc = generate_CRC32(
        reinterpret_cast<const uint8_t *>(serialized_data.data()),
        serialized_data.size());
    return calculated_crc == this->crc32;
}

void Index::write_entry(uint64_t id, const IndexEntry &entry)
{
    auto serialized_data = entry.serialize();
    storage_cluster_.write_index_block(id, reinterpret_cast<const char *>(serialized_data.data()));
}

IndexEntry Index::read_entry(uint64_t id) const
{
    DataValidator validator = [](const char *data, size_t size) -> bool
    {
        if (size != INDEX_ENTRY_SIZE)
        {
            return false;
        }

        IndexEntry candidate;
        candidate.deserialize(data);

        return candidate.is_valid();
    };

    IndexEntry entry;

    std::unique_ptr<char[]> buffer(storage_cluster_.read_index_block(id, validator));
    entry.deserialize(buffer.get());

    return entry;
}

Index::Index(StorageCluster &storage_cluster, uint64_t index_size) : storage_cluster_(storage_cluster), index_size_(index_size) {}

IndexEntry Index::get_entry(uint64_t id) const
{
    if (id >= index_size_)
    {
        throw IndexError("Id " + std::to_string(id) + " out of bound.");
    }
    return read_entry(id);
}

uint64_t Index::find_entry_id_by_timestamp(uint64_t timestamp) const {
    auto buffer_state = storage_cluster_.get_ring_buffer_state();

    TimeStampFetcher fetcher = [this](uint64_t id) -> uint64_t {
        return get_entry(id).timestamp;
    };

    return SearchEngine::find_block_id_by_timestamp(timestamp,fetcher, buffer_state);
}

void Index::modify_entry(uint64_t id, const IndexEntry &new_entry)
{
    if (id >= index_size_)
    {
        throw IndexError("Id " + std::to_string(id) + " out of bound.");
    }
    IndexEntry mutable_entry = new_entry;
    mutable_entry.update_crc();
    write_entry(id, mutable_entry);
}
