#pragma once

#include "civ5/runtime/bnw_player_gold.hpp"
#include "civ5/runtime/bnw_turn_scheduler.hpp"
#include "civ5/runtime/research_turn.hpp"
#include <functional>
#include <optional>

namespace civ5::runtime {

enum class BnwResearchDirty : std::uint8_t {
    game_data_and_score, research_buttons
};
struct BnwPlayerResearchRules {
    const content::ResearchRules &research;
    const content::TechnologyRules &technologies;
    const content::GlobalDefineCache &defines;
    const BnwUnhappinessRules &unhappiness;
    const TreasuryRules &treasury;
    const CityYieldRules &cities;
    const MaintenanceRules &maintenance;
    std::int64_t active_team=-1,active_player=-1;
};
struct BnwPlayerResearchServices {
    std::function<bool()> lua_ready;
    std::function<std::optional<std::int64_t>(
        std::int64_t,const LiveSessionState &)> founder_benefits_read_only;
    // Must mutate only the supplied detached world, including the AI's queue
    // and RNG. It must not call external Lua/UI; those require another bridge.
    std::function<void(std::int64_t,LiveSessionState &)> choose_ai_on_detached;
    std::function<void(std::int64_t,BnwResearchDirty)> dirty_after_commit;
};

// Expansion2 CvPlayer.cpp:19532, CvPlayer::doResearch. Detached-state ONLY:
// a rejected technology grant may leave this temporary copy changed, so the
// enclosing transaction must discard it. Team grant/SetHasTech world effects
// and chooseTech UI are explicit blockers. Steam achievements are excluded.
LiveActionResult run_bnw_player_research_phase(
    LiveSessionState &,std::int64_t,const BnwPlayerResearchRules &,
    const BnwPlayerResearchServices &);
BnwTurnHandler make_bnw_player_research_handler(
    BnwPlayerResearchRules,BnwPlayerResearchServices);
LiveActionResult publish_bnw_player_research(
    LiveSession &,const BnwPlayerResearchServices &);
LiveActionResult apply_bnw_player_research(
    LiveSession &,std::int64_t,const BnwPlayerResearchRules &,
    const BnwPlayerResearchServices &);
LiveActionResult run_bnw_player_turn_with_research(
    LiveSession &,std::int64_t,std::span<const BnwTurnHandler>,
    const BnwPlayerResearchRules &,const BnwPlayerResearchServices &);

// Composes the two implemented player phases within one detached turn. Every
// other required phase remains caller-supplied and subject to preflight. Both
// UI publication queues are drained only after the entire turn commits.
LiveActionResult run_bnw_player_turn_with_gold_and_research(
    LiveSession &,std::int64_t,std::span<const BnwTurnHandler>,
    const BnwPlayerGoldRules &,const BnwPlayerGoldServices &,
    const BnwPlayerResearchRules &,const BnwPlayerResearchServices &);

} // namespace civ5::runtime
