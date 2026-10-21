#include "civ5/runtime/bnw_game_score.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using namespace civ5;
void check(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
runtime::LiveSessionState fixture() {
    saves::SavedSessionSnapshot world;
    world.game_state.layout=saves::GameStateLayout::expansion2_version1;
    world.game_state.prefix=saves::Expansion2PrefixObservation{};
    std::get<saves::Expansion2PrefixObservation>(
        world.game_state.prefix).scalars[28]=100;
    world.game_state.flags.push_back({"score_dirty",true});
    world.game_state.roster.emplace();
    for(auto &array:world.game_state.roster->arrays) array.resize(64);
    world.all_players.emplace();
    world.all_players->layout=saves::GameStateLayout::expansion2_version1;
    world.teams.emplace();
    world.teams->layout=saves::GameStateLayout::expansion2_version1;
    runtime::ResearchContext setup;
    setup.game_turn=5;
    setup.game_options={{"GAMEOPTION_NO_POLICIES",1},
                        {"GAMEOPTION_NO_RELIGION",1},
                        {"GAMEOPTION_NO_SCIENCE",1}};
    for(std::int64_t id=0;id<64;++id) {
        saves::SavedPlayerObservation player;
        player.saved_player_id=id;
        player.core.layout=saves::GameStateLayout::expansion2_version1;
        const auto field=[&](const char *name,std::int64_t value) {
            saves::PlayerCoreField f{};
            f.name=name;f.encoding=saves::PlayerCoreEncoding::i32;
            f.values.push_back(value);
            player.core.fields.push_back(std::move(f));
        };
        field("m_iTotalPopulation",id<2?5:0);
        field("m_iTotalLand",id<2?3:0);
        field("m_iNumWonders",id<2?2:0);
        field("m_iScenarioScore1",id<2?7:0);
        field("m_iScenarioScore2",0);
        field("m_iScenarioScore3",0);
        field("m_iScenarioScore4",0);
        if(id<2) {
            saves::SavedCityObservation city;
            saves::CityProductionRecord buildings{};
            buildings.name="buildings";
            buildings.arrays.push_back({"m_aBuildingGreatWork",3,{}});
            city.production.blocks.push_back(std::move(buildings));
            player.cities.cities.push_back(std::move(city));
        }
        saves::PlayerTailNode names{},values{};
        names.name="m_ReplayDataSets";names.encoding="strings";names.wire_count=0;
        values.name="m_ReplayDataSetValues";values.encoding="turn_sets";values.wire_count=0;
        player.tail.fields.push_back(std::move(names));
        player.tail.fields.push_back(std::move(values));
        world.all_players->players.push_back(std::move(player));
        saves::TeamObservation team;
        team.index=id;
        team.fields.push_back({"members_count",1});
        world.teams->teams.push_back(std::move(team));
        runtime::ResearchPlayerState status;
        status.team=id;status.alive=id<2;
        setup.players.emplace(id,status);
    }
    return {std::move(world),std::move(setup),{},
        runtime::PlayerResearchContext{},runtime::CityYieldContext{},
        runtime::BnwUnhappinessContext{},runtime::MaintenanceContext{},
        runtime::TreasuryContext{},runtime::ScienceContext{},
        runtime::ResearchTurnState{},
        runtime::SessionRandomStreams{
            runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
            runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})}};
}
} // namespace

