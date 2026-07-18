// config.h
// What is plugged into this board and the numbers to choose for it.
// Connectors and pins are in ports.h.

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

// Link to the Brain. After the Brain's last request byte the board waits
// kTurnaroundUs before driving the bus, so the Brain's port has released it.
constexpr uint32_t kBaud         = 115200;
constexpr uint32_t kTurnaroundUs = 200;

// Local USB text status, once a second. Never waits for a monitor.
constexpr bool     kUsbDebug         = true;
constexpr uint32_t kUsbBaud          = 115200;
constexpr uint32_t kUsbDebugPeriodMs = 1000;

} // namespace cfg
