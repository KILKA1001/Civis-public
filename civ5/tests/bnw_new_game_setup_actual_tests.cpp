#include "civ5/runtime/bnw_fresh_game.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <thread>
using namespace civ5;
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void refused(F&& f){bool result=false;try{f();}catch(const std::exception&){result=true;}need(result,"Mandatory BNW setup refusal missing");}
runtime::BnwNewGameSetupRequest choices(){
    runtime::BnwNewGameSetupRequest value;
    value.map_seed=std::numeric_limits<std::uint32_t>::max();value.sync_seed=0;
    value.map_source={"Assets/Maps/Continents.lua",std::nullopt,std::nullopt};
    value.map_custom_options={{1,2},{2,2},{3,2},{4,2},{5,2}};return value;
}
}
int main(int argc,char** argv){try{
    need(argc==2,"Expected genuine installed BNW root");content::Catalog catalog;const std::filesystem::path root=argv[1];
    catalog.packages.push_back(content::read_manifest(root/"Assets/DLC/Expansion/Expansion1.Civ5Pkg"));
    catalog.packages.push_back(content::read_manifest(root/"Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
    constexpr std::array<std::size_t,2> dlc{0,1};auto activated=runtime::bootstrap_bnw_fresh_game_content_owner(root,catalog,dlc);
    need(activated&&activated->content_ready(),"Real BNW activation failed");
    const runtime::BnwCreationChoices selected{"WORLDSIZE_STANDARD","CLIMATE_TEMPERATE","SEALEVEL_MEDIUM","GAMESPEED_STANDARD","ERA_ANCIENT","HANDICAP_PRINCE"};
    const auto config=runtime::BnwCreationConfig::from_selected_rules(activated,selected);
    const auto rules=runtime::BnwNewGameSetupRules::from_creation_config(config)->observe();
    std::int64_t total=0;const auto total_source=activated->rules->query_rows(
        {"SELECT TurnsPerIncrement FROM GameSpeed_Turns WHERE GameSpeedType = ?",{rules.selected.info[3].type}},
        [&](const content::RulesRowView& row){total+=row.integer("TurnsPerIncrement");});
    need(total_source.all_succeeded()&&total>0&&total<=std::numeric_limits<std::int32_t>::max(),"Actual original speed end-turn source unavailable");
    auto game=runtime::BnwFreshGame::from_creation_config(config);const auto before=game->observe();auto request=choices();
    request.game_options={{"GAMEOPTION_SIMULTANEOUS_TURNS",true},{"GAMEOPTION_DYNAMIC_TURNS",true},{"GAMEOPTION_PITBOSS",true},{"GAMEOPTION_LOCK_MODS",true},{"GAMEOPTION_QUICK_COMBAT",true}};
    game->initialize_civis_single_player_setup(request);const auto actual=game->civis_setup_observation();const auto allocated=game->observe();
    need(actual.boundary==runtime::BnwNewGameSetupBoundary::players_init_required&&allocated.boundary==runtime::BnwFreshGameBoundary::civis_player_init_required,
         "Actual Ancient init did not reach mandatory InitPlayers");
    need(actual.game_turn==0&&!actual.pending_game_turn&&actual.start_turn==0&&actual.default_end_turn==total&&actual.estimated_end_turn==total&&actual.max_turns==total&&actual.start_year==rules.start_year,
         "Actual Ancient clock/EndScore initialization differs from original BNW");
    need(actual.map_source.relative_path==request.map_source.relative_path&&actual.map_custom_options==request.map_custom_options&&
         actual.selected.info[0].id==rules.selected.info[0].id&&actual.selected.rules_sha256==rules.selected.rules_sha256,"Owned actual selection/map choices changed");
    need(!actual.game_options.at("GAMEOPTION_SIMULTANEOUS_TURNS")&&!actual.game_options.at("GAMEOPTION_DYNAMIC_TURNS")&&!actual.game_options.at("GAMEOPTION_PITBOSS")&&actual.quick_combat,
         "Single-player MP clearing/QuickCombat differs");
    // Independent direct reference to qualified original CvRandom.cpp Win32
    // arithmetic and CvGame::init's seven password draws. Embedded NUL affects
    // the password string, never the count or final seed.
    std::uint32_t seed=request.sync_seed%52319761u;std::array<char,8> password{};
    for(std::size_t i=0;i<7;++i){seed=1103515245u*seed+12345u;password[i]=static_cast<char>(((seed>>16)&65535u)*128u/65536u);}
    need(password[0]==0&&actual.admin_password.empty(),"Real first-NUL password case not exercised");
    need(actual.gameplay_random.seed==seed&&actual.gameplay_random.calls==before.gameplay_random.calls+7&&actual.admin_password==std::string(password.data()),
         "LOCK_MODS original seven-draw RNG/password contract differs");
    need(actual.map_random.seed==request.map_seed%73637381u&&actual.map_random.calls==before.map_random.calls&&
         actual.map_random.resets==before.map_random.resets+2&&actual.gameplay_random.resets==before.gameplay_random.resets+2,
         "Original reset/init RNG residues/counts differ");
    need(allocated.cache_resets==before.cache_resets+1&&actual.goody_memory&&actual.goody_memory->size()==22,"Actual Reset/cache/goody allocation missing");
    for(const auto& row:*actual.goody_memory)need(row==std::array<std::int32_t,3>{-1,-1,-1},"Original goody reset values differ");
    need(std::get<bool>(allocated.scalars.at("m_bFinalInitialized"))==false&&!actual.mandatory.empty(),"Partial setup falsely initialized the world");
    need(allocated.arrays.at("m_aiVotesCast").slots->size()==63&&allocated.arrays.at("m_aiPreviousVotesCast").slots->size()==63,
         "Qualified native team capacity differs");
    refused([&]{game->initialize_civis_single_player_setup(request);});refused([&]{game->advance_reset_prefix();});
    need(game->civis_setup_observation().gameplay_random==actual.gameplay_random,"Repeated setup advanced RNG");
    runtime::BnwFreshGame copied=*game;refused([&]{(void)copied.civis_setup_observation();});
    bool foreign=false;std::thread other([&]{try{(void)game->civis_setup_observation();}catch(const std::exception&){foreign=true;}});other.join();need(foreign,"Foreign setup thread accepted");
    auto no_score=runtime::BnwFreshGame::from_creation_config(config);auto no_score_request=choices();
    for(const auto& victory:rules.victories)if(victory&&victory->end_score)no_score_request.disabled_victories.insert(victory->identity.type);
    need(!no_score_request.disabled_victories.empty(),"Actual installed EndScore victory source missing");
    no_score->initialize_civis_single_player_setup(no_score_request);const auto without_score=no_score->civis_setup_observation();
    need(without_score.max_turns==0&&without_score.estimated_end_turn==total&&without_score.gameplay_random.calls==0,"Disabled EndScore incorrectly installed a turn limit or drew RNG");
    auto limited=runtime::BnwFreshGame::from_creation_config(config);auto limited_request=choices();limited_request.max_turns=17;
    limited->initialize_civis_single_player_setup(limited_request);need(limited->civis_setup_observation().estimated_end_turn==17,"Explicit maxTurns source lost");
    auto invalid=runtime::BnwFreshGame::from_creation_config(config);const auto invalid_before=invalid->observe();auto invalid_request=choices();invalid_request.game_options["GAMEOPTION_NOT_AUTHORED_IN_BNW"]=true;
    refused([&]{invalid->initialize_civis_single_player_setup(invalid_request);});refused([&]{(void)invalid->civis_setup_observation();});
    need(invalid->observe().cache_resets==invalid_before.cache_resets&&invalid->observe().gameplay_random==invalid_before.gameplay_random,"Invalid choice changed actual Game before validation");
    auto later_choices=selected;later_choices.era="ERA_CLASSICAL";
    auto later_config=runtime::BnwCreationConfig::from_selected_rules(activated,later_choices);const auto later_rules=runtime::BnwNewGameSetupRules::from_creation_config(later_config)->observe();
    need(later_rules.start_percent>0,"Actual Classical start-turn source absent");auto later=runtime::BnwFreshGame::from_creation_config(later_config);
    refused([&]{later->initialize_civis_single_player_setup(choices());});const auto pending=later->civis_setup_observation();
    need(pending.boundary==runtime::BnwNewGameSetupBoundary::turn_publication_required&&pending.pending_game_turn&&*pending.pending_game_turn>0&&pending.game_turn==0&&!pending.start_turn&&!pending.start_year&&!pending.goody_memory,
         "Later-era mandatory publication was skipped or initial tail ran early");
    activated->rules.reset();refused([&]{(void)game->civis_setup_observation();});
    std::cout<<"Actual installed BNW setup: owned reset/services, seed/password/turn/victory/goody parity; mandatory players/map and later-era publication retained; can_continue=0\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
