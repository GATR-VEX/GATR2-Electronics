# locaGATR board firmware

The control board's program: a sensor relay. It reads every sensor as fast as
the loop runs and answers the Brain with the latest readings over RS-485
whenever it asks. No localization here: the Brain turns the readings into a
pose.

```cpp
void loop() {
    const sensor_acquisition::SensorMap sensors = firmware::readSensors();      // step 1: every sensor into one map
    const brainlink::Exchange           brain   = firmware::answerBrain(sensors); // step 2: GET_READINGS answered from the map
    firmware::logStatus(sensors, brain);                                          // step 3: USB status line
}
```

Each step returns a plain struct and later steps take what they use. Step 3
is optional: nothing depends on it, so it can be commented out.

Status: builds with PlatformIO, 0 warnings; the sensor map, link and record
formats are host tested (21 tests). Not yet run on a control board. The Brain
end is [brain/locaGATR](../../brain/locaGATR/README.md).

## Build, flash, watch

From `pico/locaGATR`, Pico connected over its own USB:

```sh
pio run
pio run -t upload
pio device monitor
```

For a Pico 2 on the same board: `pio run -e control_board_v1_pico2`. It
builds with no warnings; it has not been run on hardware.

The USB port is only ever a status output, once a second, one `|` part per
sensor:

```text
locagatr boot=3F2A loops=180000/s requests=100 ignored=0 bad_bytes=0 left_out=0 | TW0=1520 | TW1=-87 | TW2=0
```

## Choosing sensors

What is plugged in is [config.h](include/config.h): one object per sensor,
built from its own config struct, and the list of them.

```cpp
inline sensor_acquisition::QuadratureEncoderSensor tw0({ports::kTW0});
inline sensor_acquisition::QuadratureEncoderSensor tw1({ports::kTW1});
// inline sensor_acquisition::ImuSensor            imu({ports::kIMU1});

inline sensor_acquisition::Sensor* const kSensors[] = {&tw0, &tw1};
```

Add, remove or swap a sensor there. Nothing else changes; sensors not in the
list are never started or read.

A config struct always starts with the connector the sensor is on, from
[ports.h](include/ports.h). A connector is a struct of its pins, one struct
per kind of connector (`ports::Quadrature`, `ports::I2c`), so a sensor only
takes the kind it fits: an encoder on `ports::kIMU1` does not compile. The
connector's id goes into the sensor's records, so the Brain knows which wheel
a count came from.

## The sensor shape

Every sensor derives from `sensor_acquisition::Sensor`
([sensor_acquisition.h](include/sensor_acquisition/sensor_acquisition.h)).
`write` is required; every other hook has a default that does nothing, and a
sensor overrides only what it needs.

| Hook | When | Default | Overridden by |
|---|---|---|---|
| `write(Reading&)` | step 1, after `update` | required | every sensor: its newest reading in its record format and when it was sampled; false when it has none |
| `begin()` | once at boot | nothing | anything with pins or a bus to set up. Short; slow bring-up goes in `update` |
| `update(now_us)` | step 1, every loop | nothing | polling, bring-up and retry state machines. Never waits on the hardware |
| `health()` | step 1 | `kOk` | sensors that can be starting, stale, retrying or failed |
| `describe(text)` | status line | name and health | sensors with a useful number to show |

Health is one of `kStarting`, `kOk`, `kStale`, `kRetrying`, `kFailed`
([wire.h](include/brainlink/wire.h)) and goes to the Brain with every record,
so the Brain sees "IMU1 retrying" instead of a missing sensor.

`sensor_acquisition.h` is the whole core: that shape and step 1. Nothing in
it names a particular sensor. Every sensor implementation lives in
`sensor_acquisition/sensors/`, one file pair each, whatever kind of sensor it
is, named `<what>_sensor` with a class `<What>Sensor`.

The quadrature encoder
([quadrature_encoder_sensor.h](include/sensor_acquisition/sensors/quadrature_encoder_sensor.h))
overrides `begin` (pins, interrupts), `update` (copies the interrupt count),
`write`, `health` (failed when its connector has no interrupt slot) and
`describe`. It counts in its pin interrupts, so no edge is missed however long
the loop takes.

## The sensor map

Step 1 (`readAll`, in the same file) runs every sensor's `update`, then
collects its sensor type, connector, name, health and newest reading into one
`SensorMap`, in list order. Later steps take the map and grab
what they need; `map.find(sensor_type, port)` picks one entry. The responder
turns every entry into a record; nothing past step 1 talks to a sensor
directly.

## The link

Whenever bytes arrive from the Brain ([responder.h](include/brainlink/responder.h)):

| Request | Reply |
|---|---|
| GET_READINGS | READINGS: the boot id, then one record per map entry: sensor type, port, health, size, age in microseconds, data (empty when there is no reading) |
| anything else, including this board's own reply read back | ignored and counted |

Step 2 returns a `brainlink::Exchange`: how many requests it answered this
loop, the id of the last one, and the link totals since boot.

