# IMU v2

[IMU overview](../README.md) · [All PCBs](../../README.md) · [Previous revision: v1](../IMU_V1/README.md)

**Status: ordered 2026-10-28 from [gerbers.zip](gerbers.zip).** Nothing has
been built or powered. The saved layout is the ordered board: re-plotted with
kicad-cli 10.0 it reproduces every layer of the zip, apart from the plotted
outline width of the back ground pour (0.25 mm in the zip, 0.15 mm now).

The schematic was reviewed on 2026-10-27 with KiCad 10 ERC, the netlist and
the Bosch BMI088 data sheet (BST-BMI088-DS000-19, revision 1.9). The layout
was redone after that review; [Layout](#layout) describes the ordered board.

v2 is not a corrected v1. v1 carried an ST ASM330LHHG1 on SPI with a mirrored
footprint; v2 is a new design around the Bosch BMI088 on I2C, laid out on the
standard KiCad footprint, and it plugs into the
[control board](../../ControlBoard/README.md). Only the purpose carried over: a
small board that can be mounted where the robot's rotation is best measured,
with one cable to the control board.

Project: [IMU.kicad_pro](IMU_v2/IMU.kicad_pro), one schematic sheet.

## Circuit

| Part | Role |
|---|---|
| BMI088 (`U1`), LGA-16 4.5 x 3.0 mm | 6-axis IMU: 16-bit gyroscope and accelerometer, two dies with separate I2C addresses |
| `R1`, `R2` 10 k | I2C pull-ups on SCL and SDA to 3.3 V |
| `C1` 1 uF, `C2`, `C3` 0.1 uF | Supply decoupling on VDD and VDDIO |
| `J1` JST PH 6-pin, horizontal | Cable to the control board |

The board is a single 3.3 V domain: VDD and VDDIO are tied to the 3.3 V pin
of `J1`, which the control board supplies from its own regulator.

### Interface configuration

The BMI088 is wired for I2C, following table 14 of the data sheet:

| Pin | Name | Connection | Why |
|---|---|---|---|
| 7 | PS | +3.3 V | VDDIO selects I2C (GND would select SPI) |
| 15 | SDO1 | GND | Accelerometer address 0x18 |
| 10 | SDO2 | GND | Gyroscope address 0x68 |
| 14 | CSB1 | +3.3 V | "VDDIO or DNC" in I2C mode |
| 5 | CSB2 | unconnected | "DNC (float)" in I2C mode |
| 16 | INT1 | `ACCEL_INT`, `J1` pin 5 | Accelerometer interrupt 1 |
| 12 | INT3 | `GYRO_INT`, `J1` pin 4 | Gyroscope interrupt 1 |
| 1, 13 | INT2, INT4 | unconnected | Unused interrupts must be left open |
| 2 | NC | GND | Data sheet connection for pin 2 |

The data sheet limits the bus to 400 kHz (fast mode) and 400 pF of load. With
10 k pull-ups and a short cable this is comfortable; 4.7 k would allow a longer
cable at the same speed if that ever becomes necessary.

### Interrupt pin defaults

The two interrupt outputs power up with different electrical behaviour, which
the firmware has to know:

| Pin | Default output | Default active level | Register |
|---|---|---|---|
| INT1 (accelerometer) | push-pull | active low | 0x53 `INT1_IO_CONF` |
| INT3 (gyroscope) | open-drain | active high | 0x16 `INT3_INT4_IO_CONF` |

Neither interrupt line has a pull-up or pull-down on this board or on the
control board. The gyroscope interrupt therefore does nothing until the
firmware either configures INT3 as push-pull (clear `Int3_od`) or enables a
pull-up on the Pico's GP9. Configuring push-pull is the simpler of the two.
Both lines are still useful without interrupts: the firmware can poll the
data-ready bits over I2C instead.

## Connector

`J1` pin 1 is the square pad, at the end of the row marked `3.3` on the
silkscreen.

| Pin | Signal | Control board `IMU1` pin |
|---|---|---|
| 1 | +3.3 V | 1 |
| 2 | SDA | 2 |
| 3 | SCL | 3 |
| 4 | GYRO_INT (INT3) | 4 |
| 5 | ACCEL_INT (INT1) | 5 |
| 6 | GND | 6 |

The pin order is identical at both ends, so the cable is a straight 1:1 JST PH
lead. A crimped or pre-made PH cable with the same wire colour on pin 1 at both
connectors is correct; a reversed lead would put 3.3 V on the ground pin.

## Layout

Two copper layers, 34 x 16 mm, ground pour on both sides. The front carries
the sensor, its decoupling and the connector; the back carries the two I2C
pull-ups, the pin labels and the team logo. The KiCad footprint
`Bosch_LGA-16_4.5x3mm_P0.5mm` is the data sheet landing pattern and includes a
copper keep-out under the body, which the data sheet asks for.

Two M3 holes, 23 mm apart along the long edge, take standoffs or the printed
housing that bolts to C-channel. They are placed on the board only, not in the
schematic, and carry a 6 mm pad added to the library footprint.

For the next revision:

- The silkscreen has no axis marking. The BMI088's measurement axes are fixed
  to the package; an arrow for +X and +Y on the top silkscreen, taken from
  figure 6 of the data sheet, lets the firmware's axis mapping be checked by
  eye when the board is mounted.

## ERC and DRC

- DRC on the ordered layout (2026-10-29, with schematic parity): no errors, no
  unconnected items. The two warnings and three parity warnings all concern the
  two mounting holes: modified from the library, and on the board only.
- ERC: two `power_pin_not_driven` errors, because 3.3 V arrives on a connector
  (a `PWR_FLAG` clears them), and one `pin_to_pin` error where the symbol's
  two output pins SDO1 and SDO2 are both tied to ground. That connection is
  exactly what the data sheet specifies for the default addresses; the error
  comes from the symbol's pin types, not the circuit.

## Firmware notes

- Accelerometer at 0x18, gyroscope at 0x68, 7-bit addresses.
- After power-up the accelerometer part is in suspend; it has to be switched
  on through `ACC_PWR_CTRL` (0x7D) and `ACC_PWR_CONF` (0x7C) before it
  produces data. The gyroscope part starts in normal mode.
- Keep at least 2 us between consecutive writes in normal mode and 450 us in
  suspend mode (data sheet table 13).
- For the heading estimate only the gyroscope Z axis and the accelerometer for
  tilt are needed; see the [IMU overview](../README.md).

## Files

- [KiCad project](IMU_v2/IMU.kicad_pro), [schematic](IMU_v2/IMU.kicad_sch), [layout](IMU_v2/IMU.kicad_pcb)
- [gerbers.zip](gerbers.zip): the fabrication set the boards were ordered from
- `IMU_v2/IMU348B.kicad_prl` is a stray per-user preference file from an
  earlier project name. Git ignores `.kicad_prl` files, so it is harmless; it
  can be deleted.
- The imported I3G4250DTR footprint from the earlier v2 attempt is no longer
  used and has been removed, together with the placeholder `IMU_v2.kicad_*`
  files. The project is now `IMU.kicad_pro` in the same folder.
