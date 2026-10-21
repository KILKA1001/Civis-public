#include "civ5/runtime/bnw_player_uprising_turn.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include <climits>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool condition,const char *message) {
    if(!condition) throw std::runtime_error(message);
}
saves::PlayerCoreField field(const char *name,saves::PlayerCoreEncoding encoding,
                             std::int64_t value) {
    saves::PlayerCoreField result{};
    result.name=name;result.encoding=encoding;result.values={value};
    return result;
}
void populate_rules(content::RulesDatabase &db,std::int64_t minimum=4,
                    std::int64_t possible=3,std::int64_t speed=100) {
    db.apply_sql("CREATE TABLE Defines(Name TEXT,Value INTEGER);"
        "INSERT INTO Defines VALUES('VERY_UNHAPPY_THRESHOLD',-10),"
        "('UNHAPPINESS_PER_CITY',3),('UNHAPPINESS_PER_CAPTURED_CITY',5),"
        "('UNHAPPINESS_PER_POPULATION',1),"
        "('UNHAPPINESS_PER_OCCUPIED_POPULATION',1),"
        "('FREE_UNIT_HAPPINESS',0);"
        "CREATE TABLE HandicapInfos(ID INTEGER,Type TEXT,NumCitiesUnhappinessMod INTEGER,"
        "PopulationUnhappinessMod INTEGER,AIUnhappinessPercent INTEGER);"
        "INSERT INTO HandicapInfos VALUES(0,'HANDICAP_TEST',100,100,100);"
        "CREATE TABLE Specialists(ID INTEGER,Type TEXT);"
        "INSERT INTO Specialists VALUES(0,'SPECIALIST_CITIZEN');"
        "CREATE TABLE GameSpeeds(ID INTEGER,Type TEXT,TrainPercent INTEGER);"
        "INSERT INTO GameSpeeds VALUES(0,'GAMESPEED_TEST',100);");
    db.apply_sql("INSERT INTO Defines VALUES('UPRISING_COUNTER_MIN',"+
                 std::to_string(minimum)+"),('UPRISING_COUNTER_POSSIBLE',"+
                 std::to_string(possible)+");UPDATE GameSpeeds SET TrainPercent="+
                 std::to_string(speed)+" WHERE ID=0;");
}
SessionCheckpointIdentity identity(const content::RulesDatabase &db) {
    return {"fixture",identify_bytes(db.gameplay_fingerprint_data()).sha256};
}
LiveSessionState fixture(std::int64_t count,bool very_unhappy=true) {
    LiveSessionState state{saves::SavedSessionSnapshot{},ResearchContext{}, {},
        PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},
        MaintenanceContext{},TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{3,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{5,0,0})}};
    state.world.game_state.layout=saves::GameStateLayout::expansion2_version1;
    auto &all=state.world.all_players.emplace();
    all.layout=saves::GameStateLayout::expansion2_version1;
    auto &player=all.players.emplace_back();
    player.saved_player_id=5;
    player.core.layout=saves::GameStateLayout::expansion2_version1;
    player.core.fields={field("m_eID",saves::PlayerCoreEncoding::i32,5),
        field("m_bAlive",saves::PlayerCoreEncoding::boolean,1),
        field("m_iUprisingCounter",saves::PlayerCoreEncoding::i32,count)};
    state.setup.profile=state.cities.profile=content::TechnologyProfile::bnw;
    state.setup.speed=0;state.setup.game_handicap=0;
    state.setup.game_options.push_back({"GAMEOPTION_NO_HAPPINESS",0});
    auto &setup=state.setup.players[5];
    setup.alive=true;setup.slot_status=3;setup.team=0;setup.handicap=0;
    state.cities.players[5];
    state.unhappiness.world_cities_percent=100;
    auto &unhappy=state.unhappiness.players[5];
    unhappy.happiness=very_unhappy?0:20;
    unhappy.units=1100; // 11 unhappiness, so excess is -11 or +9.
    return state;
}
std::int64_t counter(const LiveSessionState &state) {
    for(const auto &entry:state.world.all_players->players.front().core.fields)
        if(entry.name && std::string_view(entry.name)=="m_iUprisingCounter")
            return entry.values.at(0);
    throw std::runtime_error("fixture uprising counter absent");
}
void blocked(LiveSessionState &state,const content::RulesDatabase &db,
             BnwUprisingTurnBlocker blocker) {
    const auto before=counter(state);
    const auto random=state.random.gameplay.state();
    bool saw=false;
    try {(void)apply_bnw_player_uprising_turn(state,5,db,identity(db));}
    catch(const BnwUprisingTurnError &error) {saw=error.blocker()==blocker;}
    check(saw,"wrong BNW uprising typed blocker");
    check(counter(state)==before && state.random.gameplay.state()==random,
          "blocked BNW uprising consumed RNG or changed counter");
}
} // namespace

