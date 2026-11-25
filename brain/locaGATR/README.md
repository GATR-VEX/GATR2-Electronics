# locaGATR (Brain)

The Brain end of the control board link, over one V5 Smart Port. The board
streams its readings; this reads them and keeps the newest. The Localizer in
GATR2-Override (`lib/locaGATR`) runs it; use it directly only to read the
board without the Localizer.

```cpp
std::optional<locagatr::Io> io = locagatr::openSmartPort(16, 921600);
locagatr::Link link(*io, locagatr::Timing{});

const locagatr::Poll poll = link.read();               // one task, every tick
std::optional<int32_t> tw0 = locagatr::encoderCount(poll, 0);
```

## Link

`read()` takes every frame that has arrived since the last call and keeps the
newest. It never writes and never waits.

| Event | Result |
|---|---|
| New frame | `updated`, records in `readings` |
| No good frame for 20 ms | `fresh` false until the next one |
| Gap in the sequence id | counted in `dropped` |
| Corrupted bytes | skipped, counted in `bad_bytes` |
| New boot id | `restarted` true: the board's counts start over |
| Record with no reading | `encoderCount()` empty for that port |

`stats()` has the counts. Format: [protocol](../../docs/protocol.md), code
shared with the firmware
([wire.h](../../pico/locaGATR/include/brainlink/wire.h),
[records/](../../pico/locaGATR/include/records/)).

## Check on the bench

- 921600 baud end to end, with the Brain receiving only.
- The Brain's serial receive buffer: at 500 frames/s about 225 bytes arrive
  per 10 ms tick. `dropped` climbing with a good cable means it overflows.
