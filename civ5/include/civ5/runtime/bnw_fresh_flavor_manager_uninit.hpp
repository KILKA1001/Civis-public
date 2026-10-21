#pragma once

#include "civ5/runtime/bnw_fresh_player_techs_uninit.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshFlavorManagerServiceState {
    // CvFlavorManager constructor does not initialize m_pPlayer.
    std::optional<bool> player_bound;
    std::optional<std::vector<std::int32_t>> personality_flavor;
    std::optional<std::vector<std::int32_t>> active_flavor;
    // Detached identities of CvFlavorRecipient pointers, preserving list order.
    std::vector<std::int32_t> target_recipients;
    bool operator==(const BnwFreshFlavorManagerServiceState &) const = default;
};

struct BnwFreshFlavorManagerUninitDraft {
    BnwFreshFlavorManagerServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

BnwFreshFlavorManagerServiceState make_bnw_fresh_flavor_manager_constructor_state();

// Expansion2 CvPlayer.cpp:682 and CvFlavorManager.cpp:225-230. This Uninit
// deletes both arrays and clears the target list; m_pPlayer is untouched.
// Archive traversal and TacticalAI/later services remain blocked.
BnwFreshFlavorManagerUninitDraft project_bnw_fresh_flavor_manager_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPlayerTechsUninitDraft &player_techs,
    std::string_view selected_rules_sha256,
    BnwFreshFlavorManagerServiceState initial);

} // namespace civ5::runtime
