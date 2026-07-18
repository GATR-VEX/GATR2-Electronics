// encoder.cpp

#include "records/encoder.h"

#include "brainlink/bytes.h"

namespace records
{
namespace encoder
{

void write(int32_t count, uint8_t* out) { brainlink::putU32(out, static_cast<uint32_t>(count)); }

std::optional<int32_t> read(const uint8_t* data, std::size_t size) {
    if (size != kSize) {
        return std::nullopt;
    }
    return static_cast<int32_t>(brainlink::getU32(data));
}

} // namespace encoder
} // namespace records
