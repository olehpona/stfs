#pragma once
#include <cstdint>
#include <stfs/endian_compat.h>

#define SERIALIZE_FIELD(buffer_ptr, value, type, serializer_func) \
    do { \
        type converted_value = serializer_func(value); \
        std::memcpy(buffer_ptr, &converted_value, sizeof(type)); \
        buffer_ptr += sizeof(type); \
    } while (0)

#define DESERIALIZE_FIELD(buffer_ptr, value_ref, type, deserializer_func) \
    do { \
        type raw_value; \
        std::memcpy(&raw_value, buffer_ptr, sizeof(type)); \
        value_ref = deserializer_func(raw_value); \
        buffer_ptr += sizeof(type); \
    } while (0)

inline uint16_t serializeU16(uint16_t data) {
    return htobe16(data);
}
inline uint32_t serializeU32(uint32_t data) {
    return htobe32(data);
}
inline uint64_t serializeU64(uint64_t data) {
    return htobe64(data);
}

inline uint16_t deserializeU16(uint16_t data) {
    return be16toh(data);
}
inline uint32_t deserializeU32(uint32_t data) {
    return be32toh(data);
}
inline uint64_t deserializeU64(uint64_t data) {
    return be64toh(data);
}