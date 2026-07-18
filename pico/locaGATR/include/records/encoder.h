// encoder.h
// A tracking wheel encoder's record data: its count since boot, i32. Counts
// never reset; take differences. The record's port is the connector (0 = TW0).
// Portable, shared with the Brain.

#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>

#include "records/sensor_types.h"

namespace records
{
namespace encoder
{

constexpr uint8_t     kSensorType = sensor_type::kEncoder;
constexpr std::size_t kSize = 4;

// kSize bytes into out.
void write(int32_t count, uint8_t* out);

// Nullopt for a wrong size.
std::optional<int32_t> read(const uint8_t* data, std::size_t size);

} // namespace encoder
} // namespace records
