#include "civ5/runtime/bnw_turn_scheduler.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace civ5::runtime;
void check(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
LiveSession session() {
    civ5::saves::SavedSession source;
    source.snapshot.game_state.layout=civ5::saves::GameStateLayout::expansion2_version1;
    LiveSessionState state{source.snapshot,ResearchContext{}, {},PlayerResearchContext{},
        CityYieldContext{},BnwUnhappinessContext{},MaintenanceContext{},TreasuryContext{},
        ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{5,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{7,0,0})}};
    state.setup.players[0].alive=true;
    return LiveSession::from_restored_bnw(std::move(source),std::move(state));
}
std::vector<BnwTurnHandler> test_handlers(std::span<const BnwTurnPhase> order,
                                            std::vector<BnwTurnPhase> &trace) {
    // Test instrumentation only. Production supplies actual GameCore handlers;
    // the scheduler never installs these or any successful defaults.
    std::vector<BnwTurnHandler> result;
    for(const auto phase:order) {
        if(phase==BnwTurnPhase::player_achievements) continue;
        result.push_back({phase,[&trace,phase](LiveSessionState &state,std::int64_t) {
        trace.push_back(phase);
        (void)state.random.gameplay.draw(10);
        if(phase==BnwTurnPhase::game_clock) ++state.setup.game_turn;
        if(phase==BnwTurnPhase::player_cities)
            state.cities.players[0].cities[1].population=8;
        return LiveActionResult{LiveActionOutcome::committed,{}};
        }});
    }
    return result;
}
bool same(std::span<const BnwTurnPhase> actual,const std::vector<BnwTurnPhase> &expected) {
    return actual.size()==expected.size() &&
           std::equal(actual.begin(),actual.end(),expected.begin());
}
} // namespace

