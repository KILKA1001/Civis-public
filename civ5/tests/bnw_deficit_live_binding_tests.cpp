#include "civ5/runtime/bnw_deficit_live_binding.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool valid,const char *message) {
    if (!valid) throw std::runtime_error(message);
}
void core(saves::SavedUnitObservation &unit,const char *name,
          std::int64_t value) {
    saves::UnitCoreField field{};field.name=name;field.values={value};
    unit.core.fields.push_back(std::move(field));
}
void tail(saves::SavedUnitObservation &unit,const char *name,
          std::int64_t value) {
    if (unit.tail.blocks.empty()) {
        saves::UnitTailNode state{};state.name="state";
        unit.tail.blocks.push_back(std::move(state));
    }
    saves::UnitTailNode field{};field.name=name;field.values={value};
    unit.tail.blocks.front().children.push_back(std::move(field));
}
saves::SavedUnitObservation unit(std::int64_t id,std::int64_t x) {
    saves::SavedUnitObservation result;
    result.core.layout=result.tail.layout=bnw;
    core(result,"m_iID",id);core(result,"m_eOwner",5);
    core(result,"m_iX",x);core(result,"m_iY",0);
    tail(result,"m_combatUnit.eOwner",-1);
    tail(result,"m_combatUnit.iID",-1);
    return result;
}
void combat(saves::SavedUnitObservation &unit,const char *name,
            std::int64_t value) {
    for (auto &field:unit.tail.blocks.front().children)
        if (field.name && std::string_view(field.name)==name) {
            field.values={value};return;
        }
    throw std::runtime_error("Fixture combat handle missing");
}
LiveSessionState state() {
    LiveSessionState result{
        saves::SavedSessionSnapshot{},ResearchContext{}, {},PlayerResearchContext{},
        CityYieldContext{},BnwUnhappinessContext{},MaintenanceContext{},
        TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{0,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{0,0,0})},
        ResearchSelectionState{},BnwGameClockState{},std::nullopt,std::nullopt};
    result.world.game_state.layout=bnw;
    auto &all=result.world.all_players.emplace();all.layout=bnw;
    auto &player=all.players.emplace_back();
    player.saved_player_id=5;player.units.layout=bnw;
    player.units.units.push_back(unit(7,1));
    player.units.units.push_back(unit(8,2));
    auto &map=result.world.map_projection.emplace();
    map.grid.width=3;map.grid.height=1;
    for (std::uint64_t x=0;x<3;++x) {
        auto &cell=map.cells.emplace_back();
        cell.index=x;cell.x=x;cell.y=0;cell.units.emplace();
    }
    map.cells[1].units->push_back({5,7});
    map.cells[2].units->push_back({5,8});
    return result;
}
template<class F> void rejects(F run) {
    bool caught=false;
    try {run();} catch (const std::exception &) {caught=true;}
    check(caught,"Expected BNW live binding blocker");
}
} // namespace

int main() {
    try {
        // BNW MilitaryAI::Read omits both defense states. CvPlot.cpp:3861
        // scans plot units; CvUnit.cpp:9936 checks combat unit OR combat city.
        auto world=state();
        BnwDeficitLiveEvidence evidence;
        rejects([&]{(void)bind_bnw_plot_unit_fighting(world,0,0,evidence);});
        evidence.plot_unit_roster_current=true;
        check(!bind_bnw_plot_unit_fighting(world,0,0,evidence),
              "Current empty plot must have no fighting unit");
        rejects([&]{(void)bind_bnw_plot_unit_fighting(world,1,0,evidence);});
        evidence.combat_city_current[{5,7}]=false;
        check(!bind_bnw_plot_unit_fighting(world,1,0,evidence),
              "Known absent combat city and unit produced false fighting");
        evidence.combat_city_current[{5,7}]=true;
        check(bind_bnw_plot_unit_fighting(world,1,0,evidence),
              "Live combat-city link did not make plot fighting");
        evidence.combat_city_current.clear();
        auto &first=world.world.all_players->players.front().units.units.front();
        combat(first,"m_combatUnit.eOwner",5);
        combat(first,"m_combatUnit.iID",8);
        check(bind_bnw_plot_unit_fighting(world,1,0,evidence),
              "Resolved serialized combat-unit link did not prove fighting");
        auto stale=world;
        stale.world.map_projection->cells[1].units->clear();
        rejects([&]{(void)bind_bnw_plot_unit_fighting(stale,1,0,evidence);});
        auto unresolved=world;
        combat(unresolved.world.all_players->players.front().units.units.front(),
               "m_combatUnit.iID",99);
        rejects([&]{(void)bind_bnw_plot_unit_fighting(
            unresolved,1,0,evidence);});

        const std::array<std::pair<std::int64_t,std::int64_t>,1> plots{{{1,0}}};
        rejects([&]{(void)bind_bnw_deficit_military_context(
            world,5,plots,evidence);});
        evidence.military_ai_current=true;
        evidence.land_defense=0;evidence.naval_defense=1;
        const auto bound=bind_bnw_deficit_military_context(
            world,5,plots,evidence);
        check(bound.land_defense==0 && bound.naval_defense==1 &&
              bound.plot_unit_fighting.at({1,0}),
              "Current AI states and plot fighting did not bind candidate inputs");
        evidence.naval_defense=4;
        rejects([&]{(void)bind_bnw_deficit_military_context(
            world,5,plots,evidence);});
        world.world.game_state.layout=saves::GameStateLayout::vp542_corpus;
        evidence.naval_defense=1;
        rejects([&]{(void)bind_bnw_deficit_military_context(
            world,5,plots,evidence);});
        std::cout << "PASS BNW deficit live binding\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW deficit live binding: " << error.what() << '\n';
        return 1;
    }
}
