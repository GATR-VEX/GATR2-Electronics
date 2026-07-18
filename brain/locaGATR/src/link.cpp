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

} // namespace

Link::Link(Io io, const Timing& timing) : io_(std::move(io)), timing_(timing) {}

// Bytes left from before this request: late replies and echoes.
void Link::drainInput() {
    std::array<uint8_t, kReadChunk> buffer{};
    while (true) {
        const std::size_t count = io_.read(buffer.data(), buffer.size());
        if (count == 0) {
            return;
        }
        scanner_.feed(buffer.data(), count, [this](const brainlink::Message& m) { onMessage(m); });
    }
}

Poll Link::transact() {
    drainInput();

    poll_.answered  = false;
    poll_.restarted = false;

    ++id_;
    const brainlink::Frame request = brainlink::encodeGetReadings(id_);
    ++stats_.requests;
    if (!io_.write(request.bytes.data(), request.size)) {
        ++stats_.write_failures;
        return poll_;
    }
    sent_us_ = io_.now_us();
    waiting_ = true;

    // Read before checking the deadline, so a reply that landed during the last
    // wait still counts.
    std::array<uint8_t, kReadChunk> buffer{};
    while (true) {
        const std::size_t count = io_.read(buffer.data(), buffer.size());
        if (count > 0) {
            scanner_.feed(buffer.data(), count,
                          [this](const brainlink::Message& m) { onMessage(m); });
        }
        if (poll_.answered || io_.now_us() - sent_us_ >= timing_.reply_timeout_us) {
            break;
        }
        if (count == 0) {
            io_.wait_us(timing_.read_wait_us);
        }
    }
    waiting_ = false;

    if (poll_.answered) {
        misses_ = 0;
    } else {
        ++stats_.timeouts;
        ++misses_;
        io_.wait_us(timing_.guard_us); // a late reply ends before the next request
    }
    poll_.fresh      = contact_ && misses_ < timing_.misses_until_stale;
    stats_.bad_bytes = scanner_.stats().skipped;
    return poll_;
}

void Link::onMessage(const brainlink::Message& message) {
    if (message.type != brainlink::Type::kReadings) {
        ++stats_.echoes;
        return;
    }
    const std::optional<brainlink::Readings> readings = brainlink::decodeReadings(message);
    if (!readings) {
        return;
    }
    if (!waiting_ || poll_.answered || message.id != id_) {
        ++stats_.late_replies;
        return;
    }
    poll_.answered    = true;
    poll_.restarted   = contact_ && readings->boot_id != boot_id_;
    poll_.readings    = *readings;
    poll_.received_us = io_.now_us();
    stats_.round_trip_us = static_cast<uint32_t>(poll_.received_us - sent_us_);
    stats_.board_restarts += poll_.restarted ? 1 : 0;
    ++stats_.replies;
    contact_ = true;
    boot_id_ = readings->boot_id;
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
    if (!poll.answered) {
        return std::nullopt;
    }
    const brainlink::Record* r = findRecord(poll.readings, records::encoder::kSensorType, port);
    if (r == nullptr) {
        return std::nullopt;
    }
    return records::encoder::read(r->data.data(), r->size);
}

} // namespace locagatr
