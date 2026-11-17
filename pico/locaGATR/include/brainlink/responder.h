// responder.h
// The board end of the link: GET_READINGS in, READINGS out. Portable.

#pragma once
#include <stddef.h>
#include <stdint.h>

#include "brainlink/wire.h"
#include "sensor_acquisition/sensor_acquisition.h"

namespace brainlink
{

// Totals since boot.
struct Stats {
    uint32_t requests  = 0;
    uint32_t bad_bytes = 0; // not part of a good frame
    uint32_t ignored   = 0; // good frames that were not a request (an echo of our own reply)
    uint32_t left_out  = 0; // records that did not fit in a reply
};

// What one feed did with the Brain.
struct Exchange {
    uint32_t answered = 0; // requests answered in these bytes
    uint8_t  last_id  = 0; // id of the last request answered
    Stats    totals;
};

// The READINGS reply to request id: every entry as a record, aged as of
// now_us. Records that do not fit are left out and added to left_out.
Frame reply(uint8_t id, const sensor_acquisition::SensorMap& sensors, uint16_t boot_id,
                  uint64_t now_us, uint32_t& left_out);

class Responder {
  public:
    // Bytes from the Brain, arriving at now_us; none is fine. on(const
    // Frame&) once per request, in order; the caller sends the frame.
    template <typename OnReply>
    Exchange feed(const uint8_t* data, size_t size, const sensor_acquisition::SensorMap& sensors,
                  uint16_t boot_id, uint64_t now_us, OnReply&& on) {
        Exchange exchange;
        scanner_.feed(data, size, [&](const Message& message) {
            if (!isGetReadings(message)) {
                ++stats_.ignored;
                return;
            }
            ++stats_.requests;
            ++exchange.answered;
            exchange.last_id = message.id;
            on(reply(message.id, sensors, boot_id, now_us, stats_.left_out));
        });
        stats_.bad_bytes = scanner_.stats().skipped;
        exchange.totals  = stats_;
        return exchange;
    }

    const Stats& stats() const { return stats_; }

  private:
    Scanner scanner_;
    Stats         stats_;
};

} // namespace brainlink
