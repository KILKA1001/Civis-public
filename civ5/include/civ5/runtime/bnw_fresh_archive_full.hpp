#pragma once

#include "civ5/runtime/bnw_fresh_player_objects.hpp"
#include "civ5/runtime/bnw_fresh_player_reset_prefix.hpp"
#include "civ5/runtime/bnw_player_archive_members.generated.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace civ5::runtime {

using BnwArchiveValue = std::variant<
    std::int32_t, std::uint32_t, bool, std::string,
    std::vector<std::int32_t>, std::vector<bool>,
    std::vector<std::array<std::int32_t,6>>,
    std::vector<std::pair<std::uint32_t,std::int32_t>>>;
struct BnwArchiveMemberState {
    std::string_view name;
    BnwArchiveMemberKind kind;
    BnwArchiveValue value;
    std::uint32_t pending_vector_commands = 0;
};
enum class BnwArchiveExplicitWriteKind : std::uint8_t {
    uninit_vector_clear_before_services,
    uninit_service_callbacks_pending,
    uninit_vector_clear_after_services,
    uninit_archive_clear_delta_pending,
    uninit_scalar_after_services,
};
struct BnwArchiveExplicitWrite {
    BnwArchiveExplicitWriteKind kind;
    std::string_view member;
    std::int64_t scalar_value = 0;
};
struct BnwArchiveResetPrefixWrite {
    std::string_view member;
    BnwArchiveValue value;
};
struct BnwFreshFullArchiveProjection {
    std::vector<BnwArchiveMemberState> defaults;
    // Ordered source-authored direct writes, staged without invoking the
    // intervening CvPlayer::uninit services or publishing a whole player.
    std::vector<BnwArchiveExplicitWrite> later_writes;
    std::vector<BnwArchiveResetPrefixWrite> reset_prefix_writes;
    std::string selected_rules_sha256;
    bool all_registered_member_defaults = false;
    bool aggregate_traversal_verified = false;
    bool uninit_services_complete = false;
    bool reset_prefix_staged = false;
    bool player_reset_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// First player starts with registered constructor values. The caller can
// supply an owned matching archive image to test reset from nondefaults.
// Both paths value-initialize every registered Expansion2 FAutoVariable in
// the checked-in generated catalog. This does not assert the unavailable
// FAutoArchive::reset traversal/order or execute uninit service callbacks.
std::vector<BnwArchiveMemberState> make_bnw_fresh_archive_defaults();
BnwFreshFullArchiveProjection project_bnw_fresh_full_archive_reset(
    const BnwFreshFirstPlayerObjects &objects,
    std::string_view selected_rules_sha256,
    std::vector<BnwArchiveMemberState> initial);
// Stage the registered subset of the existing selected early reset draft:
// eID/personality, tutorial flag, yield/player vectors, options and strings.
// The generated unregistered fields and all later reset callbacks remain in
// their current owners; staging cannot make player_reset_complete true.
bool stage_bnw_fresh_archive_reset_prefix(
    BnwFreshFullArchiveProjection &projection,
    const BnwFreshPlayerResetPrefix &early_reset);

} // namespace civ5::runtime
