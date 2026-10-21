#include "civ5/runtime/bnw_delayed_death.hpp"
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
saves::SavedUnitObservation unit(std::int64_t id,std::uint64_t index) {
    saves::SavedUnitObservation result;
    result.index=index;result.core.layout=result.tail.layout=bnw;
    core(result,"m_iID",id);core(result,"m_eOwner",5);
    core(result,"m_bDeathDelay",1);
    core(result,"m_iX",2);core(result,"m_iY",3);
    core(result,"m_iArmyId",-1);core(result,"m_iCargo",0);
    core(result,"m_iReconX",-1);core(result,"m_iReconY",-1);
    core(result,"m_iAttackPlotX",-1);core(result,"m_iAttackPlotY",-1);
    core(result,"m_iGarrisonCityID",-1);
    core(result,"m_eCapturingPlayer",-1);
    core(result,"m_eLeaderUnitType",-1);
    tail(result,"m_combatUnit.eOwner",-1);
    tail(result,"m_combatUnit.iID",-1);
    tail(result,"m_transportUnit.eOwner",-1);
    tail(result,"m_transportUnit.iID",-1);
    return result;
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
    player.units.units.push_back(unit(7,0));
    result.maintenance.profile=content::TechnologyProfile::bnw;
    result.maintenance.players[5].units[7]=MaintenanceUnit{};
    return result;
}
void update(saves::SavedUnitObservation &unit,std::string_view name,
            std::int64_t value) {
    for (auto &field:unit.core.fields)
        if (field.name && std::string_view(field.name)==name) {
            field.values={value};return;
        }
    throw std::runtime_error("Fixture core field missing");
}
template<class F> void rejects(F run) {
    bool caught=false;
    try {run();} catch (const std::exception &) {caught=true;}
    check(caught,"Expected BNW delayed-death blocker");
}
} // namespace

int main() {
    try {
        // Official Expansion2 CvUnit.cpp:16312: delayed and neither fighting
        // nor busy -> kill(false); CvPlayer.cpp:19172 removes the unit by ID.
        const BnwDeficitCandidate selected{7,0,10,BnwDeficitDomain::land};
        const BnwDelayedDeathLiveChecks ready{false,false};
        auto detached=state();
        const auto removed=stage_bnw_delayed_death_draft(
            detached,5,selected,ready);
        check(removed.unit_id==7 && removed.source_index==0 &&
              detached.world.all_players->players.front().units.units.empty() &&
              detached.maintenance.players.at(5).units.empty(),
              "Detached death failed to remove owned unit views");
        auto busy=state();
        rejects([&]{(void)stage_bnw_delayed_death_draft(
            busy,5,selected,{false,true});});
        rejects([&]{(void)stage_bnw_delayed_death_draft(
            busy,5,selected,{true,false});});
        rejects([&]{(void)stage_bnw_delayed_death_draft(
            busy,5,selected,{});});
        check(busy.world.all_players->players.front().units.units.size()==1 &&
              busy.maintenance.players.at(5).units.size()==1,
              "Incomplete live checks changed delayed-death draft");
        auto not_delayed=state();
        update(not_delayed.world.all_players->players.front()
                   .units.units.front(),"m_bDeathDelay",0);
        rejects([&]{(void)stage_bnw_delayed_death_draft(
            not_delayed,5,selected,ready);});
        auto in_army=state();
        update(in_army.world.all_players->players.front()
                   .units.units.front(),"m_iArmyId",2);
        rejects([&]{(void)stage_bnw_delayed_death_draft(
            in_army,5,selected,ready);});
        auto cargo=state();
        auto &other=cargo.world.all_players->players.front()
            .units.units.emplace_back(unit(8,1));
        for (auto &field:other.tail.blocks.front().children) {
            if (field.name && std::string_view(field.name)=="m_transportUnit.eOwner")
                field.values={5};
            if (field.name && std::string_view(field.name)=="m_transportUnit.iID")
                field.values={7};
        }
        rejects([&]{(void)stage_bnw_delayed_death_draft(
            cargo,5,selected,ready);});
        auto wrong_profile=state();
        wrong_profile.maintenance.profile=content::TechnologyProfile::vp542;
        rejects([&]{(void)stage_bnw_delayed_death_draft(
            wrong_profile,5,selected,ready);});

        saves::SavedSession source;source.snapshot=state().world;
        auto game=LiveSession::from_restored_bnw(std::move(source),state());
        const auto calls=game.state().random.gameplay.state().calls;
        const auto blocked=try_bnw_delayed_death_draft(
            game,5,selected,ready);
        check(blocked.outcome==LiveActionOutcome::rejected &&
              blocked.reason.find("Lua")!=std::string::npos &&
              game.state().world.all_players->players.front().units.units.size()==1 &&
              game.state().maintenance.players.at(5).units.size()==1 &&
              game.state().random.gameplay.state().calls==calls,
              "Partial kill escaped from rejected live transaction");
        const auto rollback=apply_live_action(game,[&](LiveSessionState &copy) {
            (void)copy.random.gameplay.draw(100);
            (void)stage_bnw_delayed_death_draft(copy,5,selected,ready);
            return LiveActionResult{LiveActionOutcome::rejected,
                                    "Mandatory effects unavailable"};
        });
        check(rollback.outcome==LiveActionOutcome::rejected &&
              game.state().random.gameplay.state().calls==calls &&
              game.state().world.all_players->players.front().units.units.size()==1 &&
              game.state().maintenance.players.at(5).units.size()==1,
              "Rejected delayed death failed to restore RNG and owned units");
        std::cout << "PASS BNW delayed-death draft\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW delayed-death draft: " << error.what() << '\n';
        return 1;
    }
}
