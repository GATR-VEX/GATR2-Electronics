// main.cpp
// Control board firmware. The steps are in firmware.h.

#include <Arduino.h>

#include "firmware.h"

void setup() { firmware::begin(); }

void loop() {
    const sensor_acquisition::SensorMap sensors = firmware::readSensors();
    const brainlink::Stats&             link    = firmware::streamToBrain(sensors);
    firmware::logStatus(sensors, link); // optional
    firmware::blinkLed();
}
