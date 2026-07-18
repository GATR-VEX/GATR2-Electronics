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
    io.write = [port](const uint8_t* data, std::size_t size) {
        const int32_t room = pros::c::serial_get_write_free(port);
        if (room == PROS_ERR || room < static_cast<int32_t>(size)) {
            return false;
        }
        const int32_t written =
            pros::c::serial_write(port, const_cast<uint8_t*>(data), static_cast<int32_t>(size));
        return written == static_cast<int32_t>(size);
    };
    // PROS_ERR is INT32_MAX, not negative.
    io.read = [port](uint8_t* data, std::size_t capacity) -> std::size_t {
        const int32_t count = pros::c::serial_read(port, data, static_cast<int32_t>(capacity));
        if (count == PROS_ERR || count <= 0) {
            return 0;
        }
        return std::min(static_cast<std::size_t>(count), capacity);
    };
    io.now_us  = [] { return pros::micros(); };
    io.wait_us = [](uint32_t us) { pros::delay(std::max<uint32_t>(1, us / 1000)); };
    return io;
}

} // namespace locagatr
