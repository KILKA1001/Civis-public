#pragma once

#include "civ5/runtime/bnw_fresh_player_objects.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshArchiveVectorStep : std::uint8_t {
    member_reset_projection,
    uninit_clear_projection,
    archive_traversal_pending,
};
struct BnwFreshArchiveVectorEvent {
    BnwFreshArchiveVectorStep step;
    std::string_view member;
    bool operator==(const BnwFreshArchiveVectorEvent &) const = default;
};
struct BnwFreshArchiveVectorState {
    std::vector<std::int32_t> values;
    std::uint32_t pending_commands = 0;
    bool archive_touched = false;
};
struct BnwFreshArchiveVectorProjection {
    BnwFreshArchiveVectorState state;
    std::vector<BnwFreshArchiveVectorEvent> source_effects;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_traversal_verified = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// Exact member effects from FAutoVector.h:386 reset() and :375 clear(),
// applied to an owned copy of the first CvPlayer vector. This projection does
// not attest that the opaque FAutoArchive::reset calls this member in a
// particular order or clears aggregate deltas. It cannot advance the builder
// past the archive call.
BnwFreshArchiveVectorProjection project_bnw_fresh_resource_vector_reset(
    const BnwFreshFirstPlayerObjects &objects,
    std::string_view selected_rules_sha256,
    BnwFreshArchiveVectorState initial);

} // namespace civ5::runtime
