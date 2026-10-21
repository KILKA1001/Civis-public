#include "civ5/runtime/bnw_player_flavor_recipients.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/live_action_transaction.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    try {action();} catch(const std::exception &) {return;}
    throw std::runtime_error("Missing BNW player flavor registration blocker");
}
LiveSessionState selected_state(const std::filesystem::path &path) {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;
    context.decode_teams=true;
    context.decode_all_players=true;
    context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto source=saves::read_saved_session(path,context);
    LiveSessionState state{
        saves::SavedSessionSnapshot{},ResearchContext{}, {},PlayerResearchContext{},
        CityYieldContext{},BnwUnhappinessContext{},MaintenanceContext{},
        TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{0,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{0,0,0})},
        ResearchSelectionState{},BnwGameClockState{},std::nullopt,std::nullopt,{}, {}, {}, {}, {}, {}};
    state.world=std::move(source.snapshot);
    check(state.world.game_state.layout==saves::GameStateLayout::expansion2_version1 &&
          state.world.pregame && state.world.all_players,
          "Selected save lacks a complete BNW player roster");
    const auto &status=state.world.pregame->archive.integer_arrays.at("slot_status");
    for(const auto &player:state.world.all_players->players)
        state.setup.players[player.saved_player_id].slot_status=
            status.at(static_cast<std::size_t>(player.saved_player_id));
    return state;
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply the selected original BNW save path");
        auto state=selected_state(std::filesystem::path(argv[1]));
        const auto &all=*state.world.all_players;
        const auto barbarian=all.context.strategy.civ_players;
        rebuild_bnw_player_flavor_recipients(state);
        std::size_t eligible=0;
        for(const auto &saved:all.players) {
            const auto id=saved.saved_player_id;
            const auto &recipients=bnw_player_flavor_recipients(state,id);
            const auto slot=state.setup.players.at(id).slot_status;
            if(id!=static_cast<std::int64_t>(barbarian) &&
               (slot==1 || slot==3)) {
                check(recipients==std::vector<BnwPlayerFlavorRecipient>{
                          BnwPlayerFlavorRecipient::technology_ai,
                          BnwPlayerFlavorRecipient::policy_ai,
                          BnwPlayerFlavorRecipient::wonder_production_ai},
                      "SDK player flavor registration order differs");
                ++eligible;
            } else check(recipients.empty(),
                         "Ineligible player received flavor targets");
        }
        check(eligible>0 && !bnw_player_flavor_recipients(state,0).empty(),
              "Selected Gandhi player was not registered");
        auto mismatched=state;
        mismatched.setup.players.at(0).slot_status=2;
        rejects([&]{(void)bnw_player_flavor_recipients(mismatched,0);});
        auto missing=state;
        missing.world.all_players->players.front().knowledge.blocks.clear();
        rejects([&]{rebuild_bnw_player_flavor_recipients(missing);});
        check(!missing.player_flavor_recipients.current,
              "Failed rebuild retained a current player registration");
        auto ineligible=state;
        ineligible.setup.players.at(0).slot_status=2;
        ineligible.world.pregame->archive.integer_arrays.at("slot_status")[0]=2;
        rebuild_bnw_player_flavor_recipients(ineligible);
        check(bnw_player_flavor_recipients(ineligible,0).empty(),
              "SS_CLOSED player received flavor targets");
        auto vp=state;
        vp.world.game_state.layout=saves::GameStateLayout::vp542_corpus;
        rejects([&]{rebuild_bnw_player_flavor_recipients(vp);});
        saves::SavedSession source;
        source.snapshot=state.world;
        auto session=LiveSession::from_restored_bnw(
            std::move(source),std::move(state));
        const auto checkpoint=session.checkpoint();
        const auto declined=apply_live_action(session,[](LiveSessionState &) {
            return LiveActionResult{LiveActionOutcome::rejected,"declined"};
        });
        check(declined.outcome==LiveActionOutcome::rejected &&
              session.state().player_flavor_recipients.current,
              "Rejected action invalidated player registrations");
        (void)apply_live_action(session,[](LiveSessionState &) {
            return LiveActionResult{LiveActionOutcome::committed,{}};
        });
        check(!session.state().player_flavor_recipients.current,
              "Committed action retained stale player registrations");
        session.restore(checkpoint);
        check(session.state().player_flavor_recipients.current &&
              bnw_player_flavor_recipients(session.state(),0).size()==3,
              "Checkpoint did not restore player registrations");
        (void)session.mutable_state();
        check(!session.state().player_flavor_recipients.current,
              "Arbitrary live edit retained player registrations");
        std::cout << "PASS BNW CvPlayer flavor load hook: "
                  << eligible << " selected-save players\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW CvPlayer flavor load hook: "
                  << error.what() << '\n';
        return 1;
    }
}
