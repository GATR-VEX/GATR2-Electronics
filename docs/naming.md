# Component names

| Name | What it is |
|---|---|
| `pico/locaGATR` | Control board firmware: reads the sensors, answers the Brain over RS-485 |
| `brain/locaGATR` | Brain library: the link to the control board |
| `locaGATR` in GATR2-Override | The Localizer: the robot pose on the Brain |
| `naviGATR` in GATR2-Override | The Navigator: autonomous routes |
| `spectaGATR` in GATR2-Override | Live pose viewer: Brain library, laptop program, browser view |
| `pcb/ControlBoard` | Control board: Pico, encoder and IMU connectors, RS-485 to the Brain |
| `pcb/IMU` | Optional IMU board; the alternative is a VEX Inertial Sensor |
| `pcb/MagneticEncoder` | Tracking wheel encoders |

The three locaGATR parts are one system: the board reads the sensors, the
Brain library carries the readings, the Localizer turns them into a pose.
