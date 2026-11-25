// streamer_gtest.cpp

#include "brainlink/streamer.h"

#include <gtest/gtest.h>

#include <vector>

namespace brainlink
{
namespace
{

using sensor_acquisition::Health;
using sensor_acquisition::SensorEntry;
using sensor_acquisition::SensorMap;

brainlink::Readings decoded(const brainlink::Frame& frame, uint8_t expected_id) {
    brainlink::Scanner                 scanner;
    std::optional<brainlink::Readings> readings;
    scanner.feed(frame.bytes.data(), frame.size, [&](const brainlink::Message& m) {
        EXPECT_EQ(m.id, expected_id);
        readings = brainlink::decodeReadings(m);
    });
    EXPECT_TRUE(readings);
    return readings ? *readings : brainlink::Readings{};
}

brainlink::Readings once(const SensorMap& map, uint64_t now_us, uint16_t boot_id = 1) {
    uint32_t left_out = 0;
    return decoded(readingsFrame(5, map, boot_id, now_us, left_out), 5);
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

TEST(Streamer, OneRecordPerEntryWithHealthAgeAndBootId) {
    SensorEntry starting;
    starting.sensor_type = 0x02;
    starting.port        = 3;
    starting.health      = Health::kRetrying; // no reading: still sent, with its health
    const SensorMap map =
        mapOf({entry(0x01, 0, Health::kOk, 1000, 4, 0xAA), starting,
               entry(0x01, 2, Health::kOk, 3000, 4, 0xBB)});

    const brainlink::Readings r = once(map, 3500, 0xBEEF);
    EXPECT_EQ(r.boot_id, 0xBEEF);
    ASSERT_EQ(r.count, 3u);

    EXPECT_EQ(r.records[0].sensor_type, 0x01);
    EXPECT_EQ(r.records[0].port, 0);
    EXPECT_EQ(r.records[0].health, Health::kOk);
    EXPECT_EQ(r.records[0].age_us, 2500u);
    ASSERT_EQ(r.records[0].size, 4u);
    EXPECT_EQ(r.records[0].data[3], 0xAA);

    EXPECT_EQ(r.records[1].sensor_type, 0x02);
    EXPECT_EQ(r.records[1].port, 3);
    EXPECT_EQ(r.records[1].health, Health::kRetrying);
    EXPECT_EQ(r.records[1].size, 0u);
    EXPECT_EQ(r.records[1].age_us, 0u);

    EXPECT_EQ(r.records[2].port, 2);
    EXPECT_EQ(r.records[2].age_us, 500u);
    EXPECT_EQ(r.records[2].data[0], 0xBB);
}

TEST(Streamer, OneFramePerPeriodWithASequenceId) {
    const SensorMap map = mapOf({entry(0x01, 0, Health::kOk, 0, 4, 1)});
    Streamer        s(2000);

    std::vector<uint8_t> ids;
    for (uint64_t t = 0; t < 10000; t += 100) { // the loop comes round every 100 us
        if (const std::optional<Frame> f = s.next(map, 1, t)) {
            ids.push_back(f->bytes[3]);
        }
    }
    EXPECT_EQ(ids, (std::vector<uint8_t>{0, 1, 2, 3, 4}));
    EXPECT_EQ(s.stats().frames, 5u);
}

TEST(Streamer, SequenceWrapsAfter255) {
    Streamer s(1);
    uint8_t  last = 0;
    for (uint64_t t = 0; t < 257; ++t) {
        last = s.next(SensorMap{}, 1, t)->bytes[3];
    }
    EXPECT_EQ(last, 0);
}

TEST(Streamer, ALateLoopDoesNotSendABurst) {
    Streamer s(2000);
    ASSERT_TRUE(s.next(SensorMap{}, 1, 0));
    ASSERT_TRUE(s.next(SensorMap{}, 1, 50000)); // stalled for 50 ms
    EXPECT_FALSE(s.next(SensorMap{}, 1, 50100));
    EXPECT_TRUE(s.next(SensorMap{}, 1, 52000));
}

TEST(Streamer, FramesFollowTheMap) {
    EXPECT_EQ(once(mapOf({entry(0x01, 0, Health::kOk, 0, 1, 1)}), 0).records[0].data[0], 1);
    EXPECT_EQ(once(mapOf({entry(0x01, 0, Health::kOk, 0, 1, 2)}), 0).records[0].data[0], 2);
}

TEST(Streamer, AgeIsZeroForASampleNotOlderThanNowAndSaturates) {
    const SensorMap map = mapOf({entry(0x01, 0, Health::kOk, 5000, 0, 0)});
    EXPECT_EQ(once(map, 4000).records[0].age_us, 0u);
    EXPECT_EQ(once(map, 5000).records[0].age_us, 0u);
    EXPECT_EQ(once(map, 5000 + (1ull << 33)).records[0].age_us, 0xFFFFFFFFu);
}

TEST(Streamer, RecordsThatDoNotFitAreLeftOutAndCounted) {
    const SensorMap map = mapOf({entry(0x01, 0, Health::kOk, 0, brainlink::kMaxRecordData, 1),
                                 entry(0x02, 1, Health::kOk, 0, 4, 2)});
    Streamer        s(1000);
    const std::optional<Frame> f = s.next(map, 1, 0);
    ASSERT_TRUE(f);
    const brainlink::Readings r = decoded(*f, 0);
    ASSERT_EQ(r.count, 1u);
    EXPECT_EQ(r.records[0].sensor_type, 0x01);
    EXPECT_EQ(s.stats().left_out, 1u);
}

} // namespace
} // namespace brainlink
