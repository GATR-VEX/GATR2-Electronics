// sensor_types.h
// Sensor types in READINGS records. One per type of sensor, never reused; 0
// is never a type. The same table is in the README ("Sensor types"); add a
// row there with every new type. Portable, shared with the Brain.

#pragma once
#include <stdint.h>

namespace records
{
namespace sensor_type
{

constexpr uint8_t kEncoder = 0x01;

} // namespace sensor_type
} // namespace records
