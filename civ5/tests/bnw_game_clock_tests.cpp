#include "civ5/runtime/bnw_game_clock.hpp"
#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool value,const char *message) {
    if (!value) throw std::runtime_error(message);
}

civ5::runtime::LiveSession make_session() {
    using namespace civ5;
    saves::SavedSession source;
    source.snapshot.game_state.layout=saves::GameStateLayout::expansion2_version1;
    source.snapshot.pregame.emplace();
    source.snapshot.pregame->layout=saves::GameStateLayout::expansion2_version1;
    source.snapshot.pregame->archive.integers["game_turn"]=10;
    auto &prefix=std::get<saves::Expansion2PrefixObservation>(
        source.snapshot.game_state.prefix);
    prefix.scalars[1]=3;
    source.snapshot.game_state.flags.push_back({"score_dirty",false});
    runtime::LiveSessionState state{
        source.snapshot,runtime::ResearchContext{}, {},runtime::PlayerResearchContext{},
        runtime::CityYieldContext{},runtime::BnwUnhappinessContext{},
        runtime::MaintenanceContext{},runtime::TreasuryContext{},
        runtime::ScienceContext{},runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})}};
    state.setup.game_turn=10;
    state.maintenance.elapsed_turns=3;
    state.game_clock.sent_auto_moves=true;
    return runtime::LiveSession::from_restored_bnw(std::move(source),std::move(state));
}
} // namespace

