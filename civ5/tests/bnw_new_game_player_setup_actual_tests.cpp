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
    refused([&]{(void)game->civis_player_setup_observation();});game->initialize_civis_team_roster_prefix(roster);const auto actual=game->civis_player_setup_observation();
    need(actual.boundary==runtime::BnwNewGamePlayerSetupBoundary::player_reset_required&&actual.initialized_teams==64&&actual.teams.size()==80&&
         game->observe().boundary==runtime::BnwFreshGameBoundary::civis_player_reset_required&&!actual.mandatory.empty(),"Genuine owned team/roster prefix boundary missing");
    const auto technologies=activated->rules->prefetch_info("Technologies",{},content::InfoCollectionBudget{65536});need(technologies.all_succeeded(),"Actual selected technology span unavailable");
    for(std::size_t i=0;i<80;++i){const auto& team=actual.teams[i];need(team.fixed.id==static_cast<std::int32_t>(i)&&team.fixed.num_members==0&&team.fixed.alive_count==0&&team.fixed.ever_alive_count==0,
         "Team constructor/init or premature player membership differs");
        need(team.fixed.relations.size()==64&&team.fixed.found_players_territory.size()==64,"Native fixed relation capacities differ");
        if(i<64){need(team.fixed.dynamic_info_arrays_allocated&&team.has_tech.size()==technologies.slots&&team.research_progress.size()==technologies.slots&&team.last_tech_acquired==-1,"Actual selected TeamTechs reset missing");
            for(const auto progress:team.research_progress)need(progress==0,"Initial research progress nonzero");for(const auto countdown:team.victory_countdown)need(countdown==-1,"Initial victory countdown differs");}
        else need(!team.fixed.dynamic_info_arrays_allocated&&team.has_tech.empty(),"Original constructor-only extra Team was initialized early");
    }
    const auto native_define=[&](const char* name){const auto value=activated->rules->global_define(name,content::GlobalDefineKind::integer);need(value.found&&value.error.empty(),"Genuine BNW PostDefine missing");return static_cast<std::int32_t>(std::get<std::int64_t>(value.value));};
    need(actual.roster[0].status==runtime::BnwFreshSlotStatus::taken&&actual.roster[0].team==0&&actual.roster[0].handicap==config->metadata().info[5].id,"Human actual selected roster lost");
    need(actual.roster[1].handicap==native_define("AI_HANDICAP")&&actual.roster[1].team==0&&actual.roster[2].team==21,"AI handicap/observer team original writes differ");
    need(actual.roster[22].minor&&actual.roster[22].status==runtime::BnwFreshSlotStatus::computer&&actual.roster[22].civilization==native_define("MINOR_CIVILIZATION")&&actual.roster[22].team==22,"Actual minor slot write differs");
    need(!actual.roster[23].minor&&actual.roster[23].status==runtime::BnwFreshSlotStatus::closed&&actual.roster[23].team==23,"Inactive slot native reset defaults differ");
    need(actual.roster[63].team==63&&actual.roster[63].status==runtime::BnwFreshSlotStatus::computer&&!actual.roster[63].minor&&actual.roster[63].civilization==native_define("BARBARIAN_CIVILIZATION")&&actual.roster[63].leader==native_define("BARBARIAN_LEADER"),"Actual barbarian source writes differ");
    need(actual.color_workspace[0]!=actual.color_workspace[1]&&actual.color_workspace[0]!=actual.color_workspace[2]&&actual.color_workspace[1]!=actual.color_workspace[2],"Original duplicate color loop not applied");
    for(std::size_t i=22;i<80;++i)need(actual.color_workspace[i]==-1,"Original color workspace extended major pass incorrectly");
    need(game->civis_setup_observation().map_random==rng.map_random&&game->civis_setup_observation().gameplay_random==rng.gameplay_random,"Team/explicit roster prefix consumed RNG");
    refused([&]{game->initialize_civis_team_roster_prefix(roster);});runtime::BnwFreshGame copy=*game;refused([&]{(void)copy.civis_player_setup_observation();});
    bool foreign=false;std::thread other([&]{try{(void)game->civis_player_setup_observation();}catch(const std::exception&){foreign=true;}});other.join();need(foreign,"Foreign player setup thread accepted");
    activated->rules.reset();refused([&]{(void)game->civis_player_setup_observation();});
    std::cout<<"Actual BNW teams64/constructor80, chosen roster/colors/PostDefines; actual Player Reset/setAlive/AI and map publication mandatory; can_continue=0\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
