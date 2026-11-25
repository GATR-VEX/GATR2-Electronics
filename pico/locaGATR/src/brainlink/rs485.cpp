// rs485.cpp

#include "brainlink/rs485.h"

#include <Arduino.h>

#include "ports.h"

namespace brainlink
{

void begin(uint32_t baud) {
    pinMode(ports::kBrain.en, OUTPUT);
    digitalWrite(ports::kBrain.en, HIGH); // drive the bus
    Serial1.setTX(ports::kBrain.tx);
    Serial1.setRX(ports::kBrain.rx);
    Serial1.begin(baud);
}

void send(const Frame& frame) { Serial1.write(frame.bytes.data(), frame.size); }

} // namespace brainlink
