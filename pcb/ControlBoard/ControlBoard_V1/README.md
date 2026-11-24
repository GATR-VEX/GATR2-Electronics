# Control board V1

[Control board overview](../README.md) · [All PCBs](../../README.md)

**Status: ordered, not yet built or powered.**

Project: [ControlBoard.kicad_pro](ControlBoard/ControlBoard.kicad_pro). Sheets:
root (Pico, power), `BrainLink` (RS-485), `IMU` (IMU connector), and three
`TrackingWheel` instances.

## What is on the board

| Part | Role |
|---|---|
| Raspberry Pi Pico (`A1`), socketed on headers | Counts the encoders, reads the IMU, answers the Brain |
| THVD1400DRLR (`U1`), 3.3 V RS-485 transceiver | Half-duplex link to the Smart Port; receive by default |
| AP2112K-3.3 (`U2`) | 3.3 V for encoders, IMU and transceiver, from the 5 V rail |
| 4P4C jack (`RJ1`, Cankemeng R-RJ11R04P-A802) | V5 Smart Port: 12 V in, RS-485 A and B |
| JST PH 4-pin `BuckConverter1` | Carries 12 V out to an external 12 V to 5 V module and 5 V back |
| JST PH 4-pin `TW0`, `TW1`, `TW2` | Tracking-wheel encoders |
| JST PH 6-pin `IMU1` | [IMU v2](../../IMU/IMU_V2/README.md) over I2C |

Two copper layers, 75.1 x 42.9 mm, ground pour on both sides, 0.6 mm vias with
0.3 mm drills. All passives are 0805 (1206 for the 22 uF parts), hand-solder
footprints.

## Power

```
Smart Port pin 1 (+12 V) --> C7 22 uF, C8 1 uF --> BuckConverter1 pins 1,2 (12 V, GND)
BuckConverter1 pins 3,4 (GND, 5 V) --> +5 V rail: C12 22 uF, C13 1 uF, C1 22 uF, C2 1 uF
+5 V --> Pico VSYS (pin 39)
+5 V --> U2 AP2112K-3.3 (EN tied to VIN, C20 1 uF in, C4 1 uF out) --> +3.3 V rail
```

The 3.3 V rail from `U2` feeds the encoders, the IMU and the transceiver. The
Pico's own 3V3 output (pin 36) is left unconnected, so the Pico's logic runs
from its internal regulator and everything else from `U2`. Both are nominally
3.3 V; the GPIO levels are compatible.

The Pico's VBUS, 3V3_EN and RUN pins are unconnected. The Pico stays
programmable over its USB connector; its on-board Schottky diode sits between
VBUS and VSYS, so USB and the 5 V rail can be present at the same time.

All capacitors are rated 50 V, including the two on the 12 V input.

## Pico pin assignment

| Pico pin | GPIO | Net | Function |
|---|---|---|---|
| 1, 2 | GP0, GP1 | `TW0_A`, `TW0_B` | Encoder 0 (A/B) |
| 4, 5 | GP2, GP3 | `TW1_A`, `TW1_B` | Encoder 1 |
| 6, 7 | GP4, GP5 | `TW2_A`, `TW2_B` | Encoder 2 |
| 9 | GP6 | `IMU_SDA` | I2C1 SDA |
| 10 | GP7 | `IMU_SCL` | I2C1 SCL |
| 11 | GP8 | `INT1_ACCEL` | BMI088 accelerometer interrupt |
| 12 | GP9 | `INT3_GYRO` | BMI088 gyroscope interrupt |
| 15 | GP11 | `EN` | RS-485 driver enable (DE and /RE), 10 k pulldown |
| 16 | GP12 | `RSTX` | UART0 TX to transceiver D |
| 17 | GP13 | `RSRX` | UART0 RX from transceiver R |
| 39 | VSYS | `+5V` | Supply |
| 3, 8, 13, 18, 23, 28, 38 | GND | `GND` | |