int main() {
    try {
        // Independently transcribed from installed Expansion2 SDK:
        // CvGame.cpp:7480 CvGame::doTurn and CvPlayer.cpp:4038/4121.
        const std::vector<BnwTurnPhase> expected_game{
            BnwTurnPhase::game_engine_begin,BnwTurnPhase::game_barbarians_begin,
            BnwTurnPhase::game_caches_and_score,BnwTurnPhase::game_deals,
            BnwTurnPhase::game_teams,BnwTurnPhase::game_map,
            BnwTurnPhase::game_interface_turn,BnwTurnPhase::game_barbarian_camps,
            BnwTurnPhase::game_barbarian_units,BnwTurnPhase::game_religion,
            BnwTurnPhase::game_trade,BnwTurnPhase::game_leagues,
            BnwTurnPhase::game_culture,BnwTurnPhase::game_clock,
            BnwTurnPhase::game_activation,BnwTurnPhase::game_victory,
            BnwTurnPhase::game_publish};
        const std::vector<BnwTurnPhase> expected_player{
            BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre,
            BnwTurnPhase::player_timers_and_revolts,
            BnwTurnPhase::player_trade_strategy_diplomacy,
            BnwTurnPhase::player_post_diplomacy_ai,
            BnwTurnPhase::player_golden_age_and_gifts,
            BnwTurnPhase::player_cities,BnwTurnPhase::player_gold,
            BnwTurnPhase::player_culture,BnwTurnPhase::player_policies,
            BnwTurnPhase::player_research,BnwTurnPhase::player_espionage,
            BnwTurnPhase::player_faith,BnwTurnPhase::player_leagues,
            BnwTurnPhase::player_anarchy_units_replay,
            BnwTurnPhase::player_ai_post,BnwTurnPhase::player_lua,
            BnwTurnPhase::player_achievements};
        check(same(bnw_game_turn_order(),expected_game) &&
              same(bnw_player_turn_order(),expected_player),
              "BNW SDK source phase order changed");
        auto game=session();
        std::vector<BnwTurnPhase> trace;
        auto handlers=test_handlers(bnw_game_turn_order(),trace);
        auto missing=handlers;missing.pop_back();
        check(preflight_bnw_game_turn(missing).missing==
                  std::vector<BnwTurnPhase>{BnwTurnPhase::game_publish},
              "Preflight missed a required game phase");
        auto outcome=run_bnw_game_turn(game,missing);
        check(outcome.outcome==LiveActionOutcome::rejected && trace.empty() &&
              game.state().setup.game_turn==0 && game.state().random.gameplay.state().calls==0,
              "Missing game phase mutated state before preflight");
        auto duplicate=handlers;duplicate.push_back(handlers.front());
        check(!preflight_bnw_game_turn(duplicate).duplicates.empty() &&
              run_bnw_game_turn(game,duplicate).outcome==LiveActionOutcome::rejected && trace.empty(),
              "Duplicate game phase passed preflight");
        auto unexpected=handlers;unexpected.push_back({BnwTurnPhase::player_gold,handlers.front().run});
        check(!preflight_bnw_game_turn(unexpected).unexpected.empty() &&
              run_bnw_game_turn(game,unexpected).outcome==LiveActionOutcome::rejected && trace.empty(),
              "Unexpected player phase passed game preflight");
        auto unbound=handlers;unbound[0].run={};
        check(!preflight_bnw_game_turn(unbound).unbound.empty() &&
              run_bnw_game_turn(game,unbound).outcome==LiveActionOutcome::rejected && trace.empty(),
              "Unbound game phase passed preflight");
        check(run_bnw_game_turn(game,handlers).outcome==LiveActionOutcome::committed &&
              trace==expected_game && game.state().setup.game_turn==1 &&
              game.state().random.gameplay.state().calls==expected_game.size() &&
              !game.readiness().can_continue(),
              "Game phases did not run in source order or incorrectly advertised continuation");
        trace.clear();
        auto rejecting=handlers;
        rejecting[10].run=[](LiveSessionState &state,std::int64_t) {
            state.setup.game_turn=99;
            (void)state.random.gameplay.draw(10);
            return LiveActionResult{LiveActionOutcome::rejected,"trade unavailable"};
        };
        outcome=run_bnw_game_turn(game,rejecting);
        check(outcome.outcome==LiveActionOutcome::rejected &&
              outcome.reason.find("game_trade")!=std::string::npos &&
              game.state().setup.game_turn==1 &&
              game.state().random.gameplay.state().calls==expected_game.size(),
              "Rejected middle game phase did not roll back clock and RNG");
        trace.clear();
        auto player_handlers=test_handlers(bnw_player_turn_order(),trace);
        check(preflight_bnw_player_turn(player_handlers).ready(),
              "Steam achievement phase incorrectly blocks standalone BNW turns");
        auto external=player_handlers;
        external.push_back({BnwTurnPhase::player_achievements,handlers.front().run});
        check(!preflight_bnw_player_turn(external).unexpected.empty() &&
              run_bnw_player_turn(game,0,external).outcome==LiveActionOutcome::rejected,
              "External Steam effect entered rollbackable BNW turn");
        check(run_bnw_player_turn(game,42,player_handlers).outcome==LiveActionOutcome::rejected &&
              trace.empty(),"Unknown player started a player turn");
        auto throwing=player_handlers;
        throwing[7].run=[](LiveSessionState &state,std::int64_t) -> LiveActionResult {
            state.treasury.players[0].balance_times100=1000;
            (void)state.random.gameplay.draw(10);
            throw std::runtime_error("DoGold host failed");
        };
        bool threw=false;
        try { (void)run_bnw_player_turn(game,0,throwing); }
        catch(const std::runtime_error &) { threw=true; }
        check(threw && game.state().cities.players.empty() &&
              game.state().treasury.players.empty() &&
              game.state().random.gameplay.state().calls==expected_game.size(),
              "Throwing player gold phase did not roll back city, treasury and RNG");
        trace.clear();
        auto gameplay_player_order=expected_player;
        gameplay_player_order.pop_back();
        check(run_bnw_player_turn(game,0,player_handlers).outcome==LiveActionOutcome::committed &&
              trace==gameplay_player_order && game.state().cities.players.at(0).cities.at(1).population==8 &&
              game.state().random.gameplay.state().calls==expected_game.size()+gameplay_player_order.size() &&
              !game.readiness().can_continue(),
              "Player phases did not run in source order or incorrectly advertised continuation");
        // These are scheduler receiver probes, not fabricated GameCore services.
        // A receiver can perform an irreversible effect before refusing or throwing.
        // Later receivers must not execute; committed state/RNG must not roll back.
        for(int failure_kind=0;failure_kind!=3;++failure_kind) {
            auto publishing=session();
            std::vector<BnwTurnPhase> mutation_trace,publication_trace;
            auto publishing_handlers=test_handlers(bnw_game_turn_order(),mutation_trace);
            int irreversible_effects=0;
            publishing_handlers[0].after_commit=[&](LiveSession &current) {
                check(current.state().setup.game_turn==1,
                      "Publication ran before the owned turn committed");
                publication_trace.push_back(BnwTurnPhase::game_engine_begin);
                return LiveActionResult{LiveActionOutcome::committed,"receiver note"};
            };
            publishing_handlers[1].after_commit=[&](LiveSession &) -> LiveActionResult {
                publication_trace.push_back(BnwTurnPhase::game_barbarians_begin);
                ++irreversible_effects;
                if(failure_kind==0)
                    return {LiveActionOutcome::rejected,"receiver unavailable"};
                if(failure_kind==1) throw std::runtime_error("receiver failed after effect");
                throw 17;
            };
            publishing_handlers[2].after_commit=[&](LiveSession &) {
                publication_trace.push_back(BnwTurnPhase::game_caches_and_score);
                return LiveActionResult{LiveActionOutcome::committed,{}};
            };
            const auto published=run_bnw_game_turn(publishing,publishing_handlers);
            check(published.outcome==LiveActionOutcome::committed &&
                  mutation_trace==expected_game && irreversible_effects==1 &&
                  publication_trace==std::vector<BnwTurnPhase>{
                      BnwTurnPhase::game_engine_begin,BnwTurnPhase::game_barbarians_begin} &&
                  publishing.state().setup.game_turn==1 &&
                  publishing.state().random.gameplay.state().calls==expected_game.size() &&
                  !publishing.readiness().can_continue(),
                  "Failed publication continued fanout or rolled back committed effects");
            check(published.reason.find("receiver note")!=std::string::npos &&
                  published.reason.find("game_barbarians_begin: publication ")!=std::string::npos &&
                  published.reason.find(failure_kind==0?"blocked":"uncertain")!=std::string::npos &&
                  published.reason.find("game_caches_and_score")==std::string::npos,
                  "Publication failure diagnostics lost the actual stopping phase");
        }
        // A successful informational reason must not be mistaken for failure.
        {
            auto publishing=session();
            std::vector<BnwTurnPhase> mutation_trace,publication_trace;
            auto publishing_handlers=test_handlers(bnw_player_turn_order(),mutation_trace);
            for(auto &entry:publishing_handlers) {
                const auto phase=entry.phase;
                entry.after_commit=[&,phase](LiveSession &) {
                    publication_trace.push_back(phase);
                    return LiveActionResult{LiveActionOutcome::committed,
                        phase==BnwTurnPhase::player_cache?"receiver note":""};
                };
            }
            const auto published=run_bnw_player_turn(publishing,0,publishing_handlers);
            check(published.outcome==LiveActionOutcome::committed &&
                  publication_trace==gameplay_player_order && mutation_trace==gameplay_player_order &&
                  published.reason=="player_cache: receiver note" &&
                  !publishing.readiness().can_continue(),
                  "Successful publication note stopped later receivers");
        }
        std::cout << "PASS BNW turn scheduler\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW turn scheduler: " << error.what() << '\n';
        return 1;
    }
}
