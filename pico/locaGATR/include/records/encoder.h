// encoder.h
// Tracking wheel encoder record: count since boot, i32. Portable.

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
