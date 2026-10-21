#include "civ5/runtime/bnw_player_city_revolt_turn.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool condition,const char *message) {
    if(!condition) throw std::runtime_error(message);
}
content::RulesDatabase &populate(content::RulesDatabase &db) {
    db.apply_sql("CREATE TABLE Defines(Name TEXT,Value INTEGER);"
        "INSERT INTO Defines VALUES('SUPER_UNHAPPY_THRESHOLD',-20),"
        "('UNHAPPINESS_PER_CITY',3),('UNHAPPINESS_PER_CAPTURED_CITY',5),"
        "('UNHAPPINESS_PER_POPULATION',1),"
        "('UNHAPPINESS_PER_OCCUPIED_POPULATION',1),"
        "('FREE_UNIT_HAPPINESS',0);"
        "CREATE TABLE HandicapInfos(ID INTEGER,Type TEXT,NumCitiesUnhappinessMod INTEGER,"
        "PopulationUnhappinessMod INTEGER,AIUnhappinessPercent INTEGER);"
        "INSERT INTO HandicapInfos VALUES(0,'HANDICAP_TEST',100,100,100);"
        "CREATE TABLE Specialists(ID INTEGER,Type TEXT);"
        "INSERT INTO Specialists VALUES(0,'SPECIALIST_CITIZEN');");
    return db;
}
SessionCheckpointIdentity identity(const content::RulesDatabase &db) {
    return {"fixture",identify_bytes(db.gameplay_fingerprint_data()).sha256};
}
saves::PlayerCoreField field(const char *name,saves::PlayerCoreEncoding encoding,
                             std::int64_t value) {
    saves::PlayerCoreField result{};
    result.name=name;result.encoding=encoding;result.values={value};
    return result;
}
LiveSessionState fixture(std::int64_t counter,std::int64_t opinion=1,
                         std::int64_t happiness=0) {
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
        field("m_iCityRevoltCounter",saves::PlayerCoreEncoding::i32,counter)};
    state.setup.profile=state.cities.profile=content::TechnologyProfile::bnw;
    state.setup.game_handicap=0;
    state.setup.game_options.push_back({"GAMEOPTION_NO_HAPPINESS",0});
    auto &setup=state.setup.players[5];
    setup.alive=true;setup.slot_status=3;setup.team=0;setup.handicap=0;
    state.cities.players[5];
    state.unhappiness.world_cities_percent=100;
    auto &unhappy=state.unhappiness.players[5];
    unhappy.happiness=happiness;
    unhappy.units=2100;
    unhappy.public_opinion=opinion;
    return state;
}
std::int64_t count(const LiveSessionState &state) {
    for(const auto &entry:state.world.all_players->players.front().core.fields)
        if(entry.name && std::string_view(entry.name)=="m_iCityRevoltCounter")
            return entry.values.at(0);
    throw std::runtime_error("fixture city revolt counter absent");
}
void blocked(LiveSessionState &state,const content::RulesDatabase &db,
             BnwCityRevoltTurnBlocker expected) {
    const auto before=count(state);
    const auto rng=state.random.gameplay.state();
    bool saw=false;
    try {(void)apply_bnw_player_city_revolt_turn(state,5,db,identity(db));}
    catch(const BnwCityRevoltTurnError &error) {saw=error.blocker()==expected;}
    check(saw,"wrong BNW city revolt typed blocker");
    check(count(state)==before && state.random.gameplay.state()==rng,
          "blocked city revolt mutated counter or gameplay RNG");
}
} // namespace

int main() {
    try {
        content::RulesDatabase db;populate(db);
        {
            auto state=fixture(3,0);
            const auto step=apply_bnw_player_city_revolt_turn(state,5,db,identity(db));
            check(step.branch==BnwCityRevoltTurnBranch::inactive && count(state)==3 &&
                  state.random.gameplay.state().calls==0,
                  "public-opinion guard differs from SDK");
        }
        {
            auto state=fixture(3,1,30);
            const auto step=apply_bnw_player_city_revolt_turn(state,5,db,identity(db));
            check(step.branch==BnwCityRevoltTurnBranch::inactive && count(state)==3,
                  "super-unhappiness guard differs from SDK");
        }
        {
            auto state=fixture(3);
            const auto step=apply_bnw_player_city_revolt_turn(state,5,db,identity(db));
            check(step.branch==BnwCityRevoltTurnBranch::decrement &&
                  step.before==3 && step.after==2 && count(state)==2 &&
                  state.random.gameplay.state().calls==0,
                  "city revolt countdown differs from SDK");
        }
        {
            auto state=fixture(1);
            blocked(state,db,BnwCityRevoltTurnBlocker::city_revolt_and_transfer);
        }
        for(const auto before:{0,-2}) {
            auto state=fixture(before);
            blocked(state,db,BnwCityRevoltTurnBlocker::city_target_and_notification);
        }
        {
            auto state=fixture(0);
            state.setup.game_options[0].value=1;
            state.cities.no_happiness=true;
            const auto step=apply_bnw_player_city_revolt_turn(state,5,db,identity(db));
            check(step.branch==BnwCityRevoltTurnBranch::inactive && count(state)==0,
                  "NO_HAPPINESS city revolt bypass differs from SDK");
        }
        {
            auto state=fixture(3);
            state.unhappiness.players.erase(5);
            blocked(state,db,BnwCityRevoltTurnBlocker::happiness);
        }
        std::cout << "BNW DoUpdateCityRevolts bounded branches PASS\n";
    } catch(const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
