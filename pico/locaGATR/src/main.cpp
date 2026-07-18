// main.cpp
// locaGATR board firmware: read every sensor as fast as the loop runs, answer
// the Brain with the latest readings when it asks. Everything is in firmware.h.

#include <Arduino.h>

#include "firmware.h"

void setup() { firmware::begin(); }

void loop() {
    const sensor_acquisition::SensorMap sensors = firmware::readSensors();
    const brainlink::Exchange           brain   = firmware::answerBrain(sensors);
    firmware::logStatus(sensors, brain); // optional debugging
}