int main(int argc,char **argv) {
    check(argc==1 || argc==3,"Expected optional selected BNW save and game root");
    content::RulesDatabase rules;
    rules.apply_sql(
        "CREATE TABLE Defines(Name TEXT,Value INTEGER);"
        "CREATE TABLE Policies(ID INTEGER PRIMARY KEY,Type TEXT);"
        "INSERT INTO Policies VALUES(0,'POLICY_TEST');"
        "INSERT INTO Defines VALUES"
        "('SCORE_CITY_MULTIPLIER',10),"
        "('SCORE_POPULATION_MULTIPLIER',4),"
        "('SCORE_LAND_MULTIPLIER',1),"
        "('SCORE_WONDER_MULTIPLIER',25),"
        "('SCORE_POLICY_MULTIPLIER',4),"
        "('SCORE_GREAT_WORK_MULTIPLIER',4),"
        "('SCORE_BELIEF_MULTIPLIER',20),"
        "('SCORE_RELIGION_CITIES_MULTIPLIER',1),"
        "('SCORE_TECH_MULTIPLIER',4);");
    const auto types=rules.cache_info_types();
    const auto defines=rules.cache_global_defines(content::GlobalDefineProfile::bnw);
    auto state=fixture();
    const auto* world_address=&state.world;
    const auto* player_storage=state.world.all_players->players.data();
    const auto* score_storage=state.world.game_state.roster->arrays[3].data();
    const auto* replay_field_storage=state.world.all_players->players[0].tail.fields.data();
    runtime::update_bnw_game_score(state,rules,types,defines);
    check(&state.world==world_address&&state.world.all_players->players.data()==player_storage&&
          state.world.game_state.roster->arrays[3].data()==score_storage&&
          state.world.all_players->players[0].tail.fields.data()==replay_field_storage,
          "Score preparation replaced actual world/player/score/tail containers");
    const auto &roster=*state.world.game_state.roster;
    check(roster.arrays[1][0]==1 && roster.arrays[1][1]==0 &&
          roster.arrays[2][1]==0 && roster.arrays[2][0]==1 &&
          roster.arrays[3][0]==90 && roster.arrays[3][1]==90,
          "BNW tied player score/rank differs from source");
    check(roster.arrays[4][0]==1 && roster.arrays[4][1]==0 &&
          roster.arrays[5][1]==0 && roster.arrays[5][0]==1 &&
          roster.arrays[6][0]==180 && roster.arrays[6][1]==180,
          "BNW tied team score/rank differs from source double sum");
    for(const auto &player:state.world.all_players->players) {
        const auto &names=player.tail.fields[0];
        const auto &series=player.tail.fields[1];
        if(player.saved_player_id==63) {
            check(names.children.empty() && series.children.empty(),
                  "BNW barbarian replay slot was changed");
            continue;
        }
        check(names.children.size()==1 && series.children.size()==1 &&
              std::string_view(reinterpret_cast<const char*>(
                names.children[0].bytes.data()),names.children[0].bytes.size())==
                "REPLAYDATASET_SCORE" &&
              series.children[0].values.size()==2 &&
              series.children[0].values[0]==5 &&
              series.children[0].values[1]==
                (player.saved_player_id<2?90:0),
              ("BNW score replay was not written for player "+
               std::to_string(player.saved_player_id)).c_str());
    }
    check(!state.world.game_state.flags.front().value,
          "BNW score dirty flag not cleared");
    for(const auto &array:roster.arrays)
        check(array[63]==0,"BNW barbarian ranking slot was changed");
    runtime::update_bnw_game_score(state,rules,types,defines);
    check(state.world.all_players->players[0].tail.fields[1].children[0].values.size()==2,
          "Clean BNW score unexpectedly wrote replay");
    state.world.game_state.flags.front().value=true;
    state.world.all_players->players[0].core.fields.clear();
    bool rejected=false;
    try {runtime::update_bnw_game_score(state,rules,types,defines);}
    catch(const std::runtime_error &) {rejected=true;}
    check(rejected && state.world.game_state.flags.front().value &&
          state.world.game_state.roster->arrays[3][0]==90,
          "Missing BNW score input changed committed ranking");
    if(argc==3) {
        auto selected=ui::bootstrap_selected_bnw_preview(
            std::filesystem::path(argv[1]),std::filesystem::path(argv[2]),0);
        if(selected.preview.blocker)
            throw std::runtime_error(selected.preview.blocker->reason);
        check(selected.preview.host!=nullptr,"Selected BNW host unavailable");
        const auto &host=*selected.preview.host;
        auto real=host.session().state();
        check(real.world.pregame.has_value() &&
              real.world.pregame->archive.integer_arrays.contains("team_types"),
              "Selected BNW pregame team_types unavailable");
        const auto &teams=real.world.pregame->archive.integer_arrays.at("team_types");
        check(teams.size()>=64,"Selected BNW pregame team_types incomplete");
        for(std::size_t id=0;id<64;++id)
            check(teams[id]>=0 && teams[id]<64,
                  "Selected BNW pregame team_types has out-of-range entry");
        auto *dirty=&real.world.game_state.flags.front();
        for(auto &flag:real.world.game_state.flags)
            if(flag.name && std::string_view(flag.name)=="score_dirty")
                dirty=&flag;
        const bool was_dirty=dirty->value;
        dirty->value=true;
        const auto original=real.world.game_state.roster->arrays;
        const auto &p0=real.world.all_players->players[0];
        const auto core=[&](std::string_view name) {
            for(const auto &field:p0.core.fields)
                if(field.name && std::string_view(field.name)==name)
                    return field.values.empty()?INT64_MIN:field.values[0];
            return INT64_MIN;
        };
        const auto gd=[&](std::string_view name) {
            const auto *value=host.defines().find(name);
            return value && std::holds_alternative<std::int64_t>(
                       value->lookup.value)
                ?std::get<std::int64_t>(value->lookup.value):INT64_MIN;
        };
        std::cerr<<"selected player0 inputs alive="<<real.setup.players.at(0).alive
                 <<" core_alive="<<core("m_bAlive")
                 <<" team="<<real.setup.players.at(0).team
                 <<" cities="<<p0.cities.cities.size()
                 <<" pop="<<core("m_iTotalPopulation")
                 <<" land="<<core("m_iTotalLand")
                 <<" wonders="<<core("m_iNumWonders")
                 <<" future="<<core("m_iScoreFromFutureTech")
                 <<" scenario="<<core("m_iScenarioScore1")<<","
                 <<core("m_iScenarioScore2")<<","
                 <<core("m_iScenarioScore3")<<","
                 <<core("m_iScenarioScore4")
                 <<" mapmod="<<std::get<saves::Expansion2PrefixObservation>(
                      real.world.game_state.prefix).scalars[28]
                 <<" citymul="<<gd("SCORE_CITY_MULTIPLIER")
                 <<" popmul="<<gd("SCORE_POPULATION_MULTIPLIER")
                 <<" landmul="<<gd("SCORE_LAND_MULTIPLIER")
                 <<" wondermul="<<gd("SCORE_WONDER_MULTIPLIER")
                 <<" policymul="<<gd("SCORE_POLICY_MULTIPLIER")
                 <<" workmul="<<gd("SCORE_GREAT_WORK_MULTIPLIER")
                 <<" beliefmul="<<gd("SCORE_BELIEF_MULTIPLIER")
                 <<" religionmul="<<gd("SCORE_RELIGION_CITIES_MULTIPLIER")
                 <<" techmul="<<gd("SCORE_TECH_MULTIPLIER")<<'\n';
        const auto selected_types=host.database().cache_info_types();
        runtime::update_bnw_game_score(real,host.database(),selected_types,
                                       host.defines());
        std::cerr<<"selected score rank0 source="
                 <<original[1][0]<<" computed="
                 <<real.world.game_state.roster->arrays[1][0]
                 <<" player0 "<<original[3][0]<<"/"
                 <<real.world.game_state.roster->arrays[3][0]
                 <<" player25 "<<original[3][25]<<"/"
                 <<real.world.game_state.roster->arrays[3][25]<<'\n';
        auto refreshed=std::find_if(real.world.game_state.flags.begin(),
                                   real.world.game_state.flags.end(),
            [](const auto &flag) {
                return flag.name && std::string_view(flag.name)=="score_dirty";
            });
        check(refreshed!=real.world.game_state.flags.end() &&
              !refreshed->value,"Selected BNW score remained dirty");
        const auto &updated=real.world.game_state.roster->arrays;
        for(std::size_t array=1;array<7;++array)
            check(updated[array][63]==original[array][63],
                  "Selected BNW barbarian ranking slot changed");
        for(std::size_t rank=0;rank<63;++rank) {
            const auto player=updated[1][rank],team=updated[4][rank];
            check(player>=0 && player<63 && team>=0 && team<63 &&
                  updated[2][player]==static_cast<std::int64_t>(rank) &&
                  updated[5][team]==static_cast<std::int64_t>(rank),
                  "Selected BNW ranking inverse mismatch");
            if(rank) {
                const auto previous_player=updated[1][rank-1];
                const auto previous_team=updated[4][rank-1];
                check(updated[3][previous_player]>=updated[3][player] &&
                      updated[6][previous_team]>=updated[6][team],
                      "Selected BNW score ranking order mismatch");
            }
        }
        std::cout<<"selected BNW score recalculated; source_dirty="
                 <<was_dirty<<" turn="<<real.setup.game_turn
                 <<" player0="<<original[3][0]<<"/"<<updated[3][0]<<'\n';
    }
}
