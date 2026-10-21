#pragma once

#include "civ5/runtime/bnw_fresh_player_post_operations.hpp"
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshPlayerScalarWrite {
    std::string_view sdk_member;
    std::int64_t value;
    bool boolean_value;
    std::uint16_t sdk_line;
    bool operator==(const BnwFreshPlayerScalarWrite &) const = default;
};

// Generated from Expansion2 CvPlayer.cpp:733-968. This is a detached
// projection of assignments AFTER archive.clearDelta. It must not be used to
// advance the source-ordered aggregate while that archive call is unresolved.
std::span<const BnwFreshPlayerScalarWrite>
bnw_fresh_player_post_delta_scalar_writes() noexcept;

enum class BnwFreshPlayerPostDeltaScalarsBlocker {
    none,
    predecessor_or_rules_invalid,
    field_set_invalid,
    archive_clear_delta_pending,
};
struct BnwFreshPlayerPostDeltaScalarsDraft {
    std::map<std::string,std::int64_t> fields;
    std::vector<BnwFreshPlayerScalarWrite> source_order;
    std::string selected_rules_sha256;
    BnwFreshPlayerPostDeltaScalarsBlocker blocker =
        BnwFreshPlayerPostDeltaScalarsBlocker::none;
    std::string_view blocked_at;
    bool detached_value_projection_complete = false;
    bool archive_clear_delta_verified = false;
    bool archive_reset_traversal_verified = false;
    bool player_uninit_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

BnwFreshPlayerPostDeltaScalarsDraft
project_bnw_fresh_player_post_delta_scalars(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshPlayerPostOperationsDraft &predecessor,
    std::string_view selected_rules_sha256,
    std::map<std::string,std::int64_t> initial_fields);

} // namespace civ5::runtime
