// bytes.h
// Little endian integers in and out of byte buffers. Portable.

#pragma once
#include <stdint.h>

namespace brainlink
{

inline void putU16(uint8_t* out, uint16_t value) {
    out[0] = static_cast<uint8_t>(value);
    out[1] = static_cast<uint8_t>(value >> 8);
}

inline void putU32(uint8_t* out, uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        out[i] = static_cast<uint8_t>(value >> (8 * i));
    }
}

inline uint16_t getU16(const uint8_t* in) { return static_cast<uint16_t>(in[0] | (in[1] << 8)); }

inline uint32_t getU32(const uint8_t* in) {
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        value |= static_cast<uint32_t>(in[i]) << (8 * i);
    }
    return value;
}

} // namespace brainlink
