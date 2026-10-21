#include "civ5/runtime/bnw_military_defense.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool valid,const char *message) {
    if (!valid) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    try { action(); }
    catch (const std::exception &) { return; }
    throw std::runtime_error("Missing BNW military defense blocker");
}
LiveSessionState world() {
    LiveSessionState state{
        saves::SavedSessionSnapshot{},ResearchContext{}, {},PlayerResearchContext{},
        CityYieldContext{},BnwUnhappinessContext{},MaintenanceContext{},
        TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{0,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{0,0,0})},
        ResearchSelectionState{},BnwGameClockState{},std::nullopt,std::nullopt};
    state.world.game_state.layout=saves::GameStateLayout::expansion2_version1;
    auto &all=state.world.all_players.emplace();
    all.layout=saves::GameStateLayout::expansion2_version1;
    all.players.emplace_back().saved_player_id=5;
    return state;
}
} // namespace

int main() {
    try {
        // SDK Expansion2 CvMilitaryAI.cpp:2199, UpdateDefenseState: land
        // comparisons are strict; navy comparisons are inclusive and ordered.
        const std::array<std::pair<int,int>,6> land{{
            {9,3},{10,2},{19,2},{20,1},{24,1},{25,0}}};
        for (const auto [count,expected]:land) {
            const auto actual=derive_bnw_military_defense(
                {count,5,10,20,8});
            check(actual.land==expected,"BNW land defense boundary differs from SDK");
        }
        const std::array<std::pair<int,int>,5> navy{{
            {4,3},{5,2},{8,2},{10,1},{11,0}}};
        for (const auto [count,expected]:navy) {
            const auto actual=derive_bnw_military_defense(
                {10,count,10,20,8});
            check(actual.naval==expected,"BNW naval defense boundary differs from SDK");
        }
        rejects([]{ (void)derive_bnw_military_defense({1,1,1,1,2147483647}); });
        rejects([]{ (void)derive_bnw_military_defense({-1,1,1,1,1}); });
        auto state=world();
        BnwMilitaryDefenseSource input{5,true,true,BnwMilitaryDefenseBase{10,5,10,20,8}};
        const auto rng_before=state.random;
        const auto result=bind_bnw_military_defense(state,5,input);
        check(result.land==2 && result.naval==2,
              "Current BNW AI data did not produce defense states");
        input.base_data_current=false;
        rejects([&]{(void)bind_bnw_military_defense(state,5,input);});
        input.base_data_current=true;input.navy_recommendation_current=false;
        rejects([&]{(void)bind_bnw_military_defense(state,5,input);});
        input.navy_recommendation_current=true;input.player=6;
        rejects([&]{(void)bind_bnw_military_defense(state,5,input);});
        input.player=5;
        rejects([&]{(void)bind_bnw_military_defense(state,6,input);});
        state.world.game_state.layout=saves::GameStateLayout::vp542_corpus;
        rejects([&]{(void)bind_bnw_military_defense(state,5,input);});
        check(state.random.gameplay.state()==rng_before.gameplay.state() &&
              state.random.map.state()==rng_before.map.state(),
              "Read-only AI binding advanced session RNG");
        std::cout << "PASS BNW military defense classification and binding\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW military defense: " << error.what() << '\n';
        return 1;
    }
}
