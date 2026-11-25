// link_gtest.cpp
// Link against a fake stream in fake time: frames built the way the board
// firmware builds them, queued as bytes for the next read().

#include "locagatr/link.h"

#include <gtest/gtest.h>

#include <deque>
#include <initializer_list>
#include <vector>

#include "records/encoder.h"

namespace locagatr
{
namespace
{

struct FakeStream {
    std::deque<uint8_t> bytes;
    uint64_t            now_us = 0;

    Io io() {
        Io io;
        io.read = [this](uint8_t* out, std::size_t capacity) {
            std::size_t n = 0;
            while (n < capacity && !bytes.empty()) {
                out[n++] = bytes.front();
                bytes.pop_front();
            }
            return n;
        };
        io.now_us = [this] { return now_us; };
        return io;
    }

    // One frame: an encoder record per count, on connectors 0, 1, 2...
    void frame(uint8_t id, std::initializer_list<int32_t> counts, uint16_t boot_id = 0x3F2A) {
        brainlink::ReadingsWriter writer(boot_id);
        uint8_t                   port = 0;
        for (const int32_t c : counts) {
            uint8_t data[records::encoder::kSize];
            records::encoder::write(c, data);
            writer.add(records::encoder::kSensorType, port++, brainlink::Health::kOk, 0, data,
                       sizeof data);
        }
        const brainlink::Frame f = writer.frame(id);
        bytes.insert(bytes.end(), f.bytes.begin(), f.bytes.begin() + f.size);
    }
};

TEST(LocagatrLink, KeepsTheNewestFrameAndDecodesEncoderCounts) {
    FakeStream board;
    Link       link(board.io(), Timing{});
    board.frame(0, {10, -3});
    board.frame(1, {20, -4});

    const Poll p = link.read();
    EXPECT_TRUE(p.updated);
    EXPECT_TRUE(p.fresh);
    EXPECT_EQ(encoderCount(p, 0), 20);
    EXPECT_EQ(encoderCount(p, 1), -4);
    EXPECT_FALSE(encoderCount(p, 2));
    EXPECT_EQ(link.stats().frames, 2u);
    EXPECT_EQ(link.stats().dropped, 0u);
}

TEST(LocagatrLink, NothingNewStaysFreshUntilStale) {
    FakeStream board;
    Link       link(board.io(), Timing{});
    board.frame(0, {1});
    ASSERT_TRUE(link.read().updated);

    board.now_us += 10000;
    Poll p = link.read();
    EXPECT_FALSE(p.updated);
    EXPECT_FALSE(encoderCount(p, 0)); // counts only with a new frame
    EXPECT_TRUE(p.fresh);

    board.now_us += 15000;
    EXPECT_FALSE(link.read().fresh);

    board.frame(1, {2});
    p = link.read();
    EXPECT_TRUE(p.updated);
    EXPECT_TRUE(p.fresh);
}

TEST(LocagatrLink, DroppedFramesAreCountedFromTheSequence) {
    FakeStream board;
    Link       link(board.io(), Timing{});
    board.frame(0, {1});
    board.frame(1, {1});
    board.frame(4, {1});
    link.read();
    EXPECT_EQ(link.stats().dropped, 2u);
}

TEST(LocagatrLink, SequenceWrapIsNotADrop) {
    FakeStream board;
    Link       link(board.io(), Timing{});
    for (const uint8_t id : {254, 255, 0, 1}) {
        board.frame(id, {1});
    }
    link.read();
    EXPECT_EQ(link.stats().frames, 4u);
    EXPECT_EQ(link.stats().dropped, 0u);
}

TEST(LocagatrLink, BoardRestartIsFlaggedOnceAndStartsTheSequenceOver) {
    FakeStream board;
    Link       link(board.io(), Timing{});
    board.frame(40, {500}, 0x1111);
    EXPECT_FALSE(link.read().restarted);

    board.frame(0, {0}, 0x2222);
    const Poll p = link.read();
    EXPECT_TRUE(p.restarted);
    EXPECT_EQ(encoderCount(p, 0), 0);
    EXPECT_EQ(link.stats().board_restarts, 1u);
    EXPECT_EQ(link.stats().dropped, 0u);

    board.frame(1, {1}, 0x2222);
    EXPECT_FALSE(link.read().restarted);
}

TEST(LocagatrLink, FrameSplitAcrossReadsIsKept) {
    FakeStream board;
    Link       link(board.io(), Timing{});
    board.frame(0, {7});
    std::deque<uint8_t> rest(board.bytes.begin() + 10, board.bytes.end());
    board.bytes.erase(board.bytes.begin() + 10, board.bytes.end());

    EXPECT_FALSE(link.read().updated);
    board.bytes = rest;
    const Poll p = link.read();
    EXPECT_TRUE(p.updated);
    EXPECT_EQ(encoderCount(p, 0), 7);
}

TEST(LocagatrLink, CorruptFrameIsSkippedAndCounted) {
    FakeStream board;
    Link       link(board.io(), Timing{});
    board.frame(0, {7});
    board.bytes[8] ^= 0x40;
    const Poll p = link.read();
    EXPECT_FALSE(p.updated);
    EXPECT_GT(link.stats().bad_bytes, 0u);
}

TEST(LocagatrLink, RecordWithoutReadingGivesNoCount) {
    FakeStream                board;
    Link                      link(board.io(), Timing{});
    brainlink::ReadingsWriter writer(1);
    writer.add(records::encoder::kSensorType, 0, brainlink::Health::kStarting, 0, nullptr, 0);
    const brainlink::Frame f = writer.frame(0);
    board.bytes.insert(board.bytes.end(), f.bytes.begin(), f.bytes.begin() + f.size);
    const Poll p = link.read();
    EXPECT_TRUE(p.updated);
    EXPECT_FALSE(encoderCount(p, 0));
}

TEST(LocagatrLink, NoBoardIsNeverFresh) {
    FakeStream board;
    Link       link(board.io(), Timing{});
    EXPECT_FALSE(link.read().fresh);
    board.now_us += 1000000;
    EXPECT_FALSE(link.read().fresh);
}

} // namespace
} // namespace locagatr
