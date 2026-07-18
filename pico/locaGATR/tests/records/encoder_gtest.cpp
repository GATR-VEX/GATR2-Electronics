// encoder_gtest.cpp

#include "records/encoder.h"

#include <gtest/gtest.h>

namespace records
{
namespace encoder
{
namespace
{

TEST(EncoderRecord, RoundTrip) {
    for (const int32_t sent : {0, 123456, -98765, 2147483647, -2147483647 - 1}) {
        uint8_t data[kSize];
        write(sent, data);
        EXPECT_EQ(read(data, sizeof(data)), sent);
    }
}

TEST(EncoderRecord, WrongSizeIsRejected) {
    const uint8_t data[kSize + 1] = {};
    EXPECT_FALSE(read(data, kSize - 1));
    EXPECT_FALSE(read(data, kSize + 1));
}

} // namespace
} // namespace encoder
} // namespace records
