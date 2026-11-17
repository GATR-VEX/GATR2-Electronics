// wire.h
// Brain <-> control board frames on RS-485. The Brain asks, the board
// answers. Little endian. Portable; the Brain compiles this file too.
//
//   frame      0xA7 0x4C | type u8 | id u8 | length u8 | payload | crc u16
//   crc        CRC-16/CCITT-FALSE, sync bytes through payload
//
//   type 0x10  GET_READINGS  Brain -> board, no payload
//   type 0x90  READINGS      board -> Brain:
//                            boot_id u16, new every boot
//                            then one record per sensor:
//                              sensor_type u8 | port u8 | health u8 | size u8 | age_us u32 | data[size]
//
// size 0: no reading yet. data: per sensor type, records/. Unknown types are
// skipped by size. id: set by the Brain, echoed in the reply.

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace brainlink
{

constexpr uint8_t     kSync0         = 0xA7;
constexpr uint8_t     kSync1         = 0x4C;
constexpr std::size_t kHeaderSize    = 5; // sync, sync, type, id, length
constexpr std::size_t kCrcSize       = 2;
constexpr std::size_t kMaxPayload    = 64;
constexpr std::size_t kMaxFrame      = kHeaderSize + kMaxPayload + kCrcSize;
constexpr std::size_t kBootIdSize    = 2;
constexpr std::size_t kRecordHeader  = 8; // sensor_type, port, health, size, age_us
constexpr std::size_t kMaxRecordData = kMaxPayload - kBootIdSize - kRecordHeader;
constexpr std::size_t kMaxRecords    = (kMaxPayload - kBootIdSize) / kRecordHeader;

enum class Type : uint8_t {
    kGetReadings = 0x10,
    kReadings    = 0x90,
};

enum class Health : uint8_t {
    kStarting = 0, // begun, no reading yet (bring-up, calibration)
    kOk       = 1, // reading is current
    kStale    = 2, // up, but nothing new lately
    kRetrying = 3, // lost or failed, trying again
    kFailed   = 4, // quick retries used up; slow retries may continue
};

const char* healthName(Health health);

struct Frame {
    std::array<uint8_t, kMaxFrame> bytes{};
    std::size_t                    size = 0;
};

struct Record {
    uint8_t                             sensor_type   = 0;
    uint8_t                             port   = 0;
    Health                              health = Health::kStarting;
    uint32_t                            age_us = 0;
    std::array<uint8_t, kMaxRecordData> data{};
    std::size_t                         size = 0;
};

// A decoded READINGS payload.
struct Readings {
    uint16_t                        boot_id = 0;
    std::array<Record, kMaxRecords> records{};
    std::size_t                     count = 0;
};

uint16_t crc16(const uint8_t* data, std::size_t size);

Frame encodeGetReadings(uint8_t id);

// Builds a READINGS frame one record at a time.
class ReadingsWriter {
  public:
    explicit ReadingsWriter(uint16_t boot_id);

    // False, and the record left out, when it does not fit.
    bool add(uint8_t sensor_type, uint8_t port, Health health, uint32_t age_us, const uint8_t* data,
             std::size_t size);

    Frame frame(uint8_t id) const;

  private:
    std::array<uint8_t, kMaxPayload> payload_{};
    std::size_t                      size_ = 0;
};

// A checked frame: type, id and payload, as the scanner found it.
struct Message {
    Type                             type = Type::kGetReadings;
    uint8_t                          id   = 0;
    std::array<uint8_t, kMaxPayload> payload{};
    std::size_t                      length = 0;
};

bool isGetReadings(const Message& message);

// Nullopt for a wrong type or a payload that does not split into records.
std::optional<Readings> decodeReadings(const Message& message);

struct ScanStats {
    uint32_t messages = 0; // frames with a good checksum
    uint32_t skipped  = 0; // bytes that were not part of one
};

// Byte stream to messages. Resyncs on the sync bytes and the checksum, so it
// can start mid frame and recovers from corrupted or missing bytes.
class Scanner {
  public:
    // on(const Message&) for every complete message in these bytes, in order.
    template <typename OnMessage> void feed(const uint8_t* data, std::size_t size, OnMessage&& on) {
        for (std::size_t i = 0; i < size; ++i) {
            append(data[i]);
            while (extract()) {
                on(message_);
            }
        }
    }

    void reset() { used_ = 0; }

    const ScanStats& stats() const { return stats_; }

  private:
    void append(uint8_t byte);
    bool extract(); // one message from the front of the window, when complete
    void drop(std::size_t count);
    void consume(std::size_t count);

    std::array<uint8_t, kMaxFrame> window_{};
    std::size_t                    used_ = 0;
    Message                        message_;
    ScanStats                      stats_;
};

} // namespace brainlink
