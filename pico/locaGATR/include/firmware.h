// firmware.h
// The steps main.cpp calls, in order. Arduino only.

#pragma once

#include "brainlink/responder.h"
#include "sensor_acquisition/sensor_acquisition.h"

namespace firmware
{

// Starts USB status, the RS-485 link and every sensor.
void begin();

// Step 1: every sensor updated; its health and newest reading in the map.
sensor_acquisition::SensorMap readSensors();

// Step 2: each GET_READINGS answered from the map.
brainlink::Exchange answerBrain(const sensor_acquisition::SensorMap& sensors);

// Step 3: a USB status line once a second. Optional.
void logStatus(const sensor_acquisition::SensorMap& sensors, const brainlink::Exchange& brain);

} // namespace firmware
