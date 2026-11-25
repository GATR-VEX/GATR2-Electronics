// config.h
// What is plugged into this board. Pins are in ports.h.

#pragma once
#include <stdint.h>

#include "ports.h"
#include "sensor_acquisition/sensors/quadrature_encoder_sensor.h"
#include "sensor_acquisition/sensor_acquisition.h"

namespace cfg
{

// Sensors plugged in, each with its config.
inline sensor_acquisition::QuadratureEncoderSensor tw0({ports::kTW0});
inline sensor_acquisition::QuadratureEncoderSensor tw1({ports::kTW1});
inline sensor_acquisition::QuadratureEncoderSensor tw2({ports::kTW2});

// Updated and reported in this order.
inline sensor_acquisition::Sensor* const kSensors[] = {&tw0, &tw1, &tw2};

// Link to the Brain: one READINGS frame every kStreamPeriodUs. Same baud on
// the Brain (gatr2::Robot::board_baud).
constexpr uint32_t kBaud           = 921600;
constexpr uint32_t kStreamPeriodUs = 2000; // 500 frames/s; 3 encoders take 0.5 ms each

// LED half period: on, then off.
constexpr uint32_t kLedHalfMs = 500;

// USB status line, once a second.
constexpr bool     kUsbDebug         = true;
constexpr uint32_t kUsbBaud          = 115200;
constexpr uint32_t kUsbDebugPeriodMs = 1000;

} // namespace cfg
