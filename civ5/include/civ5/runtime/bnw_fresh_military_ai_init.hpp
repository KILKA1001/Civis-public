#pragma once

#include "civ5/runtime/bnw_fresh_economic_ai_init.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshMilitaryInitStep : std::uint8_t {
    selected_strategies_and_player_bound,
    diplomacy_ai_handle_pending,
    strategy_flavor_and_opponent_arrays_allocated,
    scalar_counters_reset,
    strategy_and_opponent_prefix_reset,
    city_specialization_pending,
};
struct BnwFreshMilitaryInitEvent {
    BnwFreshMilitaryInitStep step;
    std::int32_t player_id;
    bool operator==(const BnwFreshMilitaryInitEvent &) const = default;
};
enum class BnwFreshMilitaryInitBlockerCode {
    player_or_economic_init_unverified,
    selected_military_info_unverified,
    sdk_opponent_array_overflow,
    allocation_budget_exceeded,
    diplomacy_handle_and_remaining_ai_pending,
};
struct BnwFreshMilitaryInitBlocker {
    BnwFreshMilitaryInitBlockerCode code;
    std::string message;
};
struct BnwFreshMilitaryAIState {
    std::int32_t player_id = -1;
    std::vector<bool> using_strategy;
    std::vector<std::int32_t> turn_strategy_adopted;
    std::vector<std::optional<std::int32_t>> temp_flavors;
    // SDK allocates these to MAX_CIV_PLAYERS but Reset writes only the first
    // GetNumMilitaryAIStrategies cells. The remainder is uninitialized.
    std::vector<std::optional<std::int32_t>> last_war_state;
    std::vector<std::optional<std::int32_t>> last_military_threat;
    std::vector<std::optional<std::int32_t>> last_military_strength;
    std::vector<std::optional<std::int32_t>> last_target_value;
    std::int32_t total_threat_weight = 1, army_type_being_built = -1;
    std::int32_t land_units = 0, ranged_land_units = 0;
    std::int32_t mobile_land_units = 0, air_units = 0;
    std::int32_t anti_air_units = 0, melee_land_units = 0;
    std::int32_t naval_units = 0, land_units_in_armies = 0;
    std::int32_t naval_units_in_armies = 0, barbarian_camps = 0;
    std::int32_t visible_barbarians = 0, recommended_military_size = 0;
    std::int32_t mandatory_reserve_size = 0;
    std::int32_t land_defense_state = -1, naval_defense_state = -1;
    std::int32_t operations_build_skipped = 0;
};
struct BnwFreshMilitaryAIInit {
    std::vector<BnwFreshMilitaryAIState> players;
    std::vector<BnwFreshMilitaryInitEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshMilitaryInitBlocker> blockers;
    bool arrays_and_reset_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshMilitaryInitBlockerCode code) const;
    void rollback() noexcept;
};

// Detached CvMilitaryAI::Init/Reset immediately after EconomicAI. The
// DiplomacyAI object exists in the SDK CvPlayer constructor, but no real
// counterpart is bound here; its pointer remains an explicit blocker.
BnwFreshMilitaryAIInit draft_bnw_fresh_military_ai_init(
    const BnwFreshGameContentResult &selected,
    const BnwFreshPlayerInfoReset &player_info,
    const BnwFreshEconomicAIInit &economic,
    std::string_view selected_rules_sha256,
    std::uint64_t maximum_total_cells = 16ULL * 1024 * 1024);

} // namespace civ5::runtime
