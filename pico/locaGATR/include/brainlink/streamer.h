// streamer.h
// The board end of the link: a READINGS frame every period, sent without
// waiting on the Brain. Portable.

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <optional>

#include "brainlink/wire.h"
#include "sensor_acquisition/sensor_acquisition.h"

namespace brainlink
{

// Totals since boot.
struct Stats {
    uint32_t frames   = 0;
    uint32_t left_out = 0; // records that did not fit in a frame
};

// The READINGS frame for the map: every entry as a record, aged as of now_us.
// Records that do not fit are left out and added to left_out.
Frame readingsFrame(uint8_t sequence, const sensor_acquisition::SensorMap& sensors,
                    uint16_t boot_id, uint64_t now_us, uint32_t& left_out);

class Streamer {
  public:
    explicit Streamer(uint32_t period_us) : period_us_(period_us) {}

    // The next frame once period_us has passed since the last, else nothing.
    // Its id counts up by one per frame, so the Brain can count lost ones.
    std::optional<Frame> next(const sensor_acquisition::SensorMap& sensors, uint16_t boot_id,
                              uint64_t now_us);

    const Stats& stats() const { return stats_; }

  private:
    uint32_t period_us_;
    uint64_t last_us_  = 0;
    bool     sent_     = false;
    uint8_t  sequence_ = 0;
    Stats    stats_;
};

} // namespace brainlink
