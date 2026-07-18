# PCBs

Each board family has a README on its purpose and numbered revisions with
schematics, layouts, libraries and design and test notes.

| Board | Purpose |
|---|---|
| [Control board](ControlBoard/README.md) | Pico sensor board: encoders and IMU in, readings to the Brain over RS-485 |
| [IMU](IMU/README.md) | Optional inertial measurements for heading and tilt, on a cable to the control board |
| [Magnetic encoder](MagneticEncoder/README.md) | Compact tracking-wheel encoders |

Pick a revision and open its `.kicad_pro`. Keep project files and custom
libraries together. The magnetic encoder's symbol table holds a library path
from one workstation that may need setting on another machine.

New boards and revisions follow the
[PCB contribution guide](../docs/contributing/README.md). The control board's
firmware is in [pico/](../pico/README.md).
