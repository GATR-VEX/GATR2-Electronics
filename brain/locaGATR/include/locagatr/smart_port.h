// smart_port.h
// A V5 Smart Port as the link's Io. Source in pros/.

#pragma once
#include <cstdint>
#include <optional>

#include "locagatr/link.h"

namespace locagatr
{

// Port 1..21. Nullopt when the port could not be set up.
std::optional<Io> openSmartPort(uint8_t port, int32_t baud);

} // namespace locagatr
