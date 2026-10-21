#include "civ5/runtime/bnw_deficit_scrap.hpp"
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
void field(saves::SavedUnitObservation &unit,const char *name,
           std::int64_t value) {
    saves::UnitCoreField item{};item.name=name;item.values={value};
    unit.core.fields.push_back(std::move(item));
}
std::int64_t delay(const LiveSessionState &state) {
    const auto &unit=state.world.all_players->players.front().units.units.front();
    for (const auto &field:unit.core.fields)
        if (field.name && std::string_view(field.name)=="m_bDeathDelay")
            return field.values.at(0);
    throw std::runtime_error("Death-delay field missing in fixture");
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
    auto &unit=player.units.units.emplace_back();
    unit.index=0;unit.core.layout=bnw;
    field(unit,"m_iID",7);field(unit,"m_eOwner",5);
    field(unit,"m_iX",3);field(unit,"m_iY",4);
    field(unit,"m_iMoves",60);field(unit,"m_iCargo",0);
    field(unit,"m_iDamage",25);field(unit,"m_bDeathDelay",0);
    field(unit,"idUnitType",123);
    result.treasury.profile=content::TechnologyProfile::bnw;
    result.treasury.players[5].balance_times100=1200;
    return result;
}
template<class F> void rejects(F run) {
    bool caught=false;
    try {run();} catch (const std::exception &) {caught=true;}
    check(caught,"Expected BNW scrap draft blocker");
}
} // namespace

int main() {
    try {
        // Official Expansion2 CvUnit.cpp:3231 scrap() refunds before kill(true);
        // 3252 GetScrapGold truncates after refund, health and speed steps.
        const BnwDeficitCandidate selected{7,0,10,BnwDeficitDomain::land};
        BnwDeficitScrapRules rules;
        rules.production_cost_by_unit_hash[123]=80;
        rules.refund_percent=10;rules.train_percent=100;
        rules.max_hit_points=100;rules.unit_delete_disabled=false;
        const BnwDeficitScrapPlot owned{3,4,5,false};
        auto detached=state();
        const auto staged=stage_bnw_deficit_scrap_draft(
            detached,5,selected,rules,owned);
        check(staged.refund_gold==6 && staged.unit_id==7 &&
              detached.treasury.players.at(5).balance_times100==1800 &&
              delay(detached)==1,
              "Owned BNW scrap draft missed SDK refund or delayed-death flag");
        auto foreign=state();
        const auto outside=stage_bnw_deficit_scrap_draft(
            foreign,5,selected,rules,{3,4,2,false});
        check(outside.refund_gold==0 &&
              foreign.treasury.players.at(5).balance_times100==1200 &&
              delay(foreign)==1,
              "Scrap outside owner's plot incorrectly refunded gold");
        auto unavailable=state();
        rejects([&]{(void)stage_bnw_deficit_scrap_draft(
            unavailable,5,selected,rules,{});});
        check(delay(unavailable)==0 &&
              unavailable.treasury.players.at(5).balance_times100==1200,
              "Unbound live plot changed detached state");
        rejects([&]{(void)stage_bnw_deficit_scrap_draft(
            unavailable,5,selected,rules,{9,4,5,false});});
        rejects([&]{(void)stage_bnw_deficit_scrap_draft(
            unavailable,5,{8,0,10,BnwDeficitDomain::land},rules,owned);});
        auto cargo=state();
        for (auto &field:cargo.world.all_players->players.front()
                 .units.units.front().core.fields)
            if (field.name && std::string_view(field.name)=="m_iCargo")
                field.values.front()=1;
        rejects([&]{(void)stage_bnw_deficit_scrap_draft(
            cargo,5,selected,rules,owned);});
        check(delay(cargo)==0 &&
              cargo.treasury.players.at(5).balance_times100==1200,
              "Cargo branch was mistaken for simple scrap");
        auto vp=state();vp.treasury.profile=content::TechnologyProfile::vp542;
        rejects([&]{(void)stage_bnw_deficit_scrap_draft(
            vp,5,selected,rules,owned);});
        auto overflow=state();
        rules.production_cost_by_unit_hash[123]=INT32_MAX;
        rejects([&]{(void)stage_bnw_deficit_scrap_draft(
            overflow,5,selected,rules,owned);});
        rules.production_cost_by_unit_hash[123]=80;

        saves::SavedSession source;source.snapshot=state().world;
        auto game=LiveSession::from_restored_bnw(std::move(source),state());
        const auto before=game.state().random.gameplay.state().calls;
        const auto blocked=try_bnw_deficit_scrap_draft(
            game,5,selected,rules,owned);
        check(blocked.outcome==LiveActionOutcome::rejected &&
              blocked.reason.find("Lua")!=std::string::npos &&
              game.state().treasury.players.at(5).balance_times100==1200 &&
              delay(game.state())==0 &&
              game.state().random.gameplay.state().calls==before,
              "Unpublished draft escaped into live treasury, unit or RNG");
        const auto rollback=apply_live_action(game,[&](LiveSessionState &copy) {
            (void)copy.random.gameplay.draw(100);
            (void)stage_bnw_deficit_scrap_draft(copy,5,selected,rules,owned);
            return LiveActionResult{LiveActionOutcome::rejected,
                                    "Required Lua unavailable"};
        });
        check(rollback.outcome==LiveActionOutcome::rejected &&
              game.state().random.gameplay.state().calls==before &&
              game.state().treasury.players.at(5).balance_times100==1200 &&
              delay(game.state())==0,
              "Rejected DoDeficit draft failed to roll back RNG and owned world");
        std::cout << "PASS BNW deficit scrap draft\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW deficit scrap draft: " << error.what() << '\n';
        return 1;
    }
}
