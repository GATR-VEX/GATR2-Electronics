// link.h
// The Brain end of the control board link: one GET_READINGS, then wait for
// its READINGS or a timeout, never a second request in flight. Owned by one
// task. Frames and records are the board firmware's own files
// (pico/locaGATR: brainlink/wire.h, records/). Portable: I/O and time come in
// through Io, so tests run it against a fake board.

#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

#include "brainlink/wire.h"

namespace locagatr
{

struct Timing {
    uint32_t reply_timeout_us   = 7000; // request sent to reply complete; 3 encoders at 115200 take ~4.7 ms
    uint32_t guard_us           = 1000; // after a timeout, before the next request
    uint32_t read_wait_us       = 1000; // between reads while waiting for a reply
    uint32_t misses_until_stale = 3;    // timeouts in a row
};

struct Io {
    std::function<bool(const uint8_t*, std::size_t)>  write; // all bytes queued, or false
    std::function<std::size_t(uint8_t*, std::size_t)> read;  // bytes that arrived, never waits
    std::function<uint64_t()>                         now_us;
    std::function<void(uint32_t)>                     wait_us;
};

struct LinkStats {
    uint32_t requests       = 0;
    uint32_t replies        = 0; // answers to the request just sent
    uint32_t timeouts       = 0;
    uint32_t write_failures = 0;
    uint32_t late_replies   = 0; // answers to an earlier, timed-out request
    uint32_t echoes         = 0; // own requests read back from the bus
    uint32_t bad_bytes      = 0; // not part of a good frame
    uint32_t board_restarts = 0; // new boot id
    uint32_t round_trip_us  = 0; // last answered request
};

// One transaction.
struct Poll {
    bool                answered    = false; // readings are this request's answer
    bool                fresh       = false; // fewer than misses_until_stale timeouts in a row
    bool                restarted   = false; // new boot id in this answer: counts start over
    brainlink::Readings readings;
    uint64_t            received_us = 0;     // Io::now_us when the answer arrived
};

class Link {
  public:
    Link(Io io, const Timing& timing);

    // Waits at most reply_timeout_us, plus guard_us after a timeout.
    Poll transact();

    const LinkStats& stats() const { return stats_; }

  private:
    void drainInput();
    void onMessage(const brainlink::Message& message);

    Io                 io_;
    Timing             timing_;
    LinkStats          stats_;
    brainlink::Scanner scanner_;
    Poll               poll_;
    uint8_t            id_      = 0;
    bool               waiting_ = false; // a request is in flight
    bool               contact_ = false; // the board has answered at least once
    uint16_t           boot_id_ = 0;
    uint64_t           sent_us_ = 0;
    uint32_t           misses_  = 0;
};

// The record for this sensor, or null.
const brainlink::Record* findRecord(const brainlink::Readings& readings, uint8_t sensor_type,
                                    uint8_t port);

// The encoder count on this board connector (0 = TW0), when the poll was
// answered and the record carries one.
std::optional<int32_t> encoderCount(const Poll& poll, uint8_t port);

} // namespace locagatr
