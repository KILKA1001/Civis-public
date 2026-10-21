#pragma once
#include "civ5/saves/army_operations.hpp"
#include "civ5/saves/cities.hpp"
#include "civ5/saves/game_progression.hpp"
#include "civ5/saves/player_administration.hpp"
#include "civ5/saves/player_core.hpp"
#include "civ5/saves/player_diplomacy.hpp"
#include "civ5/saves/player_espionage.hpp"
#include "civ5/saves/player_knowledge.hpp"
#include "civ5/saves/player_planning.hpp"
#include "civ5/saves/player_strategy.hpp"
#include "civ5/saves/player_tail.hpp"
#include "civ5/saves/player_trade_culture.hpp"
#include "civ5/saves/player_traits.hpp"
#include "civ5/saves/units.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::saves {
struct AllPlayersContext {
    // This is source wire context selected by the rules profile. It is not a
    // host capacity and may be larger than the original game's fixed arrays.
    std::uint64_t player_count = 64;
    PlayerStrategyWireDimensions strategy;
    PlayerDiplomacyWireDimensions diplomacy;
    PlayerPlanningWireDimensions planning;
    std::optional<PlayerTraitsWireDimensions> traits;
    std::optional<PlayerTradeCultureWireDimensions> trade_culture;
    std::optional<std::uint64_t> bnw_projects;
    runtime::ProfileExtensions profile_extensions;
};

// BNW stores its project collection (including null-hash slots) before the
// players. Resolve that wire span from the save, never from currently installed
// rules. An explicitly supplied incompatible count is rejected.
AllPlayersContext resolve_saved_player_context(const GameProgressionObservation &progression,
                                               AllPlayersContext context);

struct SavedPlayerObservation {
    std::uint64_t index = 0, start_offset = 0, end_offset = 0;
    std::int64_t saved_player_id = 0;
    PlayerCoreObservation core;
    PlayerStrategyObservation strategy;
    PlayerDiplomacyObservation diplomacy;
    PlayerKnowledgeObservation knowledge;
    PlayerPlanningObservation planning;
    PlayerTraitsObservation traits;
    PlayerEspionageObservation espionage;
    PlayerTradeCultureObservation trade_culture;
    PlayerAdministrationObservation administration;
    CitiesObservation cities;
    UnitsObservation units;
    ArmyOperationsObservation army_operations;
    PlayerTailObservation tail;
};

struct AllPlayersObservation {
    GameStateLayout layout;
    AllPlayersContext context;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<SavedPlayerObservation> players;
};

// Reads every serialized player in source order. Each player is parsed with
// the same complete sequence as the first-player importer. A failed player
// aborts the whole observation and includes its index; no partial collection
// is returned. No original load hooks, AI evaluation, ID remapping or Steam
// calls are performed.
AllPlayersObservation observe_all_players(runtime::BinaryReader &input, GameStateLayout layout,
                                          AllPlayersContext context = {}, GameStateBudget budget = {});
} // namespace civ5::saves
