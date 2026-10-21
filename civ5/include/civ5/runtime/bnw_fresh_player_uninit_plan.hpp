#pragma once

#include "civ5/runtime/bnw_fresh_player_objects.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshPlayerUninitKind : std::uint8_t {
    sync_archive_reset,
    vector_clear,
    service_uninit,
    optional_service_uninit,
    later_player_state_reset,
};
struct BnwFreshPlayerUninitEvent {
    BnwFreshPlayerUninitKind kind;
    std::string_view sdk_member;
    bool operator==(const BnwFreshPlayerUninitEvent &) const = default;
};
enum class BnwFreshPlayerUninitBlockerCode {
    first_player_or_rules_unverified,
    sync_archive_reset_semantics_unavailable,
    service_uninit_effects_pending,
};
struct BnwFreshPlayerUninitBlocker {
    BnwFreshPlayerUninitBlockerCode code;
    std::string message;
};
struct BnwFreshPlayerUninitPlan {
    std::int32_t player_id = -1;
    std::vector<BnwFreshPlayerUninitEvent> planned_source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshPlayerUninitBlocker> blockers;
    bool preflight_complete = false;
    bool sync_archive_reset_verified = false;
    bool uninit_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshPlayerUninitBlockerCode code) const;
    void rollback() noexcept;
};

// Read-only preflight of CvPlayer::reset(eID) -> FAutoArchive::reset ->
// CvPlayer::uninit. FirePlace SDK declares FAutoArchive::reset but does not
// contain its implementation; no archive mutation or service callback is
// published by this plan. The ordered vector and service frontier is used to
// keep the fresh builder at the first unresolved effect.
BnwFreshPlayerUninitPlan plan_bnw_fresh_first_player_uninit(
    const BnwFreshGameContentResult &selected,
    const BnwFreshFirstPlayerObjects &objects,
    std::string_view selected_rules_sha256);

} // namespace civ5::runtime
