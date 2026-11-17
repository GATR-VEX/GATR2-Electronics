# locaGATR (Brain)

The Brain end of the control board link, over one V5 Smart Port. It asks the
board for its readings and decodes the reply. The Localizer in GATR2-Override
(`lib/locaGATR`) runs it; use it directly only to read the board without the
Localizer.

```cpp
std::optional<locagatr::Io> io = locagatr::openSmartPort(16, 115200);
locagatr::Link link(*io, locagatr::Timing{});

const locagatr::Poll poll = link.transact();          // one task, every tick
std::optional<int32_t> tw0 = locagatr::encoderCount(poll, 0);
```

## Link

`transact()` sends GET_READINGS and waits up to 7 ms for the reply with the
same id, one request at a time. After a timeout it waits 1 ms more so a late
reply can't overlap the next request.

| Event | Result |
|---|---|
| Reply | `answered`, records in `readings` |
| Three timeouts in a row | `fresh` false until the next reply |
| Late reply or own request read back | ignored, counted |
| New boot id | `restarted` true: the board's counts start over |
| Record with no reading | `encoderCount()` empty for that port |

`stats()` has the counts and the last round trip. Frames and records are the
firmware's own [wire.h](../../pico/locaGATR/include/brainlink/wire.h) and
[records/](../../pico/locaGATR/include/records/).

## Check on the bench

- Whether the V5 port switches RS-485 direction itself, and how fast it
  releases the bus. The board waits 200 us before answering.
- Whether the Brain reads back its own requests (handled either way).
- 115200 baud end to end. Three encoders take about 4.7 ms per round trip.
