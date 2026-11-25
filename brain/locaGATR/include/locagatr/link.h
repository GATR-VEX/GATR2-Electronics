// link.h
// The Brain end of the control board link. The board streams READINGS frames;
// read() takes every frame that has arrived and keeps the newest. It never
// writes and never waits. Uses the board firmware's brainlink/wire.h and
// records/. Portable: I/O and time through Io.

#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

#include "brainlink/wire.h"

namespace locagatr
{

struct Timing {
    uint32_t stale_us = 20000; // no good frame for this long: not fresh
};

struct Io {
    std::function<std::size_t(uint8_t*, std::size_t)> read; // bytes that arrived, never waits
    std::function<uint64_t()>                         now_us;
};

struct LinkStats {
    uint32_t frames         = 0; // good READINGS frames
    uint32_t dropped        = 0; // frames missing from the sequence
    uint32_t bad_bytes      = 0; // not part of a good frame
    uint32_t board_restarts = 0; // new boot id
};

// One read().
struct Poll {
    bool                updated     = false; // a new frame since the last read()
    bool                fresh       = false; // a good frame within stale_us
    bool                restarted   = false; // new boot id: counts start over
    brainlink::Readings readings;            // the newest frame
    uint64_t            received_us = 0;     // Io::now_us when it was read
};

class Link {
  public:
    Link(Io io, const Timing& timing);

    Poll read();

    const LinkStats& stats() const { return stats_; }

  private:
    void onMessage(const brainlink::Message& message);

    Io                 io_;
    Timing             timing_;
    LinkStats          stats_;
    brainlink::Scanner scanner_;
    Poll               poll_;
    bool               contact_       = false; // a good frame has arrived
    uint16_t           boot_id_       = 0;
    uint8_t            next_id_       = 0;     // sequence id expected next
    uint64_t           last_frame_us_ = 0;
};

// The record for this sensor, or null.
const brainlink::Record* findRecord(const brainlink::Readings& readings, uint8_t sensor_type,
                                    uint8_t port);

// The encoder count on this board connector (0 = TW0), when a new frame
// arrived and carries one.
std::optional<int32_t> encoderCount(const Poll& poll, uint8_t port);

} // namespace locagatr
