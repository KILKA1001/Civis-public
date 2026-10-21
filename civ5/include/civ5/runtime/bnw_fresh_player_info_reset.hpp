#pragma once

#include "civ5/runtime/bnw_fresh_player_reset_prefix.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshPlayerInfoArray : std::uint8_t {
    early_reset_prefix,
    resource_used, resource_total, resource_gifted_to_minors,
    resource_export, resource_import, resource_from_minors, resource_siphoned,
    improvement_count, unit_combat_production, unit_combat_experience,
    free_building, free_promotion, unit_class_count, unit_class_making,
    building_class_count, building_class_making, project_making,
    hurry_count, hurry_modifier, loyal_member, gets_science_from_player,
    ai_services_pending,
};
struct BnwFreshPlayerInfoEvent {
    BnwFreshPlayerInfoArray array;
    std::int32_t player_id;
    bool operator==(const BnwFreshPlayerInfoEvent &) const = default;
};
enum class BnwFreshPlayerInfoBlockerCode {
    early_reset_or_selected_rules_unverified,
    selected_info_collections_unverified,
    allocation_budget_exceeded,
    ai_services_and_remaining_reset_pending,
};
struct BnwFreshPlayerInfoBlocker {
    BnwFreshPlayerInfoBlockerCode code;
    std::string message;
};
struct BnwFreshPlayerInfoSizes {
    std::uint32_t resources = 0, improvements = 0, unit_combats = 0;
    std::uint32_t buildings = 0, promotions = 0, unit_classes = 0;
    std::uint32_t building_classes = 0, projects = 0, hurries = 0;
    std::uint32_t vote_sources = 0, civ_players = 0;
};
struct BnwFreshPlayerInfoState {
    BnwFreshPlayerEarlyReset early;
    std::vector<std::int32_t> resource_used, resource_total;
    std::vector<std::int32_t> resource_gifted_to_minors;
    std::vector<std::int32_t> resource_export, resource_import;
    std::vector<std::int32_t> resource_from_minors, resource_siphoned;
    std::vector<std::int32_t> improvement_count;
    std::vector<std::int32_t> unit_combat_production, unit_combat_experience;
    std::vector<std::int32_t> free_building, free_promotion;
    std::vector<std::int32_t> unit_class_count, unit_class_making;
    std::vector<std::int32_t> building_class_count, building_class_making;
    std::vector<std::int32_t> project_making;
    std::vector<std::int32_t> hurry_count, hurry_modifier;
    std::vector<bool> loyal_member, gets_science_from_player;
};
struct BnwFreshPlayerInfoReset {
    BnwFreshPlayerInfoSizes sizes;
    std::vector<BnwFreshPlayerInfoState> players;
    std::vector<BnwFreshPlayerInfoEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshPlayerInfoBlocker> blockers;
    bool selected_arrays_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshPlayerInfoBlockerCode code) const;
    void rollback() noexcept;
};

// Continues fresh CvPlayer::reset's selected Info vectors through
// m_pabGetsScienceFromPlayer. It stops before the ordered AI/traits/treasury
// services, which are mandatory for a real initialized player.
BnwFreshPlayerInfoReset draft_bnw_fresh_player_info_reset(
    const BnwFreshGameContentResult &selected,
    const BnwFreshPlayerResetPrefix &early,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants,
    std::uint64_t maximum_total_cells = 32ULL * 1024 * 1024);

} // namespace civ5::runtime
