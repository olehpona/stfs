#include <stfs/block.h>
#include <stfs/serelization.h>
#include <stfs/crypto.h>

std::vector<char> Block::serialize() const {
    std::vector<char> buffer(BLOCK_STATIC_SIZE + block_payload_size);
    char* ptr = buffer.data();

    SERIALIZE_FIELD(ptr, timestamp, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, block_payload_size, uint64_t, serializeU64);
    std::memcpy(ptr, payload.data(), payload.size());
    ptr += payload.size();
    SERIALIZE_FIELD(ptr, crc32, uint32_t, serializeU32);
    return buffer;
}

size_t Block::deserialize(const char* buffer) {
    const char* start = buffer;

    DESERIALIZE_FIELD(buffer, timestamp, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, block_payload_size, uint64_t, deserializeU64);

    payload.resize(block_payload_size);
    std::memcpy(payload.data(), buffer, block_payload_size);
    buffer += block_payload_size;
    DESERIALIZE_FIELD(buffer, crc32, uint32_t, deserializeU32);

    return buffer - start;
}

void Block::update_crc()
{
    this->crc32 = 0;
    auto serialized_data = serialize();
    this->crc32 = generate_CRC32(
        reinterpret_cast<const uint8_t *>(serialized_data.data()),
        serialized_data.size());
}

bool Block::is_valid() const
{
    Block self_copy = *this;
    self_copy.crc32 = 0;

    auto serialized_data = self_copy.serialize();

    uint32_t calculated_crc = generate_CRC32(
        reinterpret_cast<const uint8_t *>(serialized_data.data()),
        serialized_data.size());

    return calculated_crc == this->crc32;
}