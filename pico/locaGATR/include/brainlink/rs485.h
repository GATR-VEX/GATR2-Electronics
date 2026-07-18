// rs485.h
// The Smart Port side of the link: UART0 through the THVD1400, with its
// driver enabled only while a reply goes out. Arduino only.

#pragma once
#include <stddef.h>
#include <stdint.h>

#include "brainlink/wire.h"

namespace brainlink
{

void begin(uint32_t baud);

// Bytes that have arrived, up to capacity. Never waits.
size_t receive(uint8_t* out, size_t capacity);

// Waits for the bus turnaround after the last byte received, drives the bus,
// sends the whole frame, releases the bus.
void send(const Frame& frame, uint32_t turnaround_us);

} // namespace brainlink
