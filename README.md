# GATR2 Electronics

Sensor hardware for a VEX U robot: circuit boards, control board firmware, and
the Brain library that talks to the control board.

```
tracking wheels (TW0, TW1, TW2) --> control board --RS-485, one Smart Port--> V5 Brain
IMU board (optional) -------------^   (Pico)             VEX IMU (optional) --^
```

The control board keeps the latest reading of every sensor and sends them all
when the Brain asks. The Brain turns them into a pose (locaGATR in
GATR2-Override). Heading comes from either the [IMU board](pcb/IMU/README.md)
on the control board (not read by the firmware yet) or a VEX Inertial Sensor
on the Brain.

Host tested; not run on a control board yet.

| Folder | What |
|---|---|
| [pico/locaGATR](pico/locaGATR/README.md) | Control board firmware |
| [brain/locaGATR](brain/locaGATR/README.md) | Brain library: the link to the control board |
| [pcb/ControlBoard](pcb/ControlBoard/README.md) | Control board: Pico, three encoder connectors, IMU connector, RS-485 to one Smart Port |
| [pcb/IMU](pcb/IMU/README.md) | Optional IMU board |
| [pcb/MagneticEncoder](pcb/MagneticEncoder/README.md) | Tracking wheel encoders |
| [docs](docs/README.md) | Component names, PCB contribution guide |

## Firmware

Needs [PlatformIO](https://platformio.org/). From `pico/locaGATR`:

```sh
pio run                 # build
pio run -t upload       # flash (hold BOOTSEL the first time)
pio device monitor      # status line
```

## Host tests

CMake 3.16+ and a C++17 compiler. Windows: MSYS2 ucrt64 with
`C:\msys64\ucrt64\bin` first on PATH, and `-G "MinGW Makefiles"` on each
first command.

```sh
cmake -S pico/locaGATR/tests -B build-locagatr-board-tests
cmake --build build-locagatr-board-tests
ctest --test-dir build-locagatr-board-tests

cmake -S brain -B build-brain
cmake --build build-brain --parallel
ctest --test-dir build-brain --output-on-failure
```

## Brain library in a PROS project

Clone this repo next to the robot code and include
[brain/gatr2_brain.mk](brain/gatr2_brain.mk) from the project `Makefile`; see
the [Brain README](brain/README.md).

## Contributing

Changes reach `main` only through a PR: threads resolved, `build` passing,
up to date, one code owner approval, merged by the owner. The rules are on
GitHub under Settings, Rules.
New boards and revisions: [PCB contribution guide](docs/contributing/README.md).
