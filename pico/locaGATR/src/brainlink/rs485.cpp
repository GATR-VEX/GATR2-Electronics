// rs485.cpp

#include "brainlink/rs485.h"

#include <Arduino.h>
#include <hardware/uart.h>

#include "ports.h"

namespace brainlink
{
namespace
{

uint32_t g_last_rx_us = 0;

bool transmitterIdle() { return (uart_get_hw(uart0)->fr & UART_UARTFR_BUSY_BITS) == 0; }

} // namespace

void begin(uint32_t baud) {
    pinMode(ports::kBrain.en, OUTPUT);
    digitalWrite(ports::kBrain.en, LOW); // listen
    Serial1.setTX(ports::kBrain.tx);
    Serial1.setRX(ports::kBrain.rx);
    Serial1.begin(baud);
}

size_t receive(uint8_t* out, size_t capacity) {
    size_t n = 0;
    while (n < capacity && Serial1.available() > 0) {
        out[n++] = static_cast<uint8_t>(Serial1.read());
    }
    if (n > 0) {
        g_last_rx_us = micros();
    }
    return n;
}

void send(const Frame& frame, uint32_t turnaround_us) {
    while (static_cast<int32_t>(micros() - g_last_rx_us) < static_cast<int32_t>(turnaround_us)) {
    }
    digitalWrite(ports::kBrain.en, HIGH);
    Serial1.write(frame.bytes.data(), frame.size);
    Serial1.flush();
    while (!transmitterIdle()) {
    }
    digitalWrite(ports::kBrain.en, LOW);
}

} // namespace brainlink
