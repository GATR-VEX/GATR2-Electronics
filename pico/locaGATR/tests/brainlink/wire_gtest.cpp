// wire_gtest.cpp

#include "brainlink/wire.h"

#include <gtest/gtest.h>

#include <vector>

namespace brainlink
{
namespace
{

std::vector<Message> scan(const uint8_t* data, std::size_t size) {
    Scanner              scanner;
    std::vector<Message> out;
    scanner.feed(data, size, [&](const Message& m) { out.push_back(m); });
    return out;
}

std::optional<Readings> roundTrip(const Frame& f) {
    const std::vector<Message> got = scan(f.bytes.data(), f.size);
    EXPECT_EQ(got.size(), 1u);
    return got.empty() ? std::nullopt : decodeReadings(got[0]);
}

TEST(Wire, Crc16CcittFalseCheckValue) {
    const uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    EXPECT_EQ(crc16(check, sizeof(check)), 0x29B1);
}

TEST(Wire, RecordsRoundTripInOrder) {
    const uint8_t  a[] = {1, 2, 3, 4};
    const uint8_t  b[] = {0xFF};
    ReadingsWriter writer(0x3F2A);
    ASSERT_TRUE(writer.add(0x01, 0, Health::kOk, 250, a, sizeof(a)));
    ASSERT_TRUE(writer.add(0x7E, 2, Health::kStale, 0xDEADBEEF, b, sizeof(b)));
    ASSERT_TRUE(writer.add(0x02, 0xFF, Health::kFailed, 0, nullptr, 0));

    const std::optional<Readings> r = roundTrip(writer.frame(9));
    ASSERT_TRUE(r);
    EXPECT_EQ(r->boot_id, 0x3F2A);
    ASSERT_EQ(r->count, 3u);
    EXPECT_EQ(r->records[0].sensor_type, 0x01);
    EXPECT_EQ(r->records[0].port, 0);
    EXPECT_EQ(r->records[0].health, Health::kOk);
    EXPECT_EQ(r->records[0].age_us, 250u);
    ASSERT_EQ(r->records[0].size, sizeof(a));
    EXPECT_EQ(r->records[0].data[3], 4);
    EXPECT_EQ(r->records[1].sensor_type, 0x7E);
    EXPECT_EQ(r->records[1].port, 2);
    EXPECT_EQ(r->records[1].health, Health::kStale);
    EXPECT_EQ(r->records[1].age_us, 0xDEADBEEFu);
    ASSERT_EQ(r->records[1].size, 1u);
    EXPECT_EQ(r->records[1].data[0], 0xFF);
    EXPECT_EQ(r->records[2].sensor_type, 0x02);
    EXPECT_EQ(r->records[2].port, 0xFF);
    EXPECT_EQ(r->records[2].health, Health::kFailed);
    EXPECT_EQ(r->records[2].size, 0u);
}

TEST(Wire, NoRecordsIsStillAReply) {
    const std::optional<Readings> r = roundTrip(ReadingsWriter(7).frame(1));
    ASSERT_TRUE(r);
    EXPECT_EQ(r->boot_id, 7);
    EXPECT_EQ(r->count, 0u);
}

TEST(Wire, RecordThatDoesNotFitIsRefused) {
    const uint8_t  big[kMaxRecordData] = {};
    ReadingsWriter writer(1);
    EXPECT_FALSE(writer.add(1, 0, Health::kOk, 0, big, kMaxRecordData + 1));
    EXPECT_TRUE(writer.add(1, 0, Health::kOk, 0, big, kMaxRecordData));
    EXPECT_FALSE(writer.add(2, 0, Health::kOk, 0, nullptr, 0));

    const std::optional<Readings> r = roundTrip(writer.frame(1));
    ASSERT_TRUE(r);
    ASSERT_EQ(r->count, 1u);
    EXPECT_EQ(r->records[0].size, kMaxRecordData);
}

TEST(Wire, PayloadThatDoesNotSplitIntoRecordsIsRejected) {
    Message m;
    m.type   = Type::kReadings;
    m.length = 1; // shorter than boot_id
    EXPECT_FALSE(decodeReadings(m));

    m.length = kBootIdSize + kRecordHeader - 1; // cut inside a record header
    EXPECT_FALSE(decodeReadings(m));

    m.length     = kBootIdSize + kRecordHeader + 2;
    m.payload[5] = 3; // record says 3 data bytes, only 2 follow
    EXPECT_FALSE(decodeReadings(m));

    m.payload[5] = 2;
    EXPECT_TRUE(decodeReadings(m));
}

TEST(Wire, ScannerResyncsAfterJunkAndCorruption) {
    // Junk, an unknown type, then a frame changed after its crc was computed.
    std::vector<uint8_t> stream = {0x00, 0xA7, 0x11, 0xA7, 0x4C, 0x10, 0x01, 0x00, 0xFF, 0xFF};
    Frame                bad    = ReadingsWriter(7).frame(1);
    bad.bytes[3] ^= 0x01;
    stream.insert(stream.end(), bad.bytes.begin(), bad.bytes.begin() + bad.size);
    const Frame good = ReadingsWriter(7).frame(2);
    stream.insert(stream.end(), good.bytes.begin(), good.bytes.begin() + good.size);

    Scanner              scanner;
    std::vector<Message> got;
    for (const uint8_t byte : stream) {
        scanner.feed(&byte, 1, [&](const Message& m) { got.push_back(m); });
    }
    ASSERT_EQ(got.size(), 1u);
    EXPECT_EQ(got[0].id, 2);
    EXPECT_EQ(scanner.stats().messages, 1u);
    EXPECT_EQ(scanner.stats().skipped, stream.size() - good.size);
}

TEST(Wire, HealthNames) {
    EXPECT_STREQ(healthName(Health::kStarting), "starting");
    EXPECT_STREQ(healthName(Health::kOk), "ok");
    EXPECT_STREQ(healthName(Health::kFailed), "failed");
    EXPECT_STREQ(healthName(static_cast<Health>(99)), "unknown");
}

TEST(Wire, BackToBackFramesInOneRead) {
    const Frame a = ReadingsWriter(0).frame(5);
    const Frame b = ReadingsWriter(0).frame(6);

    std::vector<uint8_t> stream(a.bytes.begin(), a.bytes.begin() + a.size);
    stream.insert(stream.end(), b.bytes.begin(), b.bytes.begin() + b.size);
    const std::vector<Message> got = scan(stream.data(), stream.size());
    ASSERT_EQ(got.size(), 2u);
    EXPECT_EQ(got[0].id, 5);
    EXPECT_EQ(got[1].id, 6);
}

} // namespace
} // namespace brainlink
