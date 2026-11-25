// streamer.cpp

#include "brainlink/streamer.h"

namespace brainlink
{
namespace
{

uint32_t ageUs(uint64_t sampled_us, uint64_t now_us) {
    if (now_us <= sampled_us) {
        return 0;
    }
    const uint64_t age = now_us - sampled_us;
    return age > 0xFFFFFFFFull ? 0xFFFFFFFFu : static_cast<uint32_t>(age);
}

} // namespace

Frame readingsFrame(uint8_t sequence, const sensor_acquisition::SensorMap& sensors,
                    uint16_t boot_id, uint64_t now_us, uint32_t& left_out) {
    ReadingsWriter writer(boot_id);
    for (size_t i = 0; i < sensors.count; ++i) {
        const sensor_acquisition::SensorEntry& e = sensors.entries[i];
        const bool added =
            e.has_reading
                ? writer.add(e.sensor_type, e.port, e.health, ageUs(e.reading.sampled_us, now_us),
                             e.reading.data.data(), e.reading.size)
                : writer.add(e.sensor_type, e.port, e.health, 0, nullptr, 0);
        if (!added) {
            ++left_out;
        }
    }
    return writer.frame(sequence);
}

std::optional<Frame> Streamer::next(const sensor_acquisition::SensorMap& sensors,
                                    uint16_t boot_id, uint64_t now_us) {
    if (sent_ && now_us - last_us_ < period_us_) {
        return std::nullopt;
    }
    // Keep the period from the schedule, not from when the loop came round.
    last_us_ = sent_ && now_us - last_us_ < 2ull * period_us_ ? last_us_ + period_us_ : now_us;
    sent_    = true;
    ++stats_.frames;
    return readingsFrame(sequence_++, sensors, boot_id, now_us, stats_.left_out);
}

} // namespace brainlink
