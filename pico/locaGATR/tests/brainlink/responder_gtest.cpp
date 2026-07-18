// responder_gtest.cpp

#include "brainlink/responder.h"

#include <gtest/gtest.h>

#include <vector>

namespace brainlink
{
namespace
{

using sensor_acquisition::Health;
using sensor_acquisition::SensorEntry;
using sensor_acquisition::SensorMap;

std::vector<brainlink::Frame> feed(Responder& r, const brainlink::Frame& frame, const SensorMap& map,
                              uint16_t boot_id, uint64_t now_us) {
    std::vector<brainlink::Frame> out;
    r.feed(frame.bytes.data(), frame.size, map, boot_id, now_us,
           [&](const brainlink::Frame& f) { out.push_back(f); });
    return out;
}

brainlink::Readings decoded(const brainlink::Frame& reply, uint8_t expected_id) {
    brainlink::Scanner                 scanner;
    std::optional<brainlink::Readings> readings;
    scanner.feed(reply.bytes.data(), reply.size, [&](const brainlink::Message& m) {
        EXPECT_EQ(m.id, expected_id);
        readings = brainlink::decodeReadings(m);
    });
    EXPECT_TRUE(readings);
    return readings ? *readings : brainlink::Readings{};
}

brainlink::Readings ask(Responder& r, const SensorMap& map, uint8_t id, uint64_t now_us,
                   uint16_t boot_id = 1) {
    const std::vector<brainlink::Frame> replies = feed(r, brainlink::encodeGetReadings(id), map, boot_id, now_us);
    EXPECT_EQ(replies.size(), 1u);
    return replies.empty() ? brainlink::Readings{} : decoded(replies[0], id);
}

SensorEntry entry(uint8_t sensor_type, uint8_t port, Health health, uint64_t sampled_us,
                  std::size_t size, uint8_t fill) {
    SensorEntry e;
    e.sensor_type        = sensor_type;
    e.port               = port;
    e.health             = health;
    e.has_reading        = true;
    e.reading.sampled_us = sampled_us;
    e.reading.size       = size;
    for (std::size_t i = 0; i < size; ++i) {
        e.reading.data[i] = fill;
    }
    return e;
}

SensorMap mapOf(std::initializer_list<SensorEntry> entries) {
    SensorMap map;
    for (const SensorEntry& e : entries) {
        map.entries[map.count++] = e;
    }
    return map;
}

TEST(Responder, OneRecordPerEntryWithHealthAgeAndBootId) {
    SensorEntry starting;
    starting.sensor_type   = 0x02;
    starting.port   = 3;
    starting.health = Health::kRetrying; // no reading: still sent, with its health
    const SensorMap map =
        mapOf({entry(0x01, 0, Health::kOk, 1000, 4, 0xAA), starting,
               entry(0x01, 2, Health::kOk, 3000, 4, 0xBB)});
    Responder r;

    const brainlink::Readings reply = ask(r, map, 42, 3500, 0xBEEF);
    EXPECT_EQ(reply.boot_id, 0xBEEF);
    ASSERT_EQ(reply.count, 3u);

    EXPECT_EQ(reply.records[0].sensor_type, 0x01);
    EXPECT_EQ(reply.records[0].port, 0);
    EXPECT_EQ(reply.records[0].health, Health::kOk);
    EXPECT_EQ(reply.records[0].age_us, 2500u);
    ASSERT_EQ(reply.records[0].size, 4u);
    EXPECT_EQ(reply.records[0].data[3], 0xAA);

    EXPECT_EQ(reply.records[1].sensor_type, 0x02);
    EXPECT_EQ(reply.records[1].port, 3);
    EXPECT_EQ(reply.records[1].health, Health::kRetrying);
    EXPECT_EQ(reply.records[1].size, 0u);
    EXPECT_EQ(reply.records[1].age_us, 0u);

    EXPECT_EQ(reply.records[2].port, 2);
    EXPECT_EQ(reply.records[2].age_us, 500u);
    EXPECT_EQ(reply.records[2].data[0], 0xBB);
    EXPECT_EQ(r.stats().requests, 1u);
}

TEST(Responder, ExchangeSaysWhatWasAnswered) {
    const SensorMap map = mapOf({entry(0x01, 0, Health::kOk, 0, 4, 1)});
    Responder       r;
    const auto      ignore = [](const brainlink::Frame&) {};

    const Exchange none = r.feed(nullptr, 0, map, 1, 0, ignore);
    EXPECT_EQ(none.answered, 0u);
    EXPECT_EQ(none.totals.requests, 0u);

    const brainlink::Frame a = brainlink::encodeGetReadings(7);
    const brainlink::Frame b = brainlink::encodeGetReadings(8);
    std::vector<uint8_t> two(a.bytes.begin(), a.bytes.begin() + a.size);
    two.insert(two.end(), b.bytes.begin(), b.bytes.begin() + b.size);
    const Exchange both = r.feed(two.data(), two.size(), map, 1, 0, ignore);
    EXPECT_EQ(both.answered, 2u);
    EXPECT_EQ(both.last_id, 8);
    EXPECT_EQ(both.totals.requests, 2u);

    const Exchange after = r.feed(nullptr, 0, map, 1, 0, ignore);
    EXPECT_EQ(after.answered, 0u);
    EXPECT_EQ(after.totals.requests, 2u); // totals carry over
}

TEST(Responder, RepliesFollowTheMap) {
    Responder r;
    EXPECT_EQ(ask(r, mapOf({entry(0x01, 0, Health::kOk, 0, 1, 1)}), 1, 0).records[0].data[0], 1);
    EXPECT_EQ(ask(r, mapOf({entry(0x01, 0, Health::kOk, 0, 1, 2)}), 2, 0).records[0].data[0], 2);
}

TEST(Responder, AgeIsZeroForASampleNotOlderThanNowAndSaturates) {
    const SensorMap map = mapOf({entry(0x01, 0, Health::kOk, 5000, 0, 0)});
    Responder       r;
    EXPECT_EQ(ask(r, map, 1, 4000).records[0].age_us, 0u);
    EXPECT_EQ(ask(r, map, 2, 5000).records[0].age_us, 0u);
    EXPECT_EQ(ask(r, map, 3, 5000 + (1ull << 33)).records[0].age_us, 0xFFFFFFFFu);
}

TEST(Responder, RecordsThatDoNotFitAreLeftOutAndCounted) {
    const SensorMap map = mapOf({entry(0x01, 0, Health::kOk, 0, brainlink::kMaxRecordData, 1),
                                 entry(0x02, 1, Health::kOk, 0, 4, 2)});
    Responder            r;
    const brainlink::Readings reply = ask(r, map, 1, 0);
    ASSERT_EQ(reply.count, 1u);
    EXPECT_EQ(reply.records[0].sensor_type, 0x01);
    EXPECT_EQ(r.stats().left_out, 1u);
}

TEST(Responder, OwnReplyReadBackAndGarbageAreIgnored) {
    const SensorMap map;
    Responder       r;
    EXPECT_TRUE(feed(r, brainlink::ReadingsWriter(0).frame(3), map, 0, 0).empty());
    EXPECT_EQ(r.stats().ignored, 1u);
    EXPECT_EQ(r.stats().requests, 0u);

    const uint8_t junk[] = {0x00, 0xA7, 0x11, 0xA7, 0x4C, 0x10, 0x09, 0x00, 0xFF, 0xFF};
    r.feed(junk, sizeof(junk), map, 0, 0, [](const brainlink::Frame&) { FAIL() << "junk answered"; });
    EXPECT_GT(r.stats().bad_bytes, 0u);

    // A request still works afterwards, even split into single bytes.
    const brainlink::Frame get      = brainlink::encodeGetReadings(9);
    std::size_t       answered = 0;
    for (std::size_t i = 0; i < get.size; ++i) {
        r.feed(&get.bytes[i], 1, map, 0, 10, [&](const brainlink::Frame& f) {
            ++answered;
            EXPECT_EQ(decoded(f, 9).count, 0u);
        });
    }
    EXPECT_EQ(answered, 1u);
}

} // namespace
} // namespace brainlink
