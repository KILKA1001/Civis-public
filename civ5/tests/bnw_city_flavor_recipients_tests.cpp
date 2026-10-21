#include "civ5/runtime/bnw_city_flavor_recipients.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/live_action_transaction.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    try {action();} catch(const std::exception &) {return;}
    throw std::runtime_error("Missing BNW city flavor load blocker");
}
std::int64_t scalar(const saves::CityCoreObservation &core,
                    std::string_view name) {
    for(const auto &field:core.fields)
        if(field.name && std::string_view(field.name)==name &&
           field.values.size()==1) return field.values.front();
    throw std::runtime_error("Selected BNW city identity unavailable");
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
    state.setup.profile=content::TechnologyProfile::bnw;
    state.cities.profile=content::TechnologyProfile::bnw;
    check(state.world.game_state.layout==bnw && state.world.all_players,
          "Selected save is not a complete BNW player/city source");
    for(const auto &saved:state.world.all_players->players) {
        auto &live=state.cities.players[saved.saved_player_id];
        for(const auto &city:saved.cities.cities) {
            const auto id=scalar(city.core,"m_iID");
            auto &entry=live.cities[id];
            entry.owner=scalar(city.core,"m_eOwner");
            entry.id=id;
            entry.source_core=std::pair{city.core.start_offset,
                                        city.core.end_offset};
        }
    }
    return state;
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply the selected original BNW save path");
        auto state=selected_state(std::filesystem::path(argv[1]));
        std::size_t city_count=0;
        for(const auto &player:state.world.all_players->players)
            city_count+=player.cities.cities.size();
        check(city_count>0,"Selected BNW save lacks a city for the hook");
        rebuild_bnw_city_flavor_recipients(state);
        check(state.city_flavor_recipients.current,
              "BNW city flavor registrations did not become current");
        std::size_t registered=0;
        for(const auto &player:state.world.all_players->players) {
            const auto &list=bnw_city_flavor_recipients(
                state,player.saved_player_id);
            check(list.size()==player.cities.cities.size(),
                  "CvCity::read recipient count differs from saved cities");
            for(std::size_t i=0;i<list.size();++i)
                check(list[i]==BnwCityHandle{player.saved_player_id,
                            scalar(player.cities.cities[i].core,"m_iID")},
                      "CvCity::read recipient source order changed");
            registered+=list.size();
        }
        check(registered==city_count,"BNW city strategy registration lost a city");
        const auto first_with_city=std::find_if(
            state.world.all_players->players.begin(),
            state.world.all_players->players.end(),
            [](const auto &p){return !p.cities.cities.empty();});
        check(first_with_city!=state.world.all_players->players.end(),
              "Selected save has no city owner");
        const auto id=scalar(first_with_city->cities.cities.front().core,"m_iID");
        const auto city_owner=first_with_city->saved_player_id;
        auto divergent=state;
        divergent.cities.players.at(city_owner).cities.at(id).id=-1;
        rejects([&]{(void)bnw_city_flavor_recipients(divergent,city_owner);});
        auto missing_strategy=state;
        missing_strategy.world.all_players->players.at(
            first_with_city-state.world.all_players->players.begin())
            .cities.cities.front().management.blocks.clear();
        rejects([&]{rebuild_bnw_city_flavor_recipients(missing_strategy);});
        check(!missing_strategy.city_flavor_recipients.current,
              "Failed strategy validation left a current registration");
        auto missing_manager=state;
        missing_manager.world.all_players->players.front().knowledge.blocks.clear();
        rejects([&]{rebuild_bnw_city_flavor_recipients(missing_manager);});
        auto vp=state;
        vp.world.game_state.layout=saves::GameStateLayout::vp542_corpus;
        rejects([&]{rebuild_bnw_city_flavor_recipients(vp);});
        saves::SavedSession source;
        source.snapshot=state.world;
        auto session=LiveSession::from_restored_bnw(
            std::move(source),std::move(state));
        const auto checkpoint=session.checkpoint();
        const auto declined=apply_live_action(session,[](LiveSessionState &) {
            return LiveActionResult{LiveActionOutcome::rejected,"declined"};
        });
        check(declined.outcome==LiveActionOutcome::rejected &&
              session.state().city_flavor_recipients.current,
              "Rejected action invalidated unchanged city registrations");
        (void)apply_live_action(session,[](LiveSessionState &) {
            return LiveActionResult{LiveActionOutcome::committed,{}};
        });
        check(!session.state().city_flavor_recipients.current,
              "Committed action retained potentially stale city registrations");
        session.restore(checkpoint);
        check(session.state().city_flavor_recipients.current &&
              !bnw_city_flavor_recipients(session.state(),city_owner).empty(),
              "Checkpoint did not restore the city registrations");
        (void)session.mutable_state();
        check(!session.state().city_flavor_recipients.current,
              "Arbitrary world edit retained city registrations");
        std::cout << "PASS BNW CvCity flavor recipient load hook: "
                  << registered << " selected-save cities\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW CvCity flavor recipient load hook: "
                  << error.what() << '\n';
        return 1;
    }
}
