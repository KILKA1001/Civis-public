#pragma once
#include <string>

namespace civ5::runtime {
// The imported payload and activated logical rules must both match when a
// live session overlay is reopened.
struct SessionCheckpointIdentity {
    std::string payload_sha256, rules_sha256;
};
} // namespace civ5::runtime
