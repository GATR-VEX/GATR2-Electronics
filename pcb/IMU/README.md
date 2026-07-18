# IMU

[All PCBs](../README.md)

A small inertial sensor board, mounted where the robot's rotation is best
measured and connected to the [control board](../ControlBoard/README.md) by one
cable. Its first job is heading: the gyroscope's yaw rate, added up over time,
corrects the heading the tracking wheels give. Its second job is tilt: the
accelerometer shows when the robot is rocking or climbing, so the localization
can discount the tracking wheels at those moments. The board only supplies
measurements.

The board is separate from the control board so that its position on the robot
can be chosen for the measurement, not for the wiring.

It is optional. A robot can take its heading from this board, read by the
control board, or from a VEX Inertial Sensor on the Brain instead. The control
board firmware does not read this board yet.

## Iterations

| Revision | Sensor and link | Status |
|---|---|---|
| [IMU v1](IMU_V1/README.md) | ST ASM330LHHG1, SPI | Assembly failed: the custom footprint was mirrored. Kept as the record |
| [IMU v2](IMU_V2/README.md) | Bosch BMI088, I2C to the control board | Ordered 2026-10-28, not yet built. A new design, not a fix of v1 |

## Cable

IMU v2 and the control board use the same six-pin JST PH order at both ends:
3.3 V, SDA, SCL, gyroscope interrupt, accelerometer interrupt, GND. The cable
is a straight 1:1 lead.