int main() {
    try {
        content::RulesDatabase db;
        populate_rules(db);
        {
            auto state=fixture(3,false);
            const auto step=apply_bnw_player_uprising_turn(state,5,db,identity(db));
            check(step.branch==BnwUprisingTurnBranch::not_very_unhappy &&
                  step.before==3 && step.after==3 && step.gameplay_rng_calls==0 &&
                  counter(state)==3 && state.random.gameplay.state().calls==0,
                  "source no-unhappiness branch differs");
        }
        {
            auto state=fixture(3);
            const auto step=apply_bnw_player_uprising_turn(state,5,db,identity(db));
            check(step.branch==BnwUprisingTurnBranch::decrement &&
                  step.before==3 && step.after==2 && counter(state)==2 &&
                  state.random.gameplay.state().calls==0,
                  "source positive countdown branch differs");
        }
        {
            auto state=fixture(1);
            blocked(state,db,BnwUprisingTurnBlocker::uprising_spawn);
        }
        for(const auto before:{0,-2}) {
            auto state=fixture(before);
            auto expected_rng=state.random.gameplay;
            const auto extra=expected_rng.draw(3);
            const auto step=apply_bnw_player_uprising_turn(state,5,db,identity(db));
            const auto expected=static_cast<std::int64_t>((4+extra)/2);
            check(step.branch==BnwUprisingTurnBranch::first_seed &&
                  step.before==before && step.after==expected &&
                  step.gameplay_rng_calls==1 && counter(state)==expected &&
                  state.random.gameplay.state()==expected_rng.state(),
                  "source first-time uprising seed or RNG differs");
        }
        for(const auto possible:{0,1}) {
            content::RulesDatabase bounded_rules;
            populate_rules(bounded_rules,4,possible,200);
            auto state=fixture(0);
            const auto step=apply_bnw_player_uprising_turn(
                state,5,bounded_rules,identity(bounded_rules));
            check(step.branch==BnwUprisingTurnBranch::first_seed &&
                  step.after==4 && counter(state)==4 &&
                  state.random.gameplay.state().calls==1,
                  "zero/one RNG bound or speed scaling differs from source");
        }
        {
            auto state=fixture(0);
            state.setup.game_options[0].value=1;
            state.cities.no_happiness=true;
            const auto step=apply_bnw_player_uprising_turn(state,5,db,identity(db));
            check(step.branch==BnwUprisingTurnBranch::not_very_unhappy &&
                  counter(state)==0 && state.random.gameplay.state().calls==0,
                  "NO_HAPPINESS bypass consumed RNG");
        }
        {
            content::RulesDatabase overflow_rules;
            populate_rules(overflow_rules,INT_MAX,3,200);
            auto state=fixture(0);
            blocked(state,overflow_rules,BnwUprisingTurnBlocker::source_integer_range);
        }
        {
            auto state=fixture(3);
            state.unhappiness.players.erase(5);
            blocked(state,db,BnwUprisingTurnBlocker::happiness);
        }
        std::cout << "BNW DoUpdateUprisings bounded branches PASS\n";
    } catch(const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
