#include "civ5/runtime/bnw_military_settler_demand.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool ok,const char *message) {
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    try {action();} catch (const std::exception &) {return;}
    throw std::runtime_error("Missing BNW settler demand blocker");
}
void unit_field(saves::SavedUnitObservation &unit,const char *name,
                std::int64_t value) {
    saves::UnitCoreField field{};field.name=name;field.values={value};
    unit.core.fields.push_back(std::move(field));
}
saves::SavedUnitObservation unit(std::int64_t id,std::int64_t type,
                                std::string_view name,std::int64_t ai) {
    saves::SavedUnitObservation result;
    result.core.layout=bnw;
    unit_field(result,"m_iID",id);unit_field(result,"m_eOwner",5);
    unit_field(result,"m_eUnitType",type);
    unit_field(result,"idUnitType",content::saved_info_type_hash(name));
    unit_field(result,"m_eUnitAIType",ai);
    return result;
}
void city_field(saves::SavedCityObservation &city,const char *name,
                std::int64_t value) {
    saves::CityCoreField field{};field.name=name;field.values={value};
    city.core.fields.push_back(std::move(field));
}
LiveSessionState world() {
    LiveSessionState state{
        saves::SavedSessionSnapshot{},ResearchContext{}, {},PlayerResearchContext{},
        CityYieldContext{},BnwUnhappinessContext{},MaintenanceContext{},
        TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{0,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{0,0,0})},
        ResearchSelectionState{},BnwGameClockState{},std::nullopt,std::nullopt};
    state.world.game_state.layout=bnw;
    auto &players=state.world.all_players.emplace();players.layout=bnw;
    auto &player=players.players.emplace_back();
    player.saved_player_id=5;player.cities.layout=player.units.layout=bnw;
    player.units.units.push_back(unit(3,0,"UNIT_SETTLER",0));
    player.units.units.push_back(unit(4,1,"UNIT_WARRIOR",1));
    auto &live=state.cities.players[5];
    for (std::int64_t id:{10,11}) {
        auto &saved=player.cities.cities.emplace_back();
        saved.core.layout=saved.production.layout=bnw;
        saved.core.start_offset=static_cast<std::uint64_t>(id*10);
        saved.core.end_offset=static_cast<std::uint64_t>(id*10+9);
        city_field(saved,"m_iID",id);city_field(saved,"m_eOwner",5);
        auto &current=live.cities[id];
        current.id=id;current.owner=5;
        current.source_core=std::pair{saved.core.start_offset,
                                      saved.core.end_offset};
        current.production_orders.emplace();
        current.production_progress.emplace();
    }
    auto &first=*live.cities.at(10).production_orders;
    CityYieldState::ProductionOrder settler;
    settler.type=0;settler.item=0;
    settler.source_hash=content::saved_info_type_hash("UNIT_SETTLER");
    first.push_back(settler);
    auto &second=*live.cities.at(11).production_orders;
    CityYieldState::ProductionOrder building;
    building.type=1;building.item=7;
    second.push_back(building);
    second.push_back(settler); // Only the head is currently being trained.
    return state;
}
} // namespace

int main() {
    try {
        content::RulesDatabase db;
        db.apply_sql("CREATE TABLE UnitAIInfos(ID INTEGER,Type TEXT);"
            "INSERT INTO UnitAIInfos VALUES(0,'UNITAI_SETTLE'),"
            "(1,'UNITAI_ATTACK');"
            "CREATE TABLE Units(ID INTEGER,Type TEXT,DefaultUnitAI TEXT);"
            "INSERT INTO Units VALUES(0,'UNIT_SETTLER','UNITAI_SETTLE'),"
            "(1,'UNIT_WARRIOR','UNITAI_ATTACK');"
            "CREATE TABLE Defines(Name TEXT,Value INTEGER);"
            "INSERT INTO Defines VALUES('MAX_HIT_POINTS',100);");
        const auto types=db.cache_info_types();
        const auto defines=db.cache_global_defines(
            content::GlobalDefineProfile::bnw);
        const auto rules=load_bnw_settler_demand_rules(db,types,defines);
        auto state=world();
        const auto rng=state.random;
        BnwSettlerDemandEvidence fresh{true,true,true};
        // SDK Expansion2 CvPlayer.cpp:3621 counts actual UnitAI on existing
        // units and DefaultUnitAI only for the city's current ORDER_TRAIN.
        const auto result=bind_bnw_settler_demand(state,5,rules,fresh);
        check(result.cities==2 && result.existing_settlers==1 &&
              result.being_trained==1 && result.total_settlers==2,
              "BNW city/settler demand differs from CvPlayer source");
        auto reassigned=state;
        reassigned.world.all_players->players.front().units.units[1]
            .core.fields[4].values={0};
        const auto actual_ai=bind_bnw_settler_demand(
            reassigned,5,rules,fresh);
        check(actual_ai.existing_settlers==2 && actual_ai.being_trained==1,
              "Existing units must use their current UnitAI");
        auto warrior_order=state;
        auto &head=warrior_order.cities.players.at(5).cities.at(10)
            .production_orders->front();
        head.item=1;
        head.source_hash=content::saved_info_type_hash("UNIT_WARRIOR");
        const auto default_ai=bind_bnw_settler_demand(
            warrior_order,5,rules,fresh);
        check(default_ai.existing_settlers==1 && default_ai.being_trained==0,
              "Train orders must use UnitEntry DefaultUnitAI");
        fresh.production_queues_current=false;
        rejects([&]{(void)bind_bnw_settler_demand(state,5,rules,fresh);});
        fresh.production_queues_current=true;fresh.unit_roster_current=false;
        rejects([&]{(void)bind_bnw_settler_demand(state,5,rules,fresh);});
        fresh.unit_roster_current=true;
        auto missing_queue=state;
        missing_queue.cities.players.at(5).cities.at(10).production_orders.reset();
        rejects([&]{(void)bind_bnw_settler_demand(
            missing_queue,5,rules,fresh);});
        auto mismatch=state;
        mismatch.cities.players.at(5).cities.at(10).source_core=
            std::pair<std::uint64_t,std::uint64_t>{1,2};
        rejects([&]{(void)bind_bnw_settler_demand(mismatch,5,rules,fresh);});
        auto wrong_hash=state;
        wrong_hash.cities.players.at(5).cities.at(10)
            .production_orders->front().source_hash=11;
        rejects([&]{(void)bind_bnw_settler_demand(
            wrong_hash,5,rules,fresh);});
        auto vp=state;
        vp.world.game_state.layout=saves::GameStateLayout::vp542_corpus;
        rejects([&]{(void)bind_bnw_settler_demand(vp,5,rules,fresh);});
        const auto vp_defines=db.cache_global_defines(
            content::GlobalDefineProfile::vp542);
        rejects([&]{(void)load_bnw_settler_demand_rules(
            db,types,vp_defines);});
        db.apply_sql("UPDATE Units SET Type='UNIT_CHANGED' "
                     "WHERE Type='UNIT_SETTLER';");
        rejects([&]{(void)load_bnw_settler_demand_rules(db,types,defines);});
        check(state.random.gameplay.state()==rng.gameplay.state() &&
              state.random.map.state()==rng.map.state(),
              "BNW settler demand advanced RNG");
        std::cout << "PASS BNW military city/settler demand\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW military city/settler demand: "
                  << error.what() << '\n';
        return 1;
    }
}
