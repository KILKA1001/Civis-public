#include "civ5/runtime/bnw_fresh_game.hpp"
#include <array>
#include <iostream>
#include <thread>
using namespace civ5;
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void refused(F&& f){bool result=false;try{f();}catch(const std::exception&){result=true;}need(result,"Incomplete BNW player source was accepted");}
}
int main(int argc,char** argv){try{
    need(argc==2,"Expected genuine installed BNW root");content::Catalog catalog;const std::filesystem::path root=argv[1];
    catalog.packages.push_back(content::read_manifest(root/"Assets/DLC/Expansion/Expansion1.Civ5Pkg"));catalog.packages.push_back(content::read_manifest(root/"Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
    constexpr std::array<std::size_t,2> dlc{0,1};auto activated=runtime::bootstrap_bnw_fresh_game_content_owner(root,catalog,dlc);need(activated&&activated->content_ready(),"Actual installed BNW activation failed");
    const runtime::BnwCreationChoices selected{"WORLDSIZE_STANDARD","CLIMATE_TEMPERATE","SEALEVEL_MEDIUM","GAMESPEED_STANDARD","ERA_ANCIENT","HANDICAP_PRINCE"};
    auto config=runtime::BnwCreationConfig::from_selected_rules(activated,selected);auto game=runtime::BnwFreshGame::from_creation_config(config);
    runtime::BnwNewGameSetupRequest setup;setup.map_source={"Assets/Maps/Continents.lua",std::nullopt,std::nullopt};game->initialize_civis_single_player_setup(setup);
    const auto rng=game->civis_setup_observation();runtime::BnwNewGamePlayerSetupRequest roster;
    roster.majors[0]={runtime::BnwFreshSlotStatus::taken,"CIVILIZATION_AMERICA","LEADER_WASHINGTON",std::nullopt,std::nullopt};
    roster.majors[1]={runtime::BnwFreshSlotStatus::computer,"CIVILIZATION_AMERICA","LEADER_WASHINGTON",0,std::nullopt};
    roster.majors[2]={runtime::BnwFreshSlotStatus::observer,"CIVILIZATION_AMERICA","LEADER_WASHINGTON",0,std::nullopt};
    roster.minors.push_back({"MINOR_CIV_GENEVA",std::nullopt});
    game->initialize_civis_team_roster_prefix(roster);game->advance_civis_first_player_reset_prefix();
    const auto actual=game->civis_player_setup_observation();
    need(actual.boundary==runtime::BnwNewGamePlayerSetupBoundary::first_player_homeland_required&&
         game->observe().boundary==runtime::BnwFreshGameBoundary::civis_first_player_homeland_required&&!actual.mandatory.empty(),"Actual Tactical normal return lacks mandatory Homeland frontier");
    need(actual.first_player_id==0&&actual.first_player_tactical_priority_turn==-1&&actual.first_player_tactical_reset_map_null==true&&actual.first_player_tactical_game_map_bound,"Existing actual Reset or SAME Game Tactical pointer was not retained");
    const std::array<std::pair<const char*,const char*>,4> names{{{"recruit_range","AI_TACTICAL_RECRUIT_RANGE"},{"reposition_range","AI_TACTICAL_REPOSITION_RANGE"},{"deploy_radius","AI_OPERATIONAL_CITY_ATTACK_DEPLOY_RANGE"},{"random_range","AI_TACTICAL_MOVE_PRIORITY_RANDOMNESS"}}};
    for(const auto& [field,source]:names){const auto value=activated->rules->global_define(source,content::GlobalDefineKind::integer);need(value.error.empty()&&actual.first_player_tactical_integers.at(field)==std::get<std::int64_t>(value.value),"Actual cached Tactical integer differs from selected Rules");}
    const auto handicap=activated->rules->query_rows({"SELECT BarbarianLandTargetRange, BarbarianSeaTargetRange FROM HandicapInfos WHERE ID = ? LIMIT 1",{static_cast<std::int64_t>(config->metadata().info[5].id)}},[&](const content::RulesRowView& row){need(actual.first_player_tactical_integers.at("land_barbarian_range")==row.integer_at(0)&&actual.first_player_tactical_integers.at("sea_barbarian_range")==row.integer_at(1),"Actual stored Game Handicap reads differ");},1);
    need(handicap.all_succeeded()&&handicap.rows==1,"Actual selected Handicap unavailable");
    const auto flavor=activated->rules->global_define("AI_TACTICAL_FLAVOR_DAMPENING_FOR_MOVE_PRIORITIZATION",content::GlobalDefineKind::source_float);
    need(flavor.error.empty()&&actual.first_player_tactical_flavor_dampening==static_cast<float>(std::get<double>(flavor.value)),"Native source-float dampening differs");
    const auto types=activated->rules->cache_info_types();need(types.report().all_succeeded(),"Actual native type lookup failed");
    need(actual.first_player_tactical_cached_types.size()==55&&actual.first_player_tactical_cached_types.front().first=="eTACTICAL_UNASSIGNED"&&actual.first_player_tactical_cached_types.back().first=="eMUPOSITION_FRONT_LINE","SDK Tactical cache assignment extent/order differs");
    const auto* unassigned=types.find("TACTICAL_UNASSIGNED");need(actual.first_player_tactical_cached_types.front().second==(unassigned?unassigned->id:-1),"Actual first Tactical cached native ID differs");
    for(const auto& team:actual.teams)need(team.fixed.num_members==0&&team.fixed.alive_count==0&&team.fixed.ever_alive_count==0,"Player Reset prefix prematurely performed setAlive/team membership");
    need(game->civis_setup_observation().map_random==rng.map_random&&game->civis_setup_observation().gameplay_random==rng.gameplay_random,"Actual Tactical Init consumed RNG");
    refused([&]{game->advance_civis_first_player_reset_prefix();});
    refused([&]{game->initialize_civis_team_roster_prefix(roster);});runtime::BnwFreshGame copy=*game;refused([&]{(void)copy.civis_player_setup_observation();});
    bool foreign=false;std::thread other([&]{try{(void)game->civis_player_setup_observation();}catch(const std::exception&){foreign=true;}});other.join();need(foreign,"Foreign player setup thread accepted");
    activated->rules.reset();refused([&]{(void)game->civis_player_setup_observation();});
    std::cout<<"Actual retained first Player Reset and SAME Game Tactical Init; Homeland/full Reset/setAlive/AI and map publication mandatory; can_continue=0\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
