// link_gtest.cpp
// Link against a fake control board in fake time. The fake answers the way
// the board firmware does: only when asked, after a turnaround, echoing the
// request id, with one encoder record per connector.

#include "locagatr/link.h"

#include <gtest/gtest.h>

#include <deque>
#include <vector>

#include "records/encoder.h"

namespace locagatr
{
namespace
{

struct Arrival {
    uint64_t             at_us = 0;
    std::vector<uint8_t> bytes;
};

struct FakeBoard {
    // Bus and board timing.
    uint64_t now_us        = 0;
    uint32_t turnaround_us = 200;
    uint32_t byte_us       = 87; // 115200 baud

    // Board state.
    int32_t  counts[3] = {};
    uint16_t boot_id   = 0x1111;
    bool     tw2_empty = false; // TW2 record sent with no reading

    // Faults.
    int      drop_next      = 0;     // requests answered with nothing
    uint32_t extra_delay_us = 0;     // next reply only
    bool     corrupt_next   = false;
    bool     echo           = false; // the Brain reads back its own requests

    std::vector<brainlink::Message> requests;
    std::deque<Arrival>             incoming; // bytes on their way to the Brain
    brainlink::Scanner              scanner;

    void advance(uint32_t us) { now_us += us; }

    void restart() {
        boot_id = static_cast<uint16_t>(boot_id + 0x0101);
        for (int32_t& c : counts) {
            c = 0;
        }
    }

    bool write(const uint8_t* data, std::size_t size) {
        if (echo) {
            incoming.push_back({now_us, std::vector<uint8_t>(data, data + size)});
        }
        scanner.feed(data, size,
                     [this, size](const brainlink::Message& m) { answer(m, size); });
        return true;
    }

    void answer(const brainlink::Message& request, std::size_t request_size) {
        requests.push_back(request);
        if (!brainlink::isGetReadings(request)) {
            return;
        }
        if (drop_next > 0) {
            --drop_next;
            return;
        }
        brainlink::ReadingsWriter writer(boot_id);
        for (uint8_t port = 0; port < 3; ++port) {
            uint8_t data[records::encoder::kSize];
            records::encoder::write(counts[port], data);
            const bool empty = port == 2 && tw2_empty;
            writer.add(records::encoder::kSensorType, port,
                       empty ? brainlink::Health::kStarting : brainlink::Health::kOk,
                       empty ? 0 : 150, data, empty ? 0 : records::encoder::kSize);
        }
        const brainlink::Frame f = writer.frame(request.id);
        std::vector<uint8_t>   bytes(f.bytes.begin(), f.bytes.begin() + f.size);
        if (corrupt_next) {
            bytes[10] ^= 0x10;
            corrupt_next = false;
        }
        const uint64_t at = now_us + request_size * byte_us + turnaround_us +
                            bytes.size() * byte_us + extra_delay_us;
        extra_delay_us = 0;
        incoming.push_back({at, std::move(bytes)});
    }

    std::size_t read(uint8_t* data, std::size_t capacity) {
        std::size_t n = 0;
        while (!incoming.empty() && incoming.front().at_us <= now_us && n < capacity) {
            auto&             front = incoming.front().bytes;
            const std::size_t take  = std::min(capacity - n, front.size());
            std::copy(front.begin(), front.begin() + static_cast<std::ptrdiff_t>(take), data + n);
            front.erase(front.begin(), front.begin() + static_cast<std::ptrdiff_t>(take));
            n += take;
            if (front.empty()) {
                incoming.pop_front();
            }
        }
        return n;
    }

