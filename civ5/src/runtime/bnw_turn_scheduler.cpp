#include "civ5/runtime/bnw_turn_scheduler.hpp"
#include <algorithm>
#include <array>
#include <exception>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace civ5::runtime {
namespace {
// Source: installed Civilization V SDK/CvGameCoreSource/
// CvGameCoreDLL_Expansion2/CvGame.cpp, CvGame::doTurn at line 7480.
constexpr std::array game_order{
    BnwTurnPhase::game_engine_begin,
    BnwTurnPhase::game_barbarians_begin,
    BnwTurnPhase::game_caches_and_score,
    BnwTurnPhase::game_deals,
    BnwTurnPhase::game_teams,
    BnwTurnPhase::game_map,
    BnwTurnPhase::game_interface_turn,
    BnwTurnPhase::game_barbarian_camps,
    BnwTurnPhase::game_barbarian_units,
    BnwTurnPhase::game_religion,
    BnwTurnPhase::game_trade,
    BnwTurnPhase::game_leagues,
    BnwTurnPhase::game_culture,
    BnwTurnPhase::game_clock,
    BnwTurnPhase::game_activation,
    BnwTurnPhase::game_victory,
    BnwTurnPhase::game_publish,
};
// Source: the same SDK's CvPlayer.cpp, CvPlayer::doTurn at line 4038 and
// CvPlayer::doTurnPostDiplomacy at line 4121. City turns precede DoGold;
// culture/policies intervene between DoGold and doResearch.
constexpr std::array player_order{
    BnwTurnPhase::player_cache,
    BnwTurnPhase::player_ai_pre,
    BnwTurnPhase::player_timers_and_revolts,
    BnwTurnPhase::player_trade_strategy_diplomacy,
    BnwTurnPhase::player_post_diplomacy_ai,
    BnwTurnPhase::player_golden_age_and_gifts,
    BnwTurnPhase::player_cities,
    BnwTurnPhase::player_gold,
    BnwTurnPhase::player_culture,
    BnwTurnPhase::player_policies,
    BnwTurnPhase::player_research,
    BnwTurnPhase::player_espionage,
    BnwTurnPhase::player_faith,
    BnwTurnPhase::player_leagues,
    BnwTurnPhase::player_anarchy_units_replay,
    BnwTurnPhase::player_ai_post,
    BnwTurnPhase::player_lua,
    BnwTurnPhase::player_achievements,
};
// CvPlayerAchievements::StartTurn only caches achievement Info IDs and may call
// gDLL->UnlockAchievement. Steam achievements are outside the standalone BNW
// milestone, and that external call must never run inside a rollbackable turn.
constexpr bool external_milestone_phase(BnwTurnPhase phase) {
    return phase==BnwTurnPhase::player_achievements;
}
constexpr std::array<std::string_view,game_order.size()+player_order.size()> names{
    "game_engine_begin","game_barbarians_begin","game_caches_and_score",
    "game_deals","game_teams","game_map","game_interface_turn",
    "game_barbarian_camps","game_barbarian_units","game_religion","game_trade",
    "game_leagues","game_culture","game_clock","game_activation","game_victory",
    "game_publish","player_cache","player_ai_pre","player_timers_and_revolts",
    "player_trade_strategy_diplomacy","player_post_diplomacy_ai",
    "player_golden_age_and_gifts","player_cities","player_gold","player_culture",
    "player_policies","player_research","player_espionage","player_faith",
    "player_leagues","player_anarchy_units_replay","player_ai_post","player_lua",
    "player_achievements"};
static_assert(static_cast<std::size_t>(BnwTurnPhase::player_achievements)+1==names.size());

BnwTurnPreflight preflight(std::span<const BnwTurnPhase> order,
                           std::span<const BnwTurnHandler> handlers) {
    BnwTurnPreflight result;
    std::set<BnwTurnPhase> expected;
    for(const auto phase:order)
        if(!external_milestone_phase(phase)) expected.insert(phase);
    std::map<BnwTurnPhase,std::size_t> counts;
    for(const auto &handler:handlers) {
        if(!expected.contains(handler.phase)) {
            result.unexpected.push_back(handler.phase);
            continue;
        }
        ++counts[handler.phase];
        if(!handler.run) result.unbound.push_back(handler.phase);
    }
    for(const auto phase:order) {
        if(external_milestone_phase(phase)) continue;
        const auto count=counts[phase];
        if(count==0) result.missing.push_back(phase);
        else if(count>1) result.duplicates.push_back(phase);
    }
    return result;
}

std::string failed_preflight(const BnwTurnPreflight &result) {
    const auto describe=[](std::string_view label,const auto &values)->std::string {
        if(values.empty()) return {};
        return std::string(label)+": "+std::string(bnw_turn_phase_name(values.front()));
    };
    for(const auto &entry:{describe("missing",result.missing),
                           describe("duplicate",result.duplicates),
                           describe("unbound",result.unbound),
                           describe("unexpected",result.unexpected)})
        if(!entry.empty()) return "BNW turn preflight "+entry;
    return "BNW turn preflight failed";
}

LiveActionResult run(LiveSession &session,std::int64_t player,
                     std::span<const BnwTurnPhase> order,
                     std::span<const BnwTurnHandler> handlers) {
    const auto checked=preflight(order,handlers);
    if(!checked.ready()) return {LiveActionOutcome::rejected,failed_preflight(checked)};
    if(player!=-1) {
        const auto found=session.state().setup.players.find(player);
        if(found==session.state().setup.players.end() || !found->second.alive)
            return {LiveActionOutcome::rejected,"BNW player turn requires a living player"};
    }
    auto result=apply_live_action(session,[&](LiveSessionState &state) {
        for(const auto phase:order) {
            if(external_milestone_phase(phase)) continue;
            const auto found=std::find_if(handlers.begin(),handlers.end(),
                [&](const BnwTurnHandler &entry){return entry.phase==phase;});
            auto result=found->run(state,player);
            if(result.outcome==LiveActionOutcome::rejected) {
                result.reason="BNW phase "+std::string(bnw_turn_phase_name(phase))+": "+result.reason;
                return result;
            }
        }
        return LiveActionResult{LiveActionOutcome::committed,{}};
    });
    if(result.outcome==LiveActionOutcome::rejected) return result;
    std::string publication;
    for(const auto phase:order) {
        if(external_milestone_phase(phase)) continue;
        const auto found=std::find_if(handlers.begin(),handlers.end(),
            [&](const BnwTurnHandler &entry){return entry.phase==phase;});
        if(!found->after_commit) continue;
        std::string detail;
        bool publication_failed=false;
        try {
            const auto delivered=found->after_commit(session);
            if(delivered.outcome==LiveActionOutcome::rejected) {
                detail="publication blocked: "+delivered.reason;
                publication_failed=true;
            } else detail=delivered.reason;
        } catch(const std::exception &error) {
            detail=std::string("publication uncertain: ")+error.what();
            publication_failed=true;
        } catch(...) {
            detail="publication uncertain";
            publication_failed=true;
        }
        if(detail.empty()) continue;
        if(!publication.empty()) publication+="; ";
        publication+=std::string(bnw_turn_phase_name(phase))+": "+detail;
        // Later receivers can depend on this external effect. The owned turn
        // is already committed, so neither rollback nor continued fanout is safe.
        if(publication_failed) break;
    }
    result.reason=std::move(publication);
    return result;
}
} // namespace

std::string_view bnw_turn_phase_name(BnwTurnPhase phase) {
    const auto index=static_cast<std::size_t>(phase);
    return index<names.size()?names[index]:"unknown";
}
std::span<const BnwTurnPhase> bnw_game_turn_order() { return game_order; }
std::span<const BnwTurnPhase> bnw_player_turn_order() { return player_order; }
BnwTurnPreflight preflight_bnw_game_turn(std::span<const BnwTurnHandler> handlers) {
    return preflight(game_order,handlers);
}
BnwTurnPreflight preflight_bnw_player_turn(std::span<const BnwTurnHandler> handlers) {
    return preflight(player_order,handlers);
}
LiveActionResult run_bnw_game_turn(LiveSession &session,std::span<const BnwTurnHandler> handlers) {
    return run(session,-1,game_order,handlers);
}
LiveActionResult run_bnw_player_turn(LiveSession &session,std::int64_t player,
                                     std::span<const BnwTurnHandler> handlers) {
    return run(session,player,player_order,handlers);
}

} // namespace civ5::runtime
