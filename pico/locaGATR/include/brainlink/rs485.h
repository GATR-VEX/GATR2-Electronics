// rs485.h
// UART0 through the THVD1400, driving the bus only while replying. Arduino
// only.

#pragma once
#include <stddef.h>
#include <stdint.h>

#include "brainlink/wire.h"

namespace brainlink
{

void begin(uint32_t baud);

// Bytes that have arrived, up to capacity. Never waits.
size_t receive(uint8_t* out, size_t capacity);

// Waits the turnaround, drives the bus, sends the frame, releases the bus.
void send(const Frame& frame, uint32_t turnaround_us);

} // namespace brainlink
