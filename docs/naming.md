# Component names

| Name | What it is |
|---|---|
| `pico/locaGATR` | Control board firmware. Reads the sensors and answers the Brain with the latest readings over RS-485. |
| `brain/locaGATR` | Brain library for PROS. Asks the control board for its readings and decodes them. |
| `locaGATR` in GATR2-Override | The Localizer: the robot pose on the Brain, built on `brain/locaGATR`. |
| `naviGATR` in GATR2-Override | The Navigator: autonomous routes and path following, using the Localizer's pose. |
| `spectaGATR` | Live pose viewer: a Brain library plus a laptop program and browser view. Lives in GATR2-Override. |
| `pcb/ControlBoard` | The control board: Pico, encoder and IMU connectors, RS-485 to the Brain. |
| `pcb/IMU` | Optional IMU board on a cable to the control board; the alternative is a VEX Inertial Sensor on the Brain. |
| `pcb/MagneticEncoder` | Tracking wheel encoder boards. |

All three locaGATR parts are one system: the board reads the sensors, the
Brain library carries the readings over, and the Localizer turns them into a
pose.
