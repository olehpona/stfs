#pragma once
#include <cstdint>

uint32_t generate_CRC32(const uint8_t* data, uint64_t data_size);
bool validate_CRC32(const uint8_t* data, uint32_t crc32, uint64_t data_size);