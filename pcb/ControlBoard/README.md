# Control board

[All PCBs](../README.md)

The control board is the robot's sensor board. It carries a Raspberry Pi Pico
with connectors for three tracking-wheel encoders and the
[IMU v2](../IMU/README.md) board, and answers the V5 Brain with the latest
readings over RS-485 through one Smart Port cable, which also powers it.

```
tracking wheels (3x A/B)  --+
IMU v2 (I2C, BMI088)      --+--> Pico on the control board --RS-485--> V5 Smart Port --> Brain (locaGATR)
Brain +12 V from the port --> 12 V to 5 V module --> Pico and 3.3 V regulator
```

The Brain side of the link is [brain/locaGATR](../../brain/locaGATR/README.md):
the board streams the latest sensor readings and the Brain only listens. The
board firmware is
[pico/locaGATR](../../pico/locaGATR/README.md).

## Iterations

| Revision | Status |
|---|---|
| [Control board V1](ControlBoard_V1/README.md) | Ordered, not yet built |

## Connected boards and cables

| Connector | Mates with | Cable |
|---|---|---|
| `IMU1`, JST PH 6-pin | `J1` on [IMU v2](../IMU/IMU_V2/README.md) | Straight 1:1 PH cable, pin 1 to pin 1 |
| `TW0`, `TW1`, `TW2`, JST PH 4-pin | Tracking-wheel encoders, 3.3 V A/B quadrature | 3.3 V, A, B, GND |
| `BuckConverter1`, JST PH 4-pin | External 12 V to 5 V module (TOBSUN 50 W type) | 12 V in, GND, GND, 5 V out |
| `RJ1`, 4P4C jack | One V5 Smart Port on the Brain | Standard VEX Smart Port cable |
