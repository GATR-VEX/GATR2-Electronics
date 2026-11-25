// wire.cpp

#include "brainlink/wire.h"

#include <cstring>

#include "brainlink/bytes.h"

namespace brainlink
{

namespace
{

Frame frame(Type type, uint8_t id, const uint8_t* payload, std::size_t length) {
    Frame f;
    f.bytes[0] = kSync0;
    f.bytes[1] = kSync1;
    f.bytes[2] = static_cast<uint8_t>(type);
    f.bytes[3] = id;
    f.bytes[4] = static_cast<uint8_t>(length);
    if (length > 0) {
        std::memcpy(&f.bytes[kHeaderSize], payload, length);
    }
    const std::size_t body = kHeaderSize + length;
    putU16(&f.bytes[body], crc16(f.bytes.data(), body));
    f.size = body + kCrcSize;
    return f;
}

bool knownType(uint8_t type) { return type == static_cast<uint8_t>(Type::kReadings); }

} // namespace

const char* healthName(Health health) {
    switch (health) {
    case Health::kStarting: return "starting";
    case Health::kOk: return "ok";
    case Health::kStale: return "stale";
    case Health::kRetrying: return "retrying";
    case Health::kFailed: return "failed";
    }
    return "unknown";
}

uint16_t crc16(const uint8_t* data, std::size_t size) {
    uint16_t crc = 0xFFFF;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= static_cast<uint16_t>(data[i] << 8);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                                 : static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

ReadingsWriter::ReadingsWriter(uint16_t boot_id) {
    putU16(payload_.data(), boot_id);
    size_ = kBootIdSize;
}

bool ReadingsWriter::add(uint8_t sensor_type, uint8_t port, Health health, uint32_t age_us,
                         const uint8_t* data, std::size_t size) {
    if (size > kMaxRecordData || size_ + kRecordHeader + size > kMaxPayload) {
        return false;
    }
    uint8_t* p = &payload_[size_];
    p[0]       = sensor_type;
    p[1]       = port;
    p[2]       = static_cast<uint8_t>(health);
    p[3]       = static_cast<uint8_t>(size);
    putU32(p + 4, age_us);
    if (size > 0) {
        std::memcpy(p + kRecordHeader, data, size);
    }
    size_ += kRecordHeader + size;
    return true;
}

Frame ReadingsWriter::frame(uint8_t id) const {
    return brainlink::frame(Type::kReadings, id, payload_.data(), size_);
}

std::optional<Readings> decodeReadings(const Message& message) {
    if (message.type != Type::kReadings || message.length < kBootIdSize) {
        return std::nullopt;
    }
    const uint8_t* p   = message.payload.data();
    const uint8_t* end = p + message.length;

    Readings readings;
    readings.boot_id = getU16(p);
    p += kBootIdSize;
    while (p < end) {
        if (static_cast<std::size_t>(end - p) < kRecordHeader || readings.count == kMaxRecords) {
            return std::nullopt;
        }
        Record& r   = readings.records[readings.count];
        r.sensor_type      = p[0];
        r.port      = p[1];
        r.health    = static_cast<Health>(p[2]);
        r.size      = p[3];
        r.age_us    = getU32(p + 4);
        p += kRecordHeader;
        if (static_cast<std::size_t>(end - p) < r.size) {
            return std::nullopt;
        }
        std::memcpy(r.data.data(), p, r.size);
        p += r.size;
        ++readings.count;
    }
    return readings;
}

void Scanner::consume(std::size_t count) {
    std::memmove(window_.data(), window_.data() + count, used_ - count);
    used_ -= count;
}

void Scanner::drop(std::size_t count) {
    consume(count);
    stats_.skipped += static_cast<uint32_t>(count);
}

// extract() keeps the window shorter than one frame, so it never overflows;
// the check only guards the invariant.
void Scanner::append(uint8_t byte) {
    if (used_ == window_.size()) {
        drop(1);
    }
    window_[used_++] = byte;
}

bool Scanner::extract() {
    while (true) {
        // Keep the window starting at a possible header.
        while (used_ > 0 && window_[0] != kSync0) {
            drop(1);
        }
        if (used_ >= 2 && window_[1] != kSync1) {
            drop(1);
            continue;
        }
        if (used_ >= 3 && !knownType(window_[2])) {
            drop(1);
            continue;
        }
        if (used_ >= kHeaderSize && window_[4] > kMaxPayload) {
            drop(1);
            continue;
        }
        if (used_ < kHeaderSize) {
            return false;
        }
        const std::size_t length = window_[4];
        const std::size_t total  = kHeaderSize + length + kCrcSize;
        if (used_ < total) {
            return false;
        }
        if (getU16(&window_[kHeaderSize + length]) == crc16(window_.data(), kHeaderSize + length)) {
            message_.type   = static_cast<Type>(window_[2]);
            message_.id     = window_[3];
            message_.length = length;
            std::memcpy(message_.payload.data(), &window_[kHeaderSize], length);
            consume(total); // a following frame may already be in the window
            ++stats_.messages;
            return true;
        }
        drop(1);
    }
}

} // namespace brainlink
