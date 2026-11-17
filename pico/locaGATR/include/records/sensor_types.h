// sensor_types.h
// Sensor types in READINGS records, never reused; 0 is not a type. Same table
// as the README. Portable.

#pragma once
#include <stdint.h>

namespace records
{
namespace sensor_type
{

constexpr uint8_t kEncoder = 0x01;

} // namespace sensor_type
} // namespace records
