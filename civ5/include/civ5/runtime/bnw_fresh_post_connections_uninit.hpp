#pragma once

#include "civ5/runtime/bnw_fresh_mid_ai_uninit.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshNotificationsUninitState {
    std::int32_t player = -1, current_lookup_index = 0;
    std::vector<std::byte> notifications;
    std::int32_t begin_index = -1, end_index = -1;
    bool operator==(const BnwFreshNotificationsUninitState &) const = default;
};
struct BnwFreshDiplomacyRequestsUninitState {
    std::int32_t player = -1, next_ai_player = -1;
    std::vector<std::byte> requests;
    bool request_active = false;
    std::int32_t request_active_from_player = -1;
    bool operator==(const BnwFreshDiplomacyRequestsUninitState &) const = default;
};
struct BnwFreshDangerPlotsUninitState {
    std::int32_t player = -1;
    std::vector<std::int32_t> danger_plots;
    bool array_allocated = false, dirty = false;
    // CvDangerPlots::Uninit does not alter its selected AI modifiers.
    std::vector<std::byte> untouched_modifiers;
    bool operator==(const BnwFreshDangerPlotsUninitState &) const = default;
};

inline constexpr std::array<std::string_view,8> kBnwFreshTraitClearOrder{
    "m_aiResourceQuantityModifier", "m_abNoTrain",
    "m_paiMovesChangeUnitCombat", "m_paiMaintenanceModifierUnitCombat",
    "m_ppaaiImprovementYieldChange", "m_ppaaiSpecialistYieldChange",
    "m_ppaaiUnimprovedFeatureYieldChange", "m_aFreeResourceXCities"};

struct BnwFreshPostConnectionsUninitInput {
    // Disengaged means the CvPlayer pointer was null, so the conditional call
    // does not occur. Engaged values are detached, not native service objects.
    std::optional<BnwFreshNotificationsUninitState> notifications;
    std::optional<BnwFreshDiplomacyRequestsUninitState> diplomacy_requests;
    std::vector<std::byte> treasury_opaque_state;
    std::array<std::vector<std::byte>,8> trait_containers;
    std::optional<BnwFreshDangerPlotsUninitState> danger_plots;
    bool operator==(const BnwFreshPostConnectionsUninitInput &) const = default;
};
struct BnwFreshPostConnectionsUninitDraft {
    BnwFreshPostConnectionsUninitInput state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool later_containers_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

BnwFreshPostConnectionsUninitInput
make_bnw_fresh_post_connections_constructor_state();

// Expansion2 CvPlayer.cpp:689-703. Optional Notifications/DiplomacyRequests,
// empty Treasury Uninit, eight ordered Traits clears, optional DangerPlots.
// The following three yield-vector clears remain the next frontier.
BnwFreshPostConnectionsUninitDraft project_bnw_fresh_post_connections_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshMidAiUninitDraft &mid_ai,
    std::string_view selected_rules_sha256,
    BnwFreshPostConnectionsUninitInput initial);

} // namespace civ5::runtime
