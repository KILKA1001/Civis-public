#pragma once
#include "civ5/runtime/binary_reader.hpp"
#include <array>

namespace civ5::saves {
// Source-ordered observation of a prefix only, not a reconstructed game.
// The 403694/VP 5.4.2 corpus is compared with published commit 8e180cfd.
// The exact saved build's 9f0904a01 source has not been retrieved.
struct VpPrefixObservation {
    std::uint32_t save_version = 0;
    std::array<std::uint32_t, 4> game_data_hash{};
    std::string gamecore_build;
    std::uint64_t end_offset = 0;
    std::array<std::int64_t, 33> scalars{};
    static constexpr std::array<const char *, 33> scalar_names{"end_turn_messages_sent",
                                                               "elapsed_game_turns",
                                                               "start_turn",
                                                               "winning_turn",
                                                               "start_year",
                                                               "estimate_end_turn",
                                                               "default_estimate_end_turn",
                                                               "turn_slice",
                                                               "cutoff_slice",
                                                               "num_cities",
                                                               "total_population",
                                                               "total_economic_value",
                                                               "highest_economic_value",
                                                               "median_economic_value",
                                                               "no_nukes_count",
                                                               "nukes_exploded",
                                                               "max_population",
                                                               "global_asset_counter_previous_turns",
                                                               "global_asset_counter_current_turn",
                                                               "init_population",
                                                               "init_land",
                                                               "init_tech",
                                                               "init_wonders",
                                                               "ai_auto_play",
                                                               "united_nations_countdown",
                                                               "num_victory_votes_tallied",
                                                               "num_victory_votes_expected",
                                                               "votes_needed_for_diplo_victory",
                                                               "map_score_mod",
                                                               "city_found_value_reference",
                                                               "num_reference_cities",
                                                               "num_major_civs_alive_at_start",
                                                               "num_minor_civs_alive_at_start"};
};
VpPrefixObservation observe_vp_prefix(runtime::BinaryReader &input);
} // namespace civ5::saves
