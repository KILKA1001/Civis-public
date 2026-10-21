#include "civ5/runtime/bnw_new_game_setup_rules.hpp"
#include <array>
#include <iostream>
#include <thread>
using namespace civ5;
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void refused(F&& f){bool result=false;try{f();}catch(const std::exception&){result=true;}need(result,"Incomplete source issued genuine gameplay setup rules");}
}
int main(int argc,char** argv){try{
    need(argc==2,"Expected genuine installed BNW root");
    content::Catalog catalog;
    const std::filesystem::path root=argv[1];
    catalog.packages.push_back(content::read_manifest(root/"Assets/DLC/Expansion/Expansion1.Civ5Pkg"));
    catalog.packages.push_back(content::read_manifest(root/"Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
    constexpr std::array<std::size_t,2> dlc{0,1};
    auto actual=runtime::bootstrap_bnw_fresh_game_content_owner(root,catalog,dlc);
    need(actual&&actual->content_ready(),"Real BNW activation failed");
    const runtime::BnwCreationChoices choices{"WORLDSIZE_STANDARD","CLIMATE_TEMPERATE","SEALEVEL_MEDIUM","GAMESPEED_STANDARD","ERA_ANCIENT","HANDICAP_PRINCE"};
    auto config=runtime::BnwCreationConfig::from_selected_rules(actual,choices);
    const auto rules=runtime::BnwNewGameSetupRules::from_creation_config(config);const auto data=rules->observe();
    need(data.selected.info[3].type=="GAMESPEED_STANDARD"&&data.selected.info[4].type=="ERA_ANCIENT"&&!data.turns.empty()&&!data.victories.empty(),
         "Actual BNW setup operands missing");
    std::int64_t expected_turns=0;const auto q=actual->rules->query_rows(
        {"SELECT TurnsPerIncrement FROM GameSpeed_Turns WHERE GameSpeedType = ?",{data.selected.info[3].type}},
        [&](const content::RulesRowView& row){expected_turns+=row.integer("TurnsPerIncrement");});
    std::int64_t observed_turns=0;for(const auto& increment:data.turns)observed_turns+=increment.turns;
    need(q.all_succeeded()&&expected_turns==observed_turns,"Actual SDK turn increment source order/values differ");
    bool end_score=false;for(const auto& victory:data.victories)if(victory&&victory->end_score)end_score=true;
    need(end_score&&!data.no_religion&&data.start_percent==0,"Actual stock Ancient/victory operands differ");
    // Equal public reports/rules are intentionally insufficient: no genuine
    // factory epoch or activation occurrence is issued by a supplied facade.
    auto copied=std::make_shared<runtime::BnwFreshGameContentResult>();copied->profile=actual->profile;
    copied->rules=actual->rules;copied->activation=actual->activation;copied->blockers=actual->blockers;
    auto diagnostic=runtime::BnwCreationConfig::from_selected_rules(copied,choices);
    refused([&]{(void)runtime::BnwNewGameSetupRules::from_creation_config(diagnostic);});
    bool foreign=false;std::thread other([&]{try{rules->require_current();}catch(const std::exception&){foreign=true;}});other.join();need(foreign,"Foreign gameplay setup thread accepted");
    rules->require_current();actual->rules.reset();refused([&]{rules->require_current();});
    std::cout<<"Actual BNW gameplay setup rules: SDK turn/era/victory sources; forged Content/retirement refused; players=uninitialized map=uncreated can_continue=0\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
