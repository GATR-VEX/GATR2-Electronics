# Control board protocol

How the Brain and the control board talk. Code:
[wire.h](../pico/locaGATR/include/brainlink/wire.h), compiled by both sides.

The Brain asks, the board answers with the latest reading of every sensor.
Nothing is resent: a lost or corrupted reply times out, and the next request
gets newer data. Encoder counts are totals since boot, so a missed reply
loses nothing.

## Link

| | |
|---|---|
| Wire | RS-485, half duplex, one V5 Smart Port |
| Bytes | UART 8N1, 115200 baud (about 11.5 bytes per ms) |
| Byte order | Little endian |
| Board | Waits 200 us after a request, drives `EN`, sends, drops `EN` |
| Brain | Waits up to 7 ms for the reply, one request at a time |

## Frame

Every message:

```
0xA7 0x4C | type u8 | id u8 | length u8 | payload[length] | crc u16
```

| Field | Meaning |
|---|---|
| `0xA7 0x4C` | Sync. Marks the start of a frame in the byte stream |
| `type` | Which message, below |
| `id` | Request number. Set by the Brain, copied into the reply. A reply with an old id is dropped |
| `length` | Payload bytes |
| `crc` | CRC-16/CCITT-FALSE over sync through payload. A frame that fails it is dropped |

At most 64 bytes of payload per reply.

## Messages

| Type | Name | Direction | Payload |
|---|---|---|---|
| `0x10` | GET_READINGS | Brain to board | none |
| `0x90` | READINGS | board to Brain | below |

### READINGS

```
boot_id u16 | record | record | ...
```

`boot_id` is random, new every boot. When it changes, the board restarted and
its counts started over.

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

The payload is capped at 64 bytes, the whole reply, not per sensor. `length`
is one byte, so the format allows up to 255; 64 keeps the worst case reply
around 6 ms at 115200 baud.

| Reply | Payload | Frame | Time at 115200 |
|---|---|---|---|
| 3 encoders | 2 + 3 × 12 = 38 | 45 bytes | 3.9 ms |
| Full | 64 | 71 bytes | 6.2 ms |

Records that do not fit are left out and counted in the board's `left_out`.
More sensors need a higher baud on both ends, or a larger cap.
