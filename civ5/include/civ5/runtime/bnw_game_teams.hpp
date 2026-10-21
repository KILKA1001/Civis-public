#pragma once

#include "civ5/runtime/bnw_turn_scheduler.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace civ5::runtime {

enum class BnwTeamTurnStepKind {
    barbarian_technology_gate,
    turns_at_war,
    turns_locked_at_war,
    minor_technology_gate,
    clear_no_trade_technology,
    small_awards_gate,
    circumnavigation_gate,
};

struct BnwTeamTurnStep {
    BnwTeamTurnStepKind kind;
    std::int64_t team=-1, target=-1, before=0, after=0;
};

struct BnwTeamTurnPlan {
    // Observable changes and mandatory gates in exact CvGame::doTurn team
    // order. Planning alone does not mutate the live world.
    std::vector<BnwTeamTurnStep> steps;
    std::uint64_t alive_teams=0;
};

struct BnwTeamTurnActiveRules {
    // Borrowed through the synchronous handler call; the selected host owns
    // these for at least as long as its LiveSession.
    const content::RulesDatabase &database;
    const content::TechnologyRules &technologies;
    const content::GlobalDefineCache &defines;
};

struct BnwTeamTurnSafeBranch {
    BnwTeamTurnPlan plan;
    bool safe=false;
    std::uint64_t barbarian_grants=0, minor_grants=0;
    bool awards_present=false, circumnavigation_possible=false;
    std::string blocker;
};

// Deterministic war/lock/no-trade prefix. Technology, award and circumnavigation
// calls are represented by ordered gates.
BnwTeamTurnPlan plan_bnw_game_teams_prefix(const LiveSessionState &state);
// Resolve every gate against the active BNW rules and live map. A technology
// grant or other unsupported mandatory effect makes safe=false.
BnwTeamTurnSafeBranch preflight_bnw_game_teams_safe_branch(
    const LiveSessionState &state,const BnwTeamTurnActiveRules &rules);

// Commits only after preflight proves every source gate has no effect. War and
// lock counters plus both raw and owned no-trade flags update in source order.
BnwTurnHandler make_bnw_game_teams_handler(const BnwTeamTurnActiveRules &rules);
// Without activated rules, an attempted phase always rejects.
BnwTurnHandler make_bnw_game_teams_handler();

} // namespace civ5::runtime
