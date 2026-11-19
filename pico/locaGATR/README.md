# locaGATR board firmware

The control board program. It reads every sensor as fast as the loop runs and
answers the Brain with the latest readings over RS-485. The pose is worked
out on the Brain.

```cpp
void loop() {
    const sensor_acquisition::SensorMap sensors = firmware::readSensors();      // 1. every sensor into one map
    const brainlink::Exchange           brain   = firmware::answerBrain(sensors); // 2. GET_READINGS answered
    firmware::logStatus(sensors, brain);                                          // 3. USB status, optional
}
```

Brain end: [brain/locaGATR](../../brain/locaGATR/README.md).

## Build, flash, watch

From `pico/locaGATR`, Pico on USB:

```sh
pio run
pio run -t upload
pio device monitor
```

Pico 2: `pio run -e control_board_v1_pico2` (not run on hardware).

Status line, once a second:

```text
locagatr boot=3F2A loops=180000/s requests=100 ignored=0 bad_bytes=0 left_out=0 | TW0=1520 | TW1=-87 | TW2=0
```

## Sensors

What is plugged in is [config.h](include/config.h):

```cpp
inline sensor_acquisition::QuadratureEncoderSensor tw0({ports::kTW0});
inline sensor_acquisition::QuadratureEncoderSensor tw1({ports::kTW1});

inline sensor_acquisition::Sensor* const kSensors[] = {&tw0, &tw1};
```

Sensors not in the list are never started or read. Each config starts with
its connector from [ports.h](include/ports.h), typed by kind
(`ports::Quadrature`, `ports::I2c`), so an encoder on the IMU connector does
not compile.

Every sensor derives from `sensor_acquisition::Sensor`
([sensor_acquisition.h](include/sensor_acquisition/sensor_acquisition.h)) and
overrides only what it needs:

| Hook | When | Default |
|---|---|---|
| `write(Reading&)` | step 1 | required: the newest reading, false if none |
| `begin()` | once at boot | nothing. Keep it short |
| `update(now_us)` | every loop | nothing. Never waits on hardware |
| `health()` | step 1 | `kOk` |
| `describe(text)` | status line | name and health |

Implementations live in `sensor_acquisition/sensors/`, named
`<what>_sensor` / `<What>Sensor`. The quadrature encoder counts in its pin
interrupts, so no edge is missed.

## Link

| Brain sends | Board answers |
|---|---|
| GET_READINGS | READINGS: boot id, then one record per sensor |
| anything else | ignored and counted |

Frames, records, sensor types, health and timing:
[protocol](../../docs/protocol.md). Code: [wire.h](include/brainlink/wire.h)
and [records/](include/records/), compiled on the Brain too.

## Adding a sensor

1. Its record: a type in `records/sensor_types.h`, a row in the
   [protocol](../../docs/protocol.md) sensor types table, and a
   `records/<name>` pair with `write` and `read`. Copy `records/encoder.h`.
2. Its driver: `sensor_acquisition/sensors/<what>_sensor`, deriving from
   `Sensor`, with a nested `Config` that starts with its connector:

   ```cpp
   class ImuSensor : public Sensor {
     public:
       struct Config {
           ports::I2c port;
           Range      range = Range::k1000dps;
       };
       explicit ImuSensor(const Config& config);
       bool write(Reading& out) const override;
   };
   ```

3. A new connector kind, if needed, in `ports.h`.
4. The object and its pointer in `config.h`.
5. On the Brain: a `RecordDecoder` for the type in GATR2-Override
   `lib/locaGATR`.

## Pins

Control board V1 ([ports.h](include/ports.h)): TW0 GP0/GP1, TW1 GP2/GP3, TW2
GP4/GP5, IMU1 I2C1 GP6/GP7 with interrupts GP8/GP9 (not read yet), Brain
UART0 GP12/GP13 with `EN` on GP11.

## Layout

```
include/ports.h, config.h      connectors, what is plugged in
sensor_acquisition/            step 1: Sensor, SensorMap, readAll
  sensors/                     one file pair per sensor
records/                       record data per sensor type
brainlink/                     wire format, responder (step 2), RS-485
firmware                       the steps
src/main.cpp                   setup() and loop()
```

Namespaces follow folders (`sensor_acquisition::`, `records::`,
`brainlink::`); `sensors/` adds no level. `records/`, `brainlink/wire`,
`brainlink/responder` and `sensor_acquisition` are portable and host tested.

```sh
cmake -S pico/locaGATR/tests -B build-locagatr-board-tests
cmake --build build-locagatr-board-tests
ctest --test-dir build-locagatr-board-tests
```

## Status

Builds with no warnings, 21 host tests. Not run on a control board.