Frames are defined in [wire.h](include/brainlink/wire.h); each sensor type's
data format lives under [records/](include/records/), portable so the Brain
compiles the same files. A reader skips sensor types it does not know by their
size. A new boot id means the board restarted. Encoder counts add up from 0 at
boot and never reset; the Brain takes differences, so a missed reply loses
nothing.

The reply waits 200 us after the request's last byte, drives `EN` high, sends,
waits for the UART to finish, and drops `EN`. The transceiver's receiver is
disabled while `EN` is high, so the board never hears itself. At 115200 baud
the reply with three encoders is 45 bytes, 3.9 ms, close to the Brain's 5 ms
window: the next sensor needs a higher baud on both ends. The payload is
capped at 64 bytes; records past that are left out and counted (`left_out`).

## Sensor types

The `sensor_type` byte of a record says what sort of sensor it is and how to
read its data. Never reused; 0 is never a type. Code:
[records/sensor_types.h](include/records/sensor_types.h).

| Type | Name | Connector | Data | Format |
|---|---|---|---|---|
| `0x01` | Encoder | `ports::Quadrature` | count since boot, i32, 4 bytes. Never resets; take differences | [records/encoder.h](include/records/encoder.h) |

Health values in every record ([wire.h](include/brainlink/wire.h)):

| Health | Name | Meaning |
|---|---|---|
| 0 | starting | begun, no reading yet (bring-up, calibration) |
| 1 | ok | reading is current |
| 2 | stale | up, but nothing new lately |
| 3 | retrying | lost or failed, trying again |
| 4 | failed | quick retries used up; slow retries may continue |

## Adding a sensor

1. **Its record**, shared with the Brain: a sensor type in
   [records/sensor_types.h](include/records/sensor_types.h), a row in the
   [Sensor types](#sensor-types) table, and a `records/<name>` pair with the
   data layout, `write` and `read`. Copy
   [records/encoder.h](include/records/encoder.h).
2. **Its driver**: a pair in `sensor_acquisition/sensors/`, named for what it
   is and ending in `_sensor` (`imu_sensor.h`, class `ImuSensor`). One class
   deriving from `Sensor` with a nested `Config`, connector first, then its
   options:

   ```cpp
   class ImuSensor : public Sensor {
     public:
       enum class Range { k500dps, k1000dps, k2000dps };

       struct Config {
           ports::I2c port;
           Range      range = Range::k1000dps;
       };

       explicit ImuSensor(const Config& config)
           : Sensor(records::gyro::kSensorType, config.port.id, config.port.name),
             config_(config) {}

       bool write(Reading& out) const override; // required
       // plus any of: begin, update, health, describe

     private:
       Config config_;
   };
   ```

3. **A new kind of connector** only if the board has one: its struct and
   constants in [ports.h](include/ports.h).
4. **The object and its pointer** in [config.h](include/config.h).
5. **On the Brain**: a decoder for its sensor type using the same
   `records/<name>`.

Nothing else changes: not the loop, the map, the responder or the wire
format.

## Configure

[config.h](include/config.h): the sensors and their configs, baud,
turnaround, USB status.

[ports.h](include/ports.h): control board V1 connectors and their pins. TW0
on GP0/GP1, TW1 on GP2/GP3, TW2 on GP4/GP5; IMU1 on I2C1 GP6/GP7 with
interrupts on GP8/GP9 (not read yet); the Brain link on UART0 GP12/GP13 with
`EN` on GP11.

## Layout

Headers under `include/`, sources under `src/`, tests under `tests/`, the
same folder names in all three.

```
include/ports.h                connectors and their pins
include/config.h               the sensors, link and status settings
sensor_acquisition/
  sensor_acquisition           core, step 1: the Sensor shape, Reading, SensorMap, readAll
  sensors/quadrature_encoder_sensor
                               interrupt driven x4 decode on a quadrature connector
records/   sensor_types        sensor type numbers
           encoder             encoder record data: one count
brainlink/ wire                frames, READINGS records, Health
           bytes               little endian helpers
           responder           step 2: request in, READINGS reply out
           rs485               UART0 through the transceiver, driver enabled only while replying
firmware                       the steps: begin, read sensors, answer the Brain, status
src/main.cpp                   setup() and loop(): the steps in order, nothing else
```

Namespaces follow folders: `sensor_acquisition::`, `records::`,
`brainlink::`, plus `ports::`, `cfg::` and `firmware::` for the files at the
top. The `sensors/` folder only groups files and adds no level
(`sensor_acquisition::QuadratureEncoderSensor`). The one nesting is one
namespace per sensor type in `records::` (`records::encoder::write`), where
every type has its own `write` and `read`.

An implementation of something is named after what it is, then what it
implements: `quadrature_encoder_sensor.h` / `QuadratureEncoderSensor`, later
`imu_sensor.h` / `ImuSensor`. The suffix says what it plugs into before the
file is opened.

`records/`, `brainlink/wire`, `brainlink/responder` and
`sensor_acquisition/sensor_acquisition` are portable and host tested; the rest
touches hardware.

Host tests:

```sh
cmake -S pico/locaGATR/tests -B build-locagatr-board-tests
cmake --build build-locagatr-board-tests
ctest --test-dir build-locagatr-board-tests
```