#include "civ5/runtime/bnw_team_tech_route_build.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
void grant_low_level_flag(LiveSessionState &state,std::int64_t technology,
                          std::string_view type) {
    state.technologies[63].set_has_tech(technology,true);
    auto &team=state.world.teams->teams[63];
    std::vector<std::int64_t> *hashes=nullptr,*flags=nullptr;
    for(auto &array:team.arrays) {
        if(array.name && std::string_view(array.name)=="tech_hashes") hashes=&array.values;
        if(array.name && std::string_view(array.name)=="has_tech") flags=&array.values;
    }
    check(hashes && flags && hashes->size()==flags->size(),
          "Technology raw arrays unavailable");
    std::size_t found=0;
    const auto hash=content::saved_info_type_hash(type);
    for(std::size_t row=0;row<hashes->size();++row)
        if((*hashes)[row]==hash) {(*flags)[row]=1;++found;}
    check(found==1,"Technology raw hash ambiguous");
}
const saves::TeamHashedArray &hashed(const LiveSessionState &state,
                                    std::string_view name) {
    for(const auto &array:state.world.teams->teams[63].hashes)
        if(array.name && name==array.name) return array;
    throw std::runtime_error("Missing team hashed array");
}
LiveSessionState synthetic() {
    LiveSessionState state{
        saves::SavedSessionSnapshot{},ResearchContext{},
        std::vector<TeamTechnologies>{},PlayerResearchContext{},
        CityYieldContext{},BnwUnhappinessContext{},MaintenanceContext{},
        TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{5,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{7,0,0})}};
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    state.world.game_state.layout=bnw;
    state.world.teams.emplace();
    state.world.teams->layout=bnw;
    state.world.teams->teams.resize(64);
    state.world.all_players.emplace();
    state.world.all_players->players.resize(64);
    ResearchPlayerState barbarian;
    barbarian.team=63;
    barbarian.alive=barbarian.ever_alive=true;
    state.setup.players.emplace(63,barbarian);
    for(std::int64_t team=0;team<64;++team)
        state.technologies.emplace_back(content::TechnologyProfile::bnw,team,1);
    auto &saved=state.world.teams->teams[63];
    saved.arrays={{"tech_hashes",1,{content::saved_info_type_hash("TECH_AGRICULTURE")}},
                  {"has_tech",1,{0}}};
    saved.hashes={{"route_change",2,{
                       {content::saved_info_type_hash("ROUTE_ROAD"),{5}},
                       {content::saved_info_type_hash("ROUTE_RAILROAD"),{0}}}},
                  {"build_time_change",2,{
                       {content::saved_info_type_hash("BUILD_ROAD"),{3}},
                       {content::saved_info_type_hash("BUILD_FARM"),{0}}}}};
    grant_low_level_flag(state,0,"TECH_AGRICULTURE");
    return state;
}
void synthetic_effects() {
    content::RulesDatabase database;
    database.apply_sql(
        "CREATE TABLE Technologies(ID INTEGER PRIMARY KEY,Type TEXT);"
        "INSERT INTO Technologies VALUES(0,'TECH_AGRICULTURE');"
        "CREATE TABLE Routes(ID INTEGER PRIMARY KEY,Type TEXT);"
        "INSERT INTO Routes VALUES(0,'ROUTE_ROAD'),(1,'ROUTE_RAILROAD');"
        "CREATE TABLE Builds(ID INTEGER PRIMARY KEY,Type TEXT);"
        "INSERT INTO Builds VALUES(0,'BUILD_ROAD'),(1,'BUILD_FARM');"
        "CREATE TABLE Resources(TechReveal TEXT,TechCityTrade TEXT);"
        "CREATE TABLE Route_TechMovementChanges(RouteType TEXT,TechType TEXT,MovementChange INTEGER);"
        "INSERT INTO Route_TechMovementChanges VALUES('ROUTE_ROAD','TECH_AGRICULTURE',2);"
        "CREATE TABLE Build_TechTimeChanges(BuildType TEXT,TechType TEXT,TimeChange INTEGER);"
        "INSERT INTO Build_TechTimeChanges VALUES('BUILD_ROAD','TECH_AGRICULTURE',-1);");
    const auto types=database.cache_info_types();
    content::TechnologyRules techs;
    techs.collection.rows=techs.collection.slots=1;
    techs.entries[0].base.type="TECH_AGRICULTURE";
    auto draft=synthetic();
    const auto prior=draft;
    const auto result=run_bnw_barbarian_route_build_prefix(
        draft,techs,database,types,0);
    check(result.applied && !result.grant_complete && result.calls.size()==4 &&
          result.calls[0].kind==BnwTechRouteBuildKind::route_movement &&
          result.calls[0].delta==2 && result.calls[0].after==7 &&
          result.calls[1].delta==0 &&
          result.calls[2].kind==BnwTechRouteBuildKind::build_time &&
          result.calls[2].delta==-1 && result.calls[2].after==2 &&
          hashed(draft,"route_change").rows[0].values[0]==7 &&
          hashed(draft,"build_time_change").rows[0].values[0]==2 &&
          hashed(prior,"route_change").rows[0].values[0]==5,
          "Source-ordered route/build deltas or zero default changed");
    auto malformed=synthetic();
    malformed.world.teams->teams[63].hashes[0].rows[0].type_hash=
        content::saved_info_type_hash("BUILD_ROAD");
    const auto before=malformed.world.teams->teams[63].hashes[1].rows[0].values[0];
    const auto rejected=run_bnw_barbarian_route_build_prefix(
        malformed,techs,database,types,0);
    check(!rejected.applied && !rejected.grant_complete &&
          malformed.world.teams->teams[63].hashes[1].rows[0].values[0]==before,
          "Malformed route hash partially changed detached state");
    auto overflow=synthetic();
    overflow.world.teams->teams[63].hashes[0].rows[0].values[0]=INT32_MAX;
    const auto overflow_result=run_bnw_barbarian_route_build_prefix(
        overflow,techs,database,types,0);
    check(!overflow_result.applied &&
          hashed(overflow,"route_change").rows[0].values[0]==INT32_MAX &&
          hashed(overflow,"build_time_change").rows[0].values[0]==3,
          "Overflowing route delta partially changed detached state");
}
void selected(const std::filesystem::path &save,const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected BNW host unavailable");
    const auto &host=*bootstrap.preview.host;
    const auto types=host.database().cache_info_types();
    auto draft=host.session().state();
    std::int64_t agriculture=-1;
    for(const auto &[slot,info]:host.technologies().entries)
        if(info.base.type=="TECH_AGRICULTURE")
            agriculture=static_cast<std::int64_t>(slot);
    check(agriculture==0,"Selected Agriculture slot changed");
    const auto rng=draft.random.gameplay.state();
    grant_low_level_flag(draft,agriculture,"TECH_AGRICULTURE");
    const auto result=run_bnw_barbarian_route_build_prefix(
        draft,host.technologies(),host.database(),types,agriculture);
    const auto nonzero=std::count_if(result.calls.begin(),result.calls.end(),
        [](const auto &call) {return call.delta!=0;});
    std::cout << "selected Agriculture route/build calls=" << result.calls.size()
              << " nonzero=" << nonzero << " applied=" << result.applied
              << " blocker=" << result.blocker << '\n';
    for(const auto &call:result.calls)
        if(call.delta)
            std::cout << call.type << " delta=" << call.delta
                      << " before=" << call.before << " after=" << call.after << '\n';
    check(result.applied && !result.grant_complete &&
          result.calls.size()==37 && nonzero==0 &&
          draft.random.gameplay.state()==rng &&
          !host.session().state().technologies[63].has_tech(agriculture),
          "Selected Gandhi route/build prefix was not safely staged");
}
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and installed game root");
    synthetic_effects();
    selected(argv[1],argv[2]);
}
