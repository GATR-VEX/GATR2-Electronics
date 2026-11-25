// rs485.h
// UART0 through the transceiver. The board only sends, so it drives the bus
// all the time. Arduino only.

#pragma once
#include <stddef.h>
#include <stdint.h>

#include "brainlink/wire.h"

namespace brainlink
{

void begin(uint32_t baud);

// Queues the frame. Waits only while the UART's buffer is full.
void send(const Frame& frame);

} // namespace brainlink
