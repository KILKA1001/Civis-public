#pragma once

#include "civ5/runtime/bnw_fresh_military_ai_init.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshCitySpecializationStep : std::uint8_t {
    military_ai_dependency_pending,
    selected_specializations_and_player_bound,
    flags_and_sentinels_reset,
    weight_vectors_cleared,
    wonder_production_ai_pending,
};
struct BnwFreshCitySpecializationEvent {
    BnwFreshCitySpecializationStep step;
    std::int32_t player_id;
    bool operator==(const BnwFreshCitySpecializationEvent &) const = default;
};
enum class BnwFreshCitySpecializationBlockerCode {
    military_or_selected_rules_unverified,
    selected_specialization_info_unverified,
    state_allocation_failed,
    military_handle_and_wonder_ai_pending,
};
struct BnwFreshCitySpecializationBlocker {
    BnwFreshCitySpecializationBlockerCode code;
    std::string message;
};
struct BnwFreshCitySpecializationState {
    std::int32_t player_id = -1;
    bool specializations_dirty = false, interrupt_wonders = false;
    std::int32_t next_specialization = -1, next_wonder = -1;
    std::int32_t wonder_city_id = -1, next_wonder_weight = 0;
    std::int32_t last_turn_evaluated = 0;
    std::vector<std::int32_t> yield_weights, production_subtype_weights;
};
struct BnwFreshCitySpecializationInit {
    std::uint32_t selected_specialization_slots = 0;
    std::vector<BnwFreshCitySpecializationState> players;
    std::vector<BnwFreshCitySpecializationEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshCitySpecializationBlocker> blockers;
    bool city_specialization_init_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshCitySpecializationBlockerCode code) const;
    void rollback() noexcept;
};

// Detached CvCitySpecializationAI::Init/Reset after MilitaryAI. Source Init
// binds the selected collection and player, then resets scalar fields and
// clears two vectors. WonderProductionAI::Init is the next required call.
BnwFreshCitySpecializationInit draft_bnw_fresh_city_specialization_init(
    const BnwFreshGameContentResult &selected,
    const BnwFreshMilitaryAIInit &military,
    std::string_view selected_rules_sha256);

} // namespace civ5::runtime
