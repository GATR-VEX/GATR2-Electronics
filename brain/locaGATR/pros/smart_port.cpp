// smart_port.cpp
// PROS only.

#include "locagatr/smart_port.h"

#include <algorithm>

#include "pros/error.h"
#include "pros/rtos.hpp"
#include "pros/serial.h"

namespace locagatr
{

std::optional<Io> openSmartPort(uint8_t port, int32_t baud) {
    const bool ok = port >= 1 && port <= 21 && pros::c::serial_enable(port) != PROS_ERR &&
                    pros::c::serial_set_baudrate(port, baud) != PROS_ERR &&
                    pros::c::serial_flush(port) != PROS_ERR;
    if (!ok) {
        return std::nullopt;
    }
    Io io;
    // PROS_ERR is INT32_MAX, not negative.
    io.read = [port](uint8_t* data, std::size_t capacity) -> std::size_t {
        const int32_t count = pros::c::serial_read(port, data, static_cast<int32_t>(capacity));
        if (count == PROS_ERR || count <= 0) {
            return 0;
        }
        return std::min(static_cast<std::size_t>(count), capacity);
    };
    io.now_us = [] { return pros::micros(); };
    return io;
}

} // namespace locagatr
