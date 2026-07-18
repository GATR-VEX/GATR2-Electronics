# locaGATR (Brain)

The Brain end of the control board link over one V5 Smart Port. It asks the
board for its readings and decodes the answer. Turning the readings into a
pose is the `Localizer` in GATR2-Override (`lib/locaGATR`), which runs this
link in its own task. Use this library directly only to read the board
without the Localizer.

Host tested; not run on hardware.

## Use

```cpp
#include "locagatr/link.h"
#include "locagatr/smart_port.h"

std::optional<locagatr::Io> io = locagatr::openSmartPort(16, 115200);
locagatr::Link link(*io, locagatr::Timing{});

// One task, every 10 ms:
const locagatr::Poll poll = link.transact();
std::optional<int32_t> tw0 = locagatr::encoderCount(poll, 0); // TW0
```

In the project Makefile: `GATR2_BRAIN_LIBS := locagatr`, as described in
[gatr2_brain.mk](../gatr2_brain.mk).

## How the link works

The Brain is the only initiator; the board only answers.

1. `transact()` sends GET_READINGS with a new id.
2. It waits for the READINGS with the same id, up to 7 ms, and never sends a
   second request in between. After a timeout it waits another 1 ms so a late
   reply cannot overlap the next request.
3. It returns a `Poll`: whether this request was answered, the decoded
   records, whether the board restarted, and whether the link is fresh.

| Event | What happens |
|---|---|
| Normal | `answered`, the records in `readings` |
| Three timeouts in a row | `fresh` false until the next answer |
| Late reply (old id) | Ignored and counted |
| Own request read back (bus echo) | Ignored and counted |
| Board restart (new boot id) | `restarted` true for that answer: its counts start over from 0 |
| Record with no reading | `encoderCount()` gives nothing for that port |

`stats()` counts all of these, plus the last round trip.

Frames and record formats are the board firmware's own files,
[brainlink/wire.h](../../pico/locaGATR/include/brainlink/wire.h) and
[records/](../../pico/locaGATR/include/records/), compiled into both sides.

## Layout

- `include/locagatr/link.h`: `Link`, `Poll`, `encoderCount()`
- `include/locagatr/smart_port.h`: a Smart Port as the link's I/O
- `src/`: portable code, host tested
- `pros/smart_port.cpp`: PROS builds only
- `tests/link_gtest.cpp`: the link against a fake board that answers the way
  the firmware does

## Unverified, check on the bench first

- Whether the V5 port switches its own RS-485 direction, and how quickly it
  releases the bus after sending. The board waits 200 us before answering.
- Whether the Brain reads back its own requests (echo); it is handled either
  way.
- 115200 baud end to end. Three encoders take about 4.7 ms per round trip at
  that rate, close to the 10 ms tick. A faster baud on both ends leaves room
  for more sensors.
