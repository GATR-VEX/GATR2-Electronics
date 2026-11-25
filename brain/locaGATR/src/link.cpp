// link.cpp

#include "locagatr/link.h"

#include <array>
#include <utility>

#include "records/encoder.h"

namespace locagatr
{

namespace
{

constexpr std::size_t kReadChunk = 64;
constexpr int         kMaxChunks = 64; // 4 KB per read(), so a flood cannot hold it

} // namespace

Link::Link(Io io, const Timing& timing) : io_(std::move(io)), timing_(timing) {}

Poll Link::read() {
    poll_.updated   = false;
    poll_.restarted = false;

    std::array<uint8_t, kReadChunk> buffer{};
    for (int i = 0; i < kMaxChunks; ++i) {
        const std::size_t count = io_.read(buffer.data(), buffer.size());
        if (count == 0) {
            break;
        }
        scanner_.feed(buffer.data(), count, [this](const brainlink::Message& m) { onMessage(m); });
    }

    poll_.fresh      = contact_ && io_.now_us() - last_frame_us_ < timing_.stale_us;
    stats_.bad_bytes = scanner_.stats().skipped;
    return poll_;
}

void Link::onMessage(const brainlink::Message& message) {
    const std::optional<brainlink::Readings> readings = brainlink::decodeReadings(message);
    if (!readings) {
        return;
    }
    const bool restart = contact_ && readings->boot_id != boot_id_;
    if (contact_ && !restart) {
        stats_.dropped += static_cast<uint8_t>(message.id - next_id_);
    }
    if (restart) {
        ++stats_.board_restarts;
        poll_.restarted = true;
    }
    ++stats_.frames;
    contact_  = true;
    boot_id_  = readings->boot_id;
    next_id_  = static_cast<uint8_t>(message.id + 1);

    poll_.updated     = true;
    poll_.readings    = *readings;
    poll_.received_us = io_.now_us();
    last_frame_us_    = poll_.received_us;
}

const brainlink::Record* findRecord(const brainlink::Readings& readings, uint8_t sensor_type,
                                    uint8_t port) {
    for (std::size_t i = 0; i < readings.count; ++i) {
        const brainlink::Record& r = readings.records[i];
        if (r.sensor_type == sensor_type && r.port == port) {
            return &r;
        }
    }
    return nullptr;
}

std::optional<int32_t> encoderCount(const Poll& poll, uint8_t port) {
    if (!poll.updated) {
        return std::nullopt;
    }
    const brainlink::Record* r = findRecord(poll.readings, records::encoder::kSensorType, port);
    if (r == nullptr) {
        return std::nullopt;
    }
    return records::encoder::read(r->data.data(), r->size);
}

} // namespace locagatr
