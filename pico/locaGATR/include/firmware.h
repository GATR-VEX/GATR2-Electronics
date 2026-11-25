// firmware.h
// The steps main.cpp calls, in order. Arduino only.

#pragma once

#include "brainlink/streamer.h"
#include "sensor_acquisition/sensor_acquisition.h"

namespace firmware
{

// Starts USB status, the LED, the RS-485 link and every sensor.
void begin();

// Step 1: every sensor updated; its health and newest reading in the map.
sensor_acquisition::SensorMap readSensors();

// Step 2: a READINGS frame to the Brain once per stream period.
const brainlink::Stats& streamToBrain(const sensor_acquisition::SensorMap& sensors);

// Step 3: a USB status line once a second. Optional.
void logStatus(const sensor_acquisition::SensorMap& sensors, const brainlink::Stats& link);

// Step 4: the LED blinks while the loop runs.
void blinkLed();

} // namespace firmware
