// main.cpp
// Control board firmware. The steps are in firmware.h.

#include <Arduino.h>

#include "firmware.h"

void setup() { firmware::begin(); }

void loop() {
    const sensor_acquisition::SensorMap sensors = firmware::readSensors();
    const brainlink::Exchange           brain   = firmware::answerBrain(sensors);
    firmware::logStatus(sensors, brain); // optional
}
