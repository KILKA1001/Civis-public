#include "civ5/runtime/live_action_transaction.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

civ5::runtime::LiveSession session() {
    using namespace civ5;
    saves::SavedSession source;
    source.snapshot.game_state.layout=saves::GameStateLayout::expansion2_version1;
    runtime::LiveSessionState state{
        source.snapshot, runtime::ResearchContext{}, {}, runtime::PlayerResearchContext{},
        runtime::CityYieldContext{}, runtime::BnwUnhappinessContext{},
        runtime::MaintenanceContext{}, runtime::TreasuryContext{},
        runtime::ScienceContext{}, runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})}};
    return runtime::LiveSession::from_restored_bnw(std::move(source),std::move(state));
}
} // namespace

int main() {
    try {
        using namespace civ5::runtime;
        auto game=session();
        auto &derived=game.mutable_state();
        derived.notification_load_transients.current=true;
        derived.player_plot_lists.current=true;
        derived.player_read_tail.current=true;
        derived.game_plot_extras.current=true;
        const auto before_calls=game.state().random.gameplay.state().calls;
        const auto rejected=apply_live_action(game,[](LiveSessionState &state) {
            state.setup.game_turn=9;
            (void)state.random.gameplay.draw(10);
            return LiveActionResult{LiveActionOutcome::rejected,"Mandatory effect unavailable"};
        });
        check(rejected.outcome==LiveActionOutcome::rejected &&
              game.state().setup.game_turn==0 &&
              game.state().random.gameplay.state().calls==before_calls &&
              game.state().notification_load_transients.current &&
              game.state().player_plot_lists.current &&
              game.state().player_read_tail.current &&
              game.state().game_plot_extras.current,
              "Rejected action changed world or consumed RNG");
        bool threw=false;
        try {
            (void)apply_live_action(game,[](LiveSessionState &state) -> LiveActionResult {
                state.setup.game_turn=12;
                (void)state.random.gameplay.draw(10);
                throw std::runtime_error("Required Lua callback failed");
            });
        } catch(const std::runtime_error &) { threw=true; }
        check(threw && game.state().setup.game_turn==0 &&
              game.state().random.gameplay.state().calls==before_calls,
              "Throwing action did not roll back the world and RNG");
        const auto before_generation=game.state_generation();
        const auto committed=apply_live_action(game,[](LiveSessionState &state) {
            state.setup.game_turn=1;
            (void)state.random.gameplay.draw(10);
            return LiveActionResult{LiveActionOutcome::committed,{}};
        });
        check(game.state_generation()==before_generation+1,"Committed action did not advance state generation");
        check(committed.outcome==LiveActionOutcome::committed &&
              game.state().setup.game_turn==1 &&
              game.state().random.gameplay.state().calls==before_calls+1 &&
              !game.state().notification_load_transients.current &&
              !game.state().player_plot_lists.current &&
              !game.state().player_read_tail.current &&
              !game.state().game_plot_extras.current &&
              !game.readiness().can_continue(),
              "Committed action did not retain state or advertised playable continuation");
        threw=false;
        try { (void)apply_live_action(game,{}); }
        catch(const std::invalid_argument &) { threw=true; }
        check(threw && game.state().setup.game_turn==1,
              "Missing action callback changed the committed state");
        std::cout << "PASS live action transaction\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL live action transaction: " << error.what() << '\n';
        return 1;
    }
}
