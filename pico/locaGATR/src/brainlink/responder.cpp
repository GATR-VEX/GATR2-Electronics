// responder.cpp

#include "brainlink/responder.h"

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

Frame reply(uint8_t id, const sensor_acquisition::SensorMap& sensors, uint16_t boot_id,
                  uint64_t now_us, uint32_t& left_out) {
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
    return writer.frame(id);
}

} // namespace brainlink
