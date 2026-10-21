#pragma once

#include "civ5/saves/all_players.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/ui/top_panel.hpp"
#include <cstdint>
#include <optional>

namespace civ5::ui {

// Saved values that use a source scale keep that scale explicit. BNW stores
// culture and Golden Age progress as whole units; VP stores them in
// hundredths. No conversion is performed here because TopPanel's presentation
// contract currently accepts whole integers only.
struct TopPanelSavedScaledValue {
    std::int64_t raw = 0;
    std::int32_t scale = 1;
};

// Exact values that can be read from a SavedPlayerObservation without running
// Civ V load hooks or recomputing cached yields. Missing optionals are
// intentional: the save parser does not retain the derived runtime values
// needed to fill the whole TopPanelState.
struct TopPanelSavedProjection {
    std::optional<std::int32_t> city_count;
    std::optional<std::int32_t> gold;
    std::optional<std::int32_t> gold_per_turn_from_diplomacy;
    std::optional<std::int32_t> city_connection_gold_times100;
    std::optional<std::int32_t> unit_cost;
    std::optional<std::int32_t> unit_supply;
    std::optional<std::int32_t> building_maintenance;
    std::optional<std::int32_t> improvement_maintenance;
    std::optional<std::int32_t> happiness_from_buildings;
    std::optional<std::int32_t> extra_happiness_per_luxury;
    std::optional<std::int32_t> happiness_per_city;
    std::optional<std::int32_t> happiness_from_minors;
    std::optional<std::int32_t> total_happiness;
    std::optional<std::int32_t> total_unhappiness;
    std::optional<TopPanelSavedScaledValue> golden_age_progress;
    std::optional<std::int32_t> golden_age_turns;
    std::optional<TopPanelSavedScaledValue> culture;
    // Serialized faith balance is exact at the save boundary. Faith income,
    // religious progression and tourism are derived live queries, not saves.
    std::optional<TopPanelSavedScaledValue> faith;
    std::optional<std::int32_t> next_policy_cost;
    std::optional<std::int32_t> unit_production_maintenance_mod;
};

TopPanelSavedProjection project_top_panel_saved_player(const saves::SavedPlayerObservation &observation);

// Applies only fields whose source units match TopPanelPlayerState. Scaled
// VP values remain available in the projection and are deliberately not
// truncated into the whole-unit presentation state.
void apply_top_panel_saved_projection(TopPanelPlayerState &state,
                                      const TopPanelSavedProjection &projection);

// Select by saved player identity, not collection position. The result stays
// partial: a screen owner must supply derived rates and dates before publishing
// a complete TopPanelState. Missing or ambiguous identities are rejected.
TopPanelSavedProjection project_top_panel_saved_player(const saves::SavedSessionSnapshot &snapshot,
                                                       std::int64_t saved_player_id);

} // namespace civ5::ui
