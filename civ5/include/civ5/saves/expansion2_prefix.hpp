#pragma once
#include "civ5/runtime/binary_reader.hpp"
#include <array>

namespace civ5::saves {
// Official Expansion2 SDK CvGame::Write: version + 29 signed integer fields.
// No mod-layout inference and no construction of game objects at this boundary.
struct Expansion2PrefixObservation {
    std::uint32_t version = 0;
    std::uint64_t end_offset = 0;
    std::array<std::int64_t, 29> scalars{};
    static constexpr std::array<const char*, 29> scalar_names{
        "end_turn_messages_sent", "elapsed_game_turns", "start_turn", "winning_turn", "start_year",
        "estimate_end_turn", "default_estimate_end_turn", "turn_slice", "cutoff_slice", "num_cities",
        "total_population", "no_nukes_count", "nukes_exploded", "max_population", "unused_1", "unused_2",
        "unused_3", "init_population", "init_land", "init_tech", "init_wonders", "ai_auto_play",
        "total_religion_tech_cost", "cached_world_religion_tech_progress", "united_nations_countdown",
        "num_victory_votes_tallied", "num_victory_votes_expected", "votes_needed_for_diplo_victory", "map_score_mod"};
};
Expansion2PrefixObservation observe_expansion2_prefix(runtime::BinaryReader& input);
} // namespace civ5::saves
