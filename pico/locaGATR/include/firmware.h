// firmware.h
// The program behind main.cpp: the board's state and its steps. main.cpp only
// calls these in order. Arduino only.

#pragma once

#include "brainlink/responder.h"
#include "sensor_acquisition/sensor_acquisition.h"

namespace firmware
{

// Starts USB status, the RS-485 link and every sensor.
void begin();

// Step 1: every sensor updated; its health and newest reading in the map.
sensor_acquisition::SensorMap readSensors();

// Step 2: bytes from the Brain, if any; each GET_READINGS answered from the
// map. What was asked and answered, with the link totals.
brainlink::Exchange answerBrain(const sensor_acquisition::SensorMap& sensors);

// Step 3: one status line to USB per second, when a monitor is attached.
// Nothing depends on it.
void logStatus(const sensor_acquisition::SensorMap& sensors, const brainlink::Exchange& brain);

} // namespace firmware
