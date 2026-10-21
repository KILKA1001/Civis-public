#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/saves/game_progression.hpp"
#include "civ5/saves/player_knowledge.hpp"
#include "civ5/saves/vp_player_knowledge_profile.hpp"
#include "civ5/saves/player_strategy.hpp"
#include "civ5/saves/vp_player_strategy_profile.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/saves/team_state.hpp"
#include "civ5/saves/vp_team_profile.hpp"

namespace civ5::content {
struct VpSaveWireDimensions {
    saves::RosterWireDimensions roster;
    saves::ProgressionWireDimensions progression;
};
// Observe the current rules after all selected actions and VP remapping. Does
// not load installed mods or change rules, and does not verify save/profile
// identity. Slots describe the selected source GameCore, not Civis capacities.
VpSaveWireDimensions vp_save_wire_dimensions(const RulesDatabase &rules, std::uint64_t player_slots,
                                             std::uint64_t team_slots);
saves::VpTeamContentDimensions vp_team_content_dimensions(const RulesDatabase &rules);
saves::VpPlayerStrategyWireDimensions player_strategy_content_dimensions(const RulesDatabase &rules);
saves::VpPlayerKnowledgeWireDimensions player_knowledge_content_dimensions(const RulesDatabase &rules);

// Compile-time dimensions of a caller-selected VP GameCore. These cannot be
// inferred from table row counts (for example, fixed TerrainTypes differs from
// the runtime Terrains collection). Zero-initialized context is not sufficient.
struct VpSessionSourceDimensions {
    std::uint64_t players = 0, teams = 0, civ_players = 0, civ_teams = 0, major = 0, minor = 0;
    std::uint64_t fixed_players = 0, fixed_teams = 0;
    std::uint64_t yields = 0, domains = 0, terrain_types = 0;
    std::uint64_t approaches = 0, minor_approaches = 0, statements = 0, log_entries = 0;
};
// Produce full map/team/player decode context from the same activated rules
// used by gameplay and rendering. The caller must run the source ID remapper
// first. This function neither mutates rules nor certifies save/mod identity.
saves::SavedSessionDecodeContext vp_saved_session_context(const RulesDatabase &rules,
                                                          const VpSessionSourceDimensions &source);
} // namespace civ5::content
