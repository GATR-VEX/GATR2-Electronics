# Control board protocol

How the control board sends its readings to the Brain. Code:
[wire.h](../pico/locaGATR/include/brainlink/wire.h), compiled by both sides.

The board streams: every 2 ms it sends the latest reading of every sensor,
without being asked. The Brain only listens and keeps the newest frame.
Nothing is resent: a lost or corrupted frame is replaced by the next one.
Encoder counts are totals since boot, so a lost frame loses nothing.

## Link

| | |
|---|---|
| Wire | RS-485, one V5 Smart Port. Only the board transmits |
| Bytes | UART 8N1, 921600 baud (about 92 bytes per ms) |
| Byte order | Little endian |
| Board | Drives `EN` all the time; one READINGS frame every 2 ms |
| Brain | Never transmits. Each tick reads what arrived and keeps the newest frame |

## Frame

Every message:

```
0xA7 0x4C | type u8 | id u8 | length u8 | payload[length] | crc u16
```

| Field | Meaning |
|---|---|
| `0xA7 0x4C` | Sync. Marks the start of a frame in the byte stream |
| `type` | Which message, below |
| `id` | Sequence number, one up per frame, wraps after 255. A gap means lost frames |
| `length` | Payload bytes |
| `crc` | CRC-16/CCITT-FALSE over sync through payload. A frame that fails it is dropped |

At most 64 bytes of payload per frame.

## Messages

| Type | Name | Direction | Payload |
|---|---|---|---|
| `0x90` | READINGS | board to Brain | below |

### READINGS

```
boot_id u16 | record | record | ...
```

`boot_id` is random, new every boot. When it changes, the board restarted and
its counts and sequence started over.

Then one record per sensor:

```
sensor_type u8 | port u8 | health u8 | size u8 | age_us u32 | data[size]
```

| Field | Meaning |
|---|---|
| `sensor_type` | Kind of sensor, table below. Unknown types are skipped by `size` |
| `port` | Board connector: 0 = TW0, 1 = TW1, 2 = TW2 |
| `health` | Table below |
| `size` | Bytes of `data`. 0: no reading yet |
| `age_us` | Age of the reading when sent |
| `data` | The reading, laid out per sensor type |

## Sensor types

Never reused; 0 is not a type. Code:
[records/](../pico/locaGATR/include/records/).

| Type | Name | Data |
|---|---|---|
| `0x01` | Encoder | Count since boot, i32 ([encoder.h](../pico/locaGATR/include/records/encoder.h)) |

## Health

| Value | Name | Meaning |
|---|---|---|
| 0 | starting | No reading yet |
| 1 | ok | Current |
| 2 | stale | Nothing new lately |
| 3 | retrying | Lost, trying again |
| 4 | failed | Quick retries used up |

## Size and timing

The payload is capped at 64 bytes per frame, not per sensor. `length` is one
byte, so the format allows up to 255.

| Frame | Payload | Frame | Time at 921600 | Share of a 2 ms period |
|---|---|---|---|---|
| 3 encoders | 2 + 3 x 12 = 38 | 45 bytes | 0.49 ms | 24% |
| Full | 64 | 71 bytes | 0.77 ms | 39% |

Records that do not fit are left out and counted in the board's `left_out`.
More sensors need a larger cap or a longer period.
