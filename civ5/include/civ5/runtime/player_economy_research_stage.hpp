#pragma once
#include "civ5/runtime/research_turn.hpp"
#include "civ5/runtime/treasury_turn.hpp"
#include <functional>

namespace civ5::runtime {
struct PlayerEconomyResearchStageResult {
    GoldTurnResult gold;
    ResearchTurnResult research;
};

// Each GameCore supplies the step between gold and later player-turn phases.
// The profile tag prevents accidentally binding another GameCore's handler.
struct GameCoreAfterGoldStep {
    content::TechnologyProfile profile;
    std::function<void(TreasuryTurnContext &,std::int64_t)> run;
};

// CvPlayer's BNW and VP turn paths call DoGold before doResearch. Calculations
// must observe the same live world: research can see gold and expense changes
// made by the GameCore step. Profiles can fill intervening phases through
// the hook; unbound mod events and later phases still prevent this diagnostic
// slice from being a complete player turn. It does not advance the game clock.
// The caller must bind equivalent income-script services in both contexts;
// std::function callbacks cannot be compared for identity here.
// Required host callbacks may throw after source-order mutations. Callers
// must restore their checkpoint before retrying an interrupted segment.
PlayerEconomyResearchStageResult do_player_economy_research_stage(
    TreasuryTurnContext &gold, ResearchTurnContext &research, std::int64_t player,
    const GameCoreAfterGoldStep &after_gold);
} // namespace civ5::runtime
