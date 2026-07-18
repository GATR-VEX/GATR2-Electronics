# GATR2 Electronics

Sensor hardware for a VEX U robot: the circuit boards, the control board
firmware, and the Brain library that talks to the control board.

```
tracking wheels (TW0, TW1, TW2) --> control board --RS-485, one Smart Port--> V5 Brain
IMU board (optional) -------------^   (Pico)             VEX IMU (optional) --^
```

The control board reads every sensor as fast as it can and keeps the latest
readings. When the Brain asks, it answers with all of them in one reply. The
Brain turns the readings into a pose, with the Localizer in GATR2-Override.

Heading comes from an IMU, which can sit on either side depending on which one
the robot uses: the [IMU board](pcb/IMU/README.md) plugs into the control board
and is read there like any other sensor, or a VEX Inertial Sensor plugs into
the Brain and is read there. The control board firmware does not read the IMU
board yet.

Status: the firmware and the Brain library are host tested. Neither has run on
a control board yet.

## What is here

| Folder | What |
|---|---|
| [pico/locaGATR](pico/locaGATR/README.md) | Control board firmware: reads the sensors, answers the Brain over RS-485. Adding a sensor is a few files; the README walks through it |
| [brain/locaGATR](brain/locaGATR/README.md) | Brain library for PROS: asks the control board for its readings and decodes them |
| [pcb/ControlBoard](pcb/ControlBoard/README.md) | The control board: a Raspberry Pi Pico, three encoder connectors, an IMU connector and an RS-485 transceiver to one Smart Port |
| [pcb/IMU](pcb/IMU/README.md) | Optional IMU board on a cable to the control board |
| [pcb/MagneticEncoder](pcb/MagneticEncoder/README.md) | Compact tracking wheel encoders |
| [docs](docs/README.md) | Component names and the PCB contribution guide |

The live pose viewer, spectaGATR, lives with the robot code in GATR2-Override.

## Control board firmware

Needs [PlatformIO](https://platformio.org/). From `pico/locaGATR`, with the
Pico on USB:

```sh
pio run                 # build
pio run -t upload       # flash (hold BOOTSEL the first time)
pio device monitor      # status line once a second
```

The board runs a Raspberry Pi Pico. The firmware also builds for a Pico 2
(`pio run -e control_board_v1_pico2`); that has not been tried on hardware.

## Host tests

Needs CMake 3.16 or newer and a C++17 compiler. On Windows that is MSYS2
ucrt64 GCC, with `C:\msys64\ucrt64\bin` first on PATH, and `-G "MinGW
Makefiles"` added to the first command of each block.

```sh
cmake -S pico/locaGATR/tests -B build-locagatr-board-tests
cmake --build build-locagatr-board-tests
ctest --test-dir build-locagatr-board-tests

cmake -S brain -B build-brain
cmake --build build-brain --parallel
ctest --test-dir build-brain --output-on-failure
```

## Using the Brain library in a PROS project

Clone this repository next to the robot code and include
[brain/gatr2_brain.mk](brain/gatr2_brain.mk) from the PROS project's
`Makefile`; the file's header shows the lines. The [Brain README](brain/README.md)
has the details.

## Contributing

New boards and board revisions follow the
[PCB contribution guide](docs/contributing/README.md).