Free for later use: GP10, GP14 to GP22, GP26 to GP28 (ADC), RUN, VBUS, 3V3_EN.

## Connectors

Pin 1 is the square pad on every JST footprint.

| Connector | Pin 1 | Pin 2 | Pin 3 | Pin 4 | Pin 5 | Pin 6 |
|---|---|---|---|---|---|---|
| `TW0`, `TW1`, `TW2` (PH 4) | +3.3 V | A | B | GND | | |
| `IMU1` (PH 6) | +3.3 V | SDA | SCL | GYRO_INT | ACCEL_INT | GND |
| `BuckConverter1` (PH 4) | +12 V | GND | GND | +5 V | | |
| `RJ1` Smart Port (4P4C) | +12 V | GND | B | A | | |

`IMU1` matches `J1` on IMU v2 pin for pin, so the cable is a straight 1:1
JST PH lead. The I2C pull-ups live on the IMU board, not here.

Each encoder input has a 100 ohm series resistor at the connector and a 10 k
pulldown at the Pico, and each encoder connector has a 1 uF capacitor on its
3.3 V pin. The IMU connector has 1 uF and 0.1 uF on 3.3 V.

The Smart Port pin order was checked against a real cable by the designer. The
RS-485 direction (which of A and B the Brain treats as non-inverting) has not
been verified; if the first bench test shows garbage at the right baud, swap A
and B at the jack.

## RS-485 link

`U1` runs from 3.3 V with 1 uF decoupling. `DE` and `/RE` are tied together on
`EN` (GP11) with a 10 k pulldown, so the transceiver listens whenever the Pico
is not driving the pin, including during boot and when the firmware has not
started. The firmware raises `EN` only for the duration of a reply, as the
Brain's [locaGATR](../../../brain/locaGATR/README.md) protocol requires: the
Brain is the only initiator and the board only answers.

There is no termination or bias network on A and B. The THVD1400 has an
internal receiver fail-safe for open, shorted and idle buses, and the Smart
Port cable is short, so this is acceptable for a first build. A footprint for
an optional 120 ohm termination across A and B would cost nothing and may help
if the first bench test shows reflections at the baud rate finally chosen.

## Not yet verified on hardware

- Smart Port RS-485 polarity and the Brain's turnaround timing, as noted above.
- The 12 V to 5 V module's start-up behaviour on the Smart Port's current
  limit. The port supplies 12 V through the Brain's own protection; the module's
  inrush with 2 x 22 uF plus its own input capacitance has not been measured.
- Encoder signal integrity through the 100 ohm series resistors at the counting
  rate of the fastest wheel.

## For the next revision

- Pico at the board edge, USB connector overhanging, so the cable goes in
  straight without levering the Pico in its socket.
- A diode (Schottky, or an ideal diode such as the LM66100) between the buck
  output and VSYS. On V1, USB alone back-feeds the 5 V rail into the buck
  module, so the buck has to be unplugged when only USB is connected.
- LEDs on the 12 V and 3.3 V rails.
- A TVS on the Smart Port +12 V pin.

## Files

- [KiCad project](ControlBoard/ControlBoard.kicad_pro), [root schematic](ControlBoard/ControlBoard.kicad_sch),
  [RS-485 sheet](ControlBoard/RS-485.kicad_sch), [IMU sheet](ControlBoard/BMI088_Connector.kicad_sch),
  [tracking wheel sheet](ControlBoard/TrackingWheel.kicad_sch), [layout](ControlBoard/ControlBoard.kicad_pcb)
- Custom footprint: `VexPorts:VexPort` (the 4P4C jack) in
  [ControlBoard/VexPorts.pretty/](ControlBoard/VexPorts.pretty/), referenced
  from the project's `fp-lib-table`. [VEX-Port-Footprint/](VEX-Port-Footprint/)
  holds the easyeda2kicad import it was derived from, with the symbol and the
  STEP and WRL models; it is not in the library table, which is why ERC warns
  about a missing `easyeda2kicad` library on one symbol
