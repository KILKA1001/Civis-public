#include "civ5/runtime/bnw_military_base_units.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool ok,const char *message) {
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    try {action();} catch (const std::exception &) {return;}
    throw std::runtime_error("Missing BNW military base-unit blocker");
}
void field(saves::SavedUnitObservation &unit,const char *name,
           std::int64_t value) {
    saves::UnitCoreField row{};row.name=name;row.values={value};
    unit.core.fields.push_back(std::move(row));
}
saves::SavedUnitObservation unit(std::int64_t id,std::string_view type,
                                std::int64_t combat,std::int64_t ai=2,
                                std::int64_t intercept=0,
                                std::int64_t army=-1) {
    saves::SavedUnitObservation result;
    result.core.layout=saves::GameStateLayout::expansion2_version1;
    field(result,"m_iID",id);field(result,"m_eOwner",5);
    std::int64_t type_id=-1;
    if (type=="UNIT_MELEE") type_id=0;
    else if (type=="UNIT_RANGED") type_id=1;
    else if (type=="UNIT_MOBILE") type_id=2;
    else if (type=="UNIT_NAVAL") type_id=3;
    else if (type=="UNIT_AIR") type_id=4;
    else if (type=="UNIT_HOVER") type_id=5;
    field(result,"m_eUnitType",type_id);
    field(result,"idUnitType",content::saved_info_type_hash(type));
    field(result,"m_eUnitAIType",ai);
    field(result,"m_iBaseCombat",combat);
    field(result,"m_iOnlyDefensiveCount",0);
    field(result,"m_iExtraRange",0);
    field(result,"m_iExtraIntercept",intercept);
    field(result,"m_iArmyId",army);
    return result;
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
    auto &player=all.players.emplace_back();
    player.saved_player_id=5;
    player.units.layout=saves::GameStateLayout::expansion2_version1;
    player.units.units.push_back(unit(1,"UNIT_MELEE",10));
    player.units.units.push_back(unit(2,"UNIT_RANGED",0));
    player.units.units.push_back(unit(3,"UNIT_MELEE",10,2,1));
    player.units.units.push_back(unit(4,"UNIT_MOBILE",10));
    player.units.units.push_back(unit(5,"UNIT_NAVAL",10,2,0,7));
    player.units.units.push_back(unit(6,"UNIT_AIR",10));
    player.units.units.push_back(unit(7,"UNIT_MELEE",10,0));
    player.units.units.push_back(unit(8,"UNIT_MELEE",0));
    return state;
}
} // namespace

int main() {
    try {
        content::RulesDatabase db;
        db.apply_sql("CREATE TABLE Domains(ID INTEGER,Type TEXT);"
            "INSERT INTO Domains VALUES(0,'DOMAIN_LAND'),(1,'DOMAIN_SEA'),"
            "(2,'DOMAIN_AIR'),(3,'DOMAIN_HOVER');"
            "CREATE TABLE UnitAIInfos(ID INTEGER,Type TEXT);"
            "INSERT INTO UnitAIInfos VALUES(0,'UNITAI_EXPLORE'),"
            "(1,'UNITAI_EXPLORE_SEA'),(2,'UNITAI_ATTACK');"
            "CREATE TABLE Units(ID INTEGER,Type TEXT,Domain TEXT,"
            "RangedCombat INTEGER,Range INTEGER,Moves INTEGER);"
            "INSERT INTO Units VALUES"
            "(0,'UNIT_MELEE','DOMAIN_LAND',0,0,2),"
            "(1,'UNIT_RANGED','DOMAIN_LAND',10,2,2),"
            "(2,'UNIT_MOBILE','DOMAIN_LAND',0,0,3),"
            "(3,'UNIT_NAVAL','DOMAIN_SEA',0,0,4),"
            "(4,'UNIT_AIR','DOMAIN_AIR',0,0,4),"
            "(5,'UNIT_HOVER','DOMAIN_HOVER',0,0,2);"
            "CREATE TABLE Defines(Name TEXT,Value INTEGER);"
            "INSERT INTO Defines VALUES('MAX_HIT_POINTS',100);");
        const auto types=db.cache_info_types();
        const auto defines=db.cache_global_defines(content::GlobalDefineProfile::bnw);
        const auto rules=load_bnw_military_base_unit_rules(db,types,defines);
        auto state=world();
        const auto rng=state.random;
        rejects([&]{(void)count_bnw_military_base_units(state,5,rules,false);});
        const auto counts=count_bnw_military_base_units(state,5,rules,true);
        // SDK Expansion2 CvMilitaryAI.cpp:2054 UpdateBaseData loop. A ranged
        // unit with zero melee combat still attacks; explorers are excluded.
        check(counts.land==4 && counts.ranged_land==1 &&
              counts.anti_air==1 && counts.mobile_land==1 &&
              counts.melee_land==1 && counts.naval==1 && counts.air==1 &&
              counts.land_in_armies==0 && counts.naval_in_armies==1,
              "BNW UpdateBaseData unit counters differ from SDK");
        auto missing=state;
        missing.world.all_players->players.front().units.units.front().core.fields[3].values={99};
        rejects([&]{(void)count_bnw_military_base_units(missing,5,rules,true);});
        auto mismatched_id=state;
        mismatched_id.world.all_players->players.front().units.units.front().core.fields[2].values={1};
        rejects([&]{(void)count_bnw_military_base_units(
            mismatched_id,5,rules,true);});
        auto hover=state;
        hover.world.all_players->players.front().units.units.front().core.fields[2].values={5};
        hover.world.all_players->players.front().units.units.front().core.fields[3].values={
            content::saved_info_type_hash("UNIT_HOVER")};
        rejects([&]{(void)count_bnw_military_base_units(hover,5,rules,true);});
        auto wrong_profile=state;
        wrong_profile.world.game_state.layout=saves::GameStateLayout::vp542_corpus;
        rejects([&]{(void)count_bnw_military_base_units(wrong_profile,5,rules,true);});
        rejects([&]{(void)count_bnw_military_base_units(state,6,rules,true);});
        db.apply_sql("UPDATE Units SET Type='UNIT_CHANGED' WHERE Type='UNIT_RANGED';");
        rejects([&]{(void)load_bnw_military_base_unit_rules(db,types,defines);});
        const auto vp_defines=db.cache_global_defines(
            content::GlobalDefineProfile::vp542);
        rejects([&]{(void)load_bnw_military_base_unit_rules(
            db,db.cache_info_types(),vp_defines);});
        check(state.random.gameplay.state()==rng.gameplay.state() &&
              state.random.map.state()==rng.map.state(),
              "BNW unit count advanced gameplay RNG");
        std::cout << "PASS BNW military UpdateBaseData unit counts\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW military UpdateBaseData unit counts: "
                  << error.what() << '\n';
        return 1;
    }
}