int main() {
    try {
        using namespace civ5::runtime;
        auto game=make_session();
        const auto checkpoint=game.checkpoint();
        const auto rng=game.state().random.gameplay.state().calls;
        std::vector<std::string> events;
        bool fail_game_data=false;
        const auto committed_before_callback=[&] {
            check(game.state().setup.game_turn==11 &&
                  game.state().world.pregame->archive.integers.at("game_turn")==11 &&
                  game.state().maintenance.elapsed_turns==4 &&
                  std::get<civ5::saves::Expansion2PrefixObservation>(
                      game.state().world.game_state.prefix).scalars[1]==4,
                  "Engine callback ran before the owned clock committed");
        };
        BnwGameClockServices services;
        services.net_debug_log=[&](std::string_view message) {
            committed_before_callback();events.emplace_back(message);
        };
        services.turn_timer_dirty=[&] {
            committed_before_callback();events.emplace_back("turn_timer");
        };
        services.game_data_dirty=[&] {
            committed_before_callback();
            if (fail_game_data) throw std::runtime_error("engine rejected dirty bit");
            events.emplace_back("game_data");
        };
        services.gameplay_turn_changed=[&](std::int64_t turn) {
            committed_before_callback();events.emplace_back("turn_changed:"+std::to_string(turn));
        };
        services.end_turn_timer_reset=[&] {
            committed_before_callback();events.emplace_back("timer_reset");
        };
        auto missing=services;missing.game_data_dirty={};
        const auto no_engine=advance_bnw_game_clock(game,missing);
        check(no_engine.outcome==LiveActionOutcome::rejected &&
              game.state().setup.game_turn==10 && events.empty(),
              "Missing mandatory engine callback advanced the clock");

        auto factory=make_bnw_game_clock_handler(services);
        check(factory.phase==BnwTurnPhase::game_clock && factory.run &&
              factory.after_commit,
              "Factory did not bind the game-clock phase");
        const auto rolled_back=apply_live_action(game,[&](LiveSessionState &detached) {
            const auto phase=factory.run(detached,-1);
            check(phase.outcome==LiveActionOutcome::committed &&
                  detached.game_clock.pending.has_value(),
                  "Phase did not stage a turn publication");
            return LiveActionResult{LiveActionOutcome::rejected,"later phase failed"};
        });
        check(rolled_back.outcome==LiveActionOutcome::rejected &&
              game.state().setup.game_turn==10 && !game.state().game_clock.pending &&
              events.empty() && game.state().random.gameplay.state().calls==rng,
              "Outer turn rollback leaked clock or external callbacks");

        std::vector<BnwTurnHandler> other_phases;
        for (const auto phase:bnw_game_turn_order()) {
            if (phase==BnwTurnPhase::game_clock) continue;
            other_phases.push_back({phase,[](LiveSessionState &,std::int64_t) {
                return LiveActionResult{LiveActionOutcome::committed,{}};
            }});
        }
        auto incomplete=other_phases;
        incomplete.pop_back();
        check(run_bnw_game_turn_with_clock(game,incomplete,services).outcome==
                  LiveActionOutcome::rejected && events.empty() &&
                  game.state().setup.game_turn==10,
              "Clock wrapper accepted a missing gameplay phase");
        auto rejected_later=other_phases;
        rejected_later.back().run=[](LiveSessionState &state,std::int64_t) {
            (void)state.random.gameplay.draw(10);
            return LiveActionResult{LiveActionOutcome::rejected,"publish failed"};
        };
        check(run_bnw_game_turn_with_clock(game,rejected_later,services).outcome==
                  LiveActionOutcome::rejected && events.empty() &&
                  game.state().setup.game_turn==10 &&
                  game.state().random.gameplay.state().calls==rng,
              "A later game phase published or retained the staged clock");
        check(run_bnw_game_turn_with_clock(game,other_phases,services).outcome==
                  LiveActionOutcome::committed &&
                  game.state().setup.game_turn==11 && events.size()==5 &&
                  !game.state().game_clock.pending,
              "Complete game phase set did not publish the clock after commit");
        game.restore(checkpoint);events.clear();
        auto direct=other_phases;
        direct.push_back(factory);
        check(run_bnw_game_turn(game,direct).outcome==
                  LiveActionOutcome::committed && events.size()==5 &&
              game.state().setup.game_turn==11 &&
              !game.state().game_clock.pending,
              "Direct BNW scheduler did not publish bound clock effects");
        game.restore(checkpoint);events.clear();

        auto reentrant_services=services;
        reentrant_services.net_debug_log=[&](std::string_view message) {
            committed_before_callback();
            check(game.state().game_clock.pending &&
                  game.state().game_clock.pending->uncertain &&
                  publish_bnw_game_clock(game,reentrant_services).outcome==
                      LiveActionOutcome::rejected,
                  "Reentrant clock publication replayed an in-flight step");
            events.emplace_back(message);
        };
        check(advance_bnw_game_clock(game,reentrant_services).outcome==
                  LiveActionOutcome::committed && events.size()==5 &&
              !game.state().game_clock.pending,
              "Clock publication did not complete once per step after reentry");
        game.restore(checkpoint);events.clear();

        game.mutable_state().world.pregame->archive.integers["game_turn"]=9;
        const auto mismatch=advance_bnw_game_clock(game,services);
        check(mismatch.outcome==LiveActionOutcome::rejected &&
              game.state().setup.game_turn==10 && events.empty(),
              "Divergent decoded turn must block before mutation");
        game.restore(checkpoint);

        const auto advanced=advance_bnw_game_clock(game,services);
        check(advanced.outcome==LiveActionOutcome::committed &&
              events==std::vector<std::string>{"Game Turn 11\n","turn_timer",
                  "game_data","turn_changed:11","timer_reset"} &&
              game.state().world.game_state.flags.front().value &&
              !game.state().game_clock.sent_auto_moves &&
              !game.state().game_clock.pending &&
              game.state().random.gameplay.state().calls==rng &&
              !game.readiness().can_continue(),
              "Clock failed to synchronize owners and publish SDK effects in order");

        const SessionCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
        const auto saved_clock=make_bnw_game_clock_checkpoint(
            game.state(),game.source().snapshot,identity);
        auto restored_world=game.source().snapshot;
        BnwGameClockState restored_clock;
        restore_bnw_game_clock_checkpoint(saved_clock,restored_world,restored_clock,identity);
        check(restored_world.pregame->archive.integers.at("game_turn")==11 &&
              std::get<civ5::saves::Expansion2PrefixObservation>(
                  restored_world.game_state.prefix).scalars[1]==4 &&
              restored_world.game_state.flags.front().value &&
              !restored_clock.sent_auto_moves && !restored_clock.pending,
              "BNW clock checkpoint did not restore all clock mirrors");
        auto wrong_identity=identity;
        wrong_identity.rules_sha256=std::string(64,'c');
        bool rejected=false;
        try { restore_bnw_game_clock_checkpoint(saved_clock,restored_world,restored_clock,wrong_identity); }
        catch(const std::runtime_error &) { rejected=true; }
        check(rejected && restored_world.pregame->archive.integers.at("game_turn")==11,
              "BNW clock checkpoint accepted different active rules");
        auto malformed=saved_clock;
        malformed.bytes.back()=2;
        restored_world=game.source().snapshot;
        rejected=false;
        try { restore_bnw_game_clock_checkpoint(malformed,restored_world,restored_clock,identity); }
        catch(const std::runtime_error &) { rejected=true; }
        check(rejected && restored_world.pregame->archive.integers.at("game_turn")==10,
              "Invalid BNW clock checkpoint partially changed the source world");

        game.restore(checkpoint);events.clear();fail_game_data=true;
        const auto uncertain=advance_bnw_game_clock(game,services);
        check(uncertain.outcome==LiveActionOutcome::committed &&
              uncertain.reason.find("uncertain")!=std::string::npos &&
              events==std::vector<std::string>{"Game Turn 11\n","turn_timer"} &&
              game.state().game_clock.pending &&
              game.state().game_clock.pending->next_step==2 &&
              game.state().game_clock.pending->uncertain,
              "Throwing engine callback must leave committed clock with uncertain publication");
        rejected=false;
        try { (void)make_bnw_game_clock_checkpoint(game.state(),game.source().snapshot,identity); }
        catch(const std::runtime_error &) { rejected=true; }
        check(rejected,"Unpublished BNW clock change was serialized");
        const auto duplicate=publish_bnw_game_clock(game,services);
        check(duplicate.outcome==LiveActionOutcome::rejected && events.size()==2,
              "Uncertain publication must not replay an external effect");
        const auto next_tick=advance_bnw_game_clock(game,services);
        check(next_tick.outcome==LiveActionOutcome::rejected &&
              game.state().setup.game_turn==11,
              "A pending publication must block another clock tick");

        game.restore(checkpoint);events.clear();fail_game_data=false;
        game.mutable_state().setup.game_turn=std::numeric_limits<std::int64_t>::max();
        game.mutable_state().world.pregame->archive.integers["game_turn"]=
            std::numeric_limits<std::int64_t>::max();
        const auto overflow=advance_bnw_game_clock(game,services);
        check(overflow.outcome==LiveActionOutcome::rejected &&
              game.state().maintenance.elapsed_turns==3 && events.empty(),
              "Clock overflow must reject without changing elapsed turns");
        std::cout << "PASS BNW game clock\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW game clock: " << error.what() << '\n';
        return 1;
    }
}