    Io io() {
        Io io;
        io.write   = [this](const uint8_t* d, std::size_t s) { return write(d, s); };
        io.read    = [this](uint8_t* d, std::size_t c) { return read(d, c); };
        io.now_us  = [this] { return now_us; };
        io.wait_us = [this](uint32_t us) { advance(us); };
        return io;
    }
};

// One 10 ms period: a transaction, then the rest of the period.
Poll period(Link& link, FakeBoard& board) {
    const uint64_t start = board.now_us;
    const Poll     poll  = link.transact();
    if (board.now_us - start < 10000) {
        board.advance(static_cast<uint32_t>(10000 - (board.now_us - start)));
    }
    return poll;
}

TEST(LocagatrLink, AsksForReadingsAndDecodesEncoderCounts) {
    FakeBoard board;
    Link      link(board.io(), Timing{});
    board.counts[0] = 1520;
    board.counts[1] = -40;
    board.counts[2] = 7;

    const Poll poll = period(link, board);
    ASSERT_EQ(board.requests.size(), 1u);
    EXPECT_EQ(board.requests[0].type, brainlink::Type::kGetReadings);
    ASSERT_TRUE(poll.answered);
    EXPECT_TRUE(poll.fresh);
    EXPECT_FALSE(poll.restarted);
    EXPECT_EQ(poll.readings.boot_id, 0x1111);
    EXPECT_EQ(poll.readings.count, 3u);
    EXPECT_EQ(encoderCount(poll, 0), 1520);
    EXPECT_EQ(encoderCount(poll, 1), -40);
    EXPECT_EQ(encoderCount(poll, 2), 7);
    EXPECT_FALSE(encoderCount(poll, 3).has_value()); // no such connector
    EXPECT_GT(link.stats().round_trip_us, 0u);
    // About 4.7 ms on the wire at 115200, seen at the next 1 ms read.
    EXPECT_LE(link.stats().round_trip_us, Timing{}.reply_timeout_us);
}

TEST(LocagatrLink, RecordWithoutReadingGivesNoCount) {
    FakeBoard board;
    board.tw2_empty = true;
    Link       link(board.io(), Timing{});
    const Poll poll = period(link, board);
    ASSERT_TRUE(poll.answered);
    EXPECT_TRUE(encoderCount(poll, 0).has_value());
    EXPECT_FALSE(encoderCount(poll, 2).has_value());
    const brainlink::Record* r =
        findRecord(poll.readings, records::encoder::kSensorType, 2);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->health, brainlink::Health::kStarting);
}

TEST(LocagatrLink, OneRequestInFlightAndIdsEchoed) {
    FakeBoard board;
    Link      link(board.io(), Timing{});
    for (int i = 0; i < 5; ++i) {
        period(link, board);
    }
    ASSERT_EQ(board.requests.size(), 5u);
    for (std::size_t i = 0; i < board.requests.size(); ++i) {
        EXPECT_EQ(board.requests[i].id, static_cast<uint8_t>(i + 1));
    }
    EXPECT_EQ(link.stats().replies, 5u);
}

TEST(LocagatrLink, TimeoutsMakeItStaleThenItRecovers) {
    FakeBoard board;
    Link      link(board.io(), Timing{});
    ASSERT_TRUE(period(link, board).fresh);

    board.drop_next = 2;
    EXPECT_FALSE(period(link, board).answered);
    EXPECT_TRUE(period(link, board).fresh); // two misses, limit 3
    board.drop_next = 3;
    Poll poll;
    for (int i = 0; i < 3; ++i) {
        poll = period(link, board);
    }
    EXPECT_FALSE(poll.fresh);
    EXPECT_EQ(link.stats().timeouts, 5u);
    EXPECT_FALSE(encoderCount(poll, 0).has_value()); // nothing answered

    EXPECT_TRUE(period(link, board).fresh);
}

TEST(LocagatrLink, LateReplyIsNeverTakenForTheNextRequest) {
    FakeBoard board;
    Link      link(board.io(), Timing{});
    period(link, board);
    board.counts[0]      = 100;
    board.extra_delay_us = 5500; // past the 5 ms timeout, inside the period
    EXPECT_FALSE(period(link, board).answered);
    EXPECT_EQ(link.stats().timeouts, 1u);
    board.counts[0] = 200;
    const Poll poll = period(link, board);
    EXPECT_EQ(link.stats().late_replies, 1u);
    EXPECT_EQ(link.stats().replies, 2u);
    EXPECT_EQ(encoderCount(poll, 0), 200); // the current answer, not the late one
}

TEST(LocagatrLink, EchoedRequestsAreIgnored) {
    FakeBoard board;
    board.echo = true;
    Link link(board.io(), Timing{});
    for (int i = 0; i < 5; ++i) {
        period(link, board);
    }
    EXPECT_EQ(link.stats().replies, 5u);
    EXPECT_EQ(link.stats().echoes, 5u);
    EXPECT_EQ(link.stats().timeouts, 0u);
}

TEST(LocagatrLink, CorruptReplyTimesOutAndIsCounted) {
    FakeBoard board;
    Link      link(board.io(), Timing{});
    period(link, board);
    board.corrupt_next = true;
    EXPECT_FALSE(period(link, board).answered);
    EXPECT_EQ(link.stats().timeouts, 1u);
    EXPECT_GT(link.stats().bad_bytes, 0u);
    EXPECT_TRUE(period(link, board).answered);
}

TEST(LocagatrLink, BoardRestartIsFlaggedOnce) {
    FakeBoard board;
    Link      link(board.io(), Timing{});
    board.counts[0] = 5000;
    period(link, board);
    board.restart();

    const Poll poll = period(link, board);
    EXPECT_TRUE(poll.answered);
    EXPECT_TRUE(poll.restarted);
    EXPECT_EQ(encoderCount(poll, 0), 0);
    EXPECT_EQ(link.stats().board_restarts, 1u);
    EXPECT_FALSE(period(link, board).restarted);
}

TEST(LocagatrLink, NoBoardIsNeverFresh) {
    FakeBoard board;
    board.drop_next = 1000;
    Link link(board.io(), Timing{});
    for (int i = 0; i < 5; ++i) {
        const Poll poll = period(link, board);
        EXPECT_FALSE(poll.answered);
        EXPECT_FALSE(poll.fresh);
    }
    EXPECT_EQ(link.stats().timeouts, 5u);
}

} // namespace
} // namespace locagatr
