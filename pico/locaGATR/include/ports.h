// ports.h
// Control board V1 connectors and their RP2040 pins, one struct per kind.

#pragma once
#include <stdint.h>

namespace ports
{

// id: the connector, as sent in READINGS records. Unique on the board.

// Tracking wheel encoder: A/B quadrature.
struct Quadrature {
    uint8_t     id;
    const char* name;
    uint8_t     pin_a;
    uint8_t     pin_b;
};

// IMU connector: I2C plus two interrupt lines.
struct I2c {
    uint8_t     id;
    const char* name;
    uint8_t     sda;
    uint8_t     scl;
    uint8_t     int1;
    uint8_t     int2;
};

// RS-485 to the Brain: THVD1400 on UART0, DE and /RE together on EN.
struct Rs485 {
    uint8_t en; // high while this board drives the bus
    uint8_t tx; // UART0 TX -> D
    uint8_t rx; // UART0 RX <- R
};

constexpr Quadrature kTW0{0, "TW0", 0, 1};
constexpr Quadrature kTW1{1, "TW1", 2, 3};
constexpr Quadrature kTW2{2, "TW2", 4, 5};
constexpr I2c        kIMU1{3, "IMU1", 6, 7, 8, 9}; // I2C1; INT1 accel, INT3 gyro on IMU v2
constexpr Rs485      kBrain{11, 12, 13};
constexpr uint8_t    kLed = 25; // on the Pico itself, Pico and Pico 2

// Every quadrature connector, kQuadrature[i].id == i.
constexpr Quadrature kQuadrature[] = {kTW0, kTW1, kTW2};

} // namespace ports
