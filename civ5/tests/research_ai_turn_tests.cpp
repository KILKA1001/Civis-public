#include "civ5/content/rules_database.hpp"
#include "civ5/content/research_rules.hpp"
#include "civ5/runtime/lua_research_services.hpp"
#include "civ5/runtime/research_ai_candidates.hpp"
#include "civ5/runtime/vp_city_yield_cache_profile.hpp"
#include "civ5/runtime/vp_maintenance_profile.hpp"
#include "civ5/runtime/vp_player_research_profile.hpp"
#include "civ5/runtime/vp_research_ai_weights.hpp"
#include "civ5/runtime/vp_science_profile.hpp"
#include "civ5/runtime/vp_treasury_income_profile.hpp"
#include "civ5/runtime/vp_treasury_minor_profile.hpp"
#include "civ5/runtime/vp_treasury_religion_profile.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5::content;
using namespace civ5::runtime;
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
struct Host final:ResearchProgressServices,ResearchTurnServices {
    std::int64_t ai_fallback_calls=0,ui_calls=0;
    std::int64_t science_times100(std::int64_t) override {throw std::logic_error("Turn bypassed live science");}
    void game_data_and_score_dirty(std::int64_t) override {}
    void acquire_technology(std::int64_t,std::int64_t,std::int64_t) override {throw std::logic_error("Unexpected technology completion");}
    void city_specializations_dirty(std::int64_t) override {}
    std::int64_t culture_per_tech(std::int64_t) override {return 0;}
    void add_culture(std::int64_t,std::int64_t) override {}
    void choose_research_ai(std::int64_t) override {++ai_fallback_calls;throw std::logic_error("AI fallback used");}
    void choose_research_ui(std::int64_t) override {++ui_calls;}
    void research_buttons_dirty(std::int64_t) override {}
    void unlock_research_achievement(std::int64_t,ResearchAchievement) override {}
};
void run(TechnologyProfile profile) {
    const bool vp=profile==TechnologyProfile::vp542;
    RulesDatabase db;
    db.apply_sql("CREATE TABLE HandicapInfos(ID,Type,ResearchPercent,ResearchPerEraModifier,AIResearchPercent,AIResearchPerEraModifier,TechNumOptionsConsidered,TechChoiceCutoffThreshold);"
        "INSERT INTO HandicapInfos VALUES(0,'H0',100,0,100,0,2,50);"
        "CREATE TABLE GameSpeeds(ID,Type,ResearchPercent);INSERT INTO GameSpeeds VALUES(0,'S0',100);"
        "CREATE TABLE Eras(ID,Type,ResearchPercent);INSERT INTO Eras VALUES(0,'E0',100);"
        "CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES"
        "('NUM_OR_TECH_PREREQS',1),('TECH_COST_TOTAL_KNOWN_TEAM_MODIFIER',0),"
        "('TECH_COST_KNOWN_PREREQ_MODIFIER',0),('NUM_CITIES_COST_MOD_SCALING',100),"
        "('RESEARCH_AGREEMENT_MOD',0),('UNIT_MAINTENANCE_GAME_MULTIPLIER',8),"
        "('UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR',7),('INITIAL_FREE_OUTSIDE_UNITS',2),"
        "('INITIAL_OUTSIDE_UNIT_GOLD_PERCENT',50),"
        "('AI_RESEARCH_WEIGHT_BASE_MOD',0.001),('AI_RESEARCH_WEIGHT_MOD_PER_TURN_LEFT',0.00001);");
    const auto defines=db.cache_global_defines(vp?GlobalDefineProfile::vp542:GlobalDefineProfile::bnw);
    const auto rules=load_research_rules(db,profile);
    TechnologyRules techs;techs.profile=profile;techs.collection.slots=2;
    for(int i=0;i<2;++i){auto &tech=techs.entries[i];tech.integers={{"Cost",10000},{"EurekaPerMillion",0}};
        tech.booleans={{"Repeat",false},{"Disable",false}};
        tech.or_prerequisites.extent=1;tech.or_prerequisites.default_value=-1;}
    ResearchContext setup;PlayerResearchContext players;ScienceContext science;TreasuryContext treasury;
    MaintenanceContext maintenance;CityYieldContext cities;TreasuryRules tr;CityYieldRules cr;
    MaintenanceRules mr;ResearchTurnState turn_state;ResearchSelectionState selection_state;
    setup.profile=players.profile=science.profile=treasury.profile=maintenance.profile=cities.profile=
        tr.profile=cr.profile=mr.profile=turn_state.profile=selection_state.profile=profile;
    setup.speed=setup.start_era=setup.game_handicap=0;setup.world_percent=100;setup.game_turn=10;
    setup.map_random_seed=123456;
    for(std::int64_t id=0;id<2;++id){
        setup.players[id]={0,0,id==0?3:1,0,true};
        players.players[id]={};players.players[id].cities[0]={};
        science.players[id]={};treasury.players[id].balance_times100=100000;
        cities.players[id].excess_happiness=0;maintenance.players[id]={};
        turn_state.players[id].cities_founded=1;turn_state.players[id].founded_first_city=true;
        turn_state.players[id].leader_type="LEADER_TEST";
    }
    setup.team_members[0]=2;players.major_players={0,1};players.civ_teams={0};
    players.teams[0].members={0,1};players.teams[0].alive_count=2;
    players.players[1].queue={1};
    science.research_agreements[0]={};cities.yield_slots=17;cities.no_happiness=true;
    maintenance.default_end_turn=maintenance.estimate_end_turn=500;maintenance.elapsed_turns=5;
    maintenance.team_eras[0]=0;
    mr.handicaps[0]={0,0,100,100,100,100,100,100,100,100,0};mr.era_building_percent[0]=100;
    CustomModOptions options;options.prefetch.all_succeeded=true;
    if(vp){
        options.entries["AI_CONTROL_TECH_CHOICE"].value=1;
        ensure_vp_maintenance_rules_profile(mr);ensure_vp_maintenance_context_profile(maintenance);
        for(auto id:{0,1}){
            players.players[id].profile_extensions.emplace<VpPlayerResearchProfileState>();
            ensure_vp_science_profile(science.players[id]);
            ensure_vp_treasury_income_profile(treasury.players[id]);
            ensure_vp_treasury_minor_profile(treasury.players[id]);
            ensure_vp_treasury_religion_profile(treasury.players[id]);
            ensure_vp_maintenance_profile(maintenance.players[id],profile);
        }
    }
    std::vector<TeamTechnologies> teams;teams.emplace_back(profile,0,2);
    MaintenanceWorld economy{setup,players,cities,teams};Host host;
    ResearchProgressContext progress{players,setup,rules,techs,defines,vp?&options:nullptr,teams,host,-1,false};
    ResearchTurnContext turn{turn_state,progress,science,economy,treasury,maintenance,tr,cr,mr,host,-1,{}};
    LuaScriptHost scripts;scripts.complete_loading();
    LuaResearchSelectionServices selection_host(scripts,[](auto){});
    selection_state.players[0].civilization="CIV_TEST";
    selection_state.disabled_technologies["CIV_TEST"]={};
    ResearchSelectionContext selection{selection_state,turn_state,players,setup,techs,vp?&options:nullptr,
        teams,maintenance.team_eras,{0,0},selection_host,-1,false};
    ResearchAiWeights weights;weights.profile=profile;
    SimulationRandom rng(vp?RandomAlgorithm::vp_reference:RandomAlgorithm::bnw,{17,0,0});
    ResearchAiChoiceContext choice{selection,turn,weights,db,scripts,rng};
    const auto result=do_research_with_ai(choice,0);
    check(result.research_enabled&&host.ai_fallback_calls==0,"Real chooser did not run inside research turn");
    check(vp?(players.players[0].current_research()==1&&!result.forced_choice):
        (players.players[0].queue.empty()&&result.forced_choice),
        "BNW and VP must retain their distinct AI choice timing");
    check(rng.state().calls==0,"Teammate adoption must not consume gameplay random");
    check(players.players[1].current_research()==1,"Research turn changed the teammate project");
    // No teammate project: the active Lua mod vetoes tech 1, and the saved
    // TechAI weights plus live science must select tech 0 on either profile.
    players.players[0].queue.clear();players.players[1].queue.clear();
    turn_state.players[0].researching.clear();teams[0].set_progress_times100(0,0);
    auto &city=cities.players[0].cities[0];city.id=0;city.owner=0;
    city.yields[3].terrain=10;
    if(vp)vp_city_cached_yields(cities,0,0)[3]=1000;
    auto &saved=weights.players[0];saved.weight_slots=2;saved.weights={{0,500},{1,200}};
    if(vp)saved.profile_extensions.emplace<VpResearchAiVector>();
    scripts.execute("data-mod","GameEvents.PlayerCanResearch.Add(function(player, tech) return tech ~= 1 end)","=data-mod.lua");
    scripts.complete_loading();
    techs.entries[0].era=1;
    const auto free_candidates=build_research_ai_candidates(selection,turn,weights,0,true);
    check(free_candidates.choices.size()==1&&free_candidates.choices[0].technology==0,
        "Lua mod veto must apply to free-technology candidates");
    if(vp){
        check(free_candidates.prefers_expensive,"VP free technology must prefer the expensive candidate");
        saved.profile_extensions.find<VpResearchAiVector>()->permanent_yields_decrease_every_era=true;
        const auto penalized=build_research_ai_candidates(selection,turn,weights,0,true);
        check(penalized.choices[0].adjusted_weight==
            std::max<std::int64_t>(1,free_candidates.choices[0].adjusted_weight/2),
            "VP future-era trait must stay inside the profile scorer");
        saved.profile_extensions.find<VpResearchAiVector>()->permanent_yields_decrease_every_era=false;
        players.major_players.erase(0);
        check(build_research_ai_candidates(selection,turn,weights,0,false).choices.empty(),
            "VP must exclude a player outside its major-player TechAI domain");
        players.major_players.insert(0);
    }else check(free_candidates.choices[0].adjusted_weight==500&&free_candidates.choices[0].turns==0,
        "BNW free technology must retain the saved weight without VP reweighting");
    const auto weighted=do_research_with_ai(choice,0);
    check(host.ai_fallback_calls==0,"Weighted turn bypassed the bound chooser");
    check(vp?(players.players[0].current_research()==0):
        (weighted.technology==0&&players.players[0].queue.empty()),
        "Lua veto and saved TechAI weights did not select the permitted technology");
    check(rng.state().calls==(vp?0U:1U),"GameCore-specific weighted draw used the wrong random stream");
    const auto before_queue=players.players[0].queue;
    const auto before_progress=teams[0].progress_times100(0);
    weights.profile=vp?TechnologyProfile::bnw:TechnologyProfile::vp542;
    bool rejected=false;
    try{do_research_with_ai(choice,0);}catch(const ResearchAiUnavailable &){rejected=true;}
    check(rejected&&players.players[0].queue==before_queue&&teams[0].progress_times100(0)==before_progress,
        "Mismatched GameCore binding must fail before changing the live research state");
}
// F01: exercise the actual public chooser, candidate/scoring/selection and
// production RNG through the existing BNW common-world fixture. Exponent zero
// keeps exact adjusted weights; it does not assume an incoming x87 FP mode.
void run_bnw_native_bound_regression() {
    constexpr auto profile=TechnologyProfile::bnw;
    RulesDatabase db;
    db.apply_sql("CREATE TABLE HandicapInfos(ID,Type,ResearchPercent,ResearchPerEraModifier,AIResearchPercent,AIResearchPerEraModifier,TechNumOptionsConsidered,TechChoiceCutoffThreshold);"
        "INSERT INTO HandicapInfos VALUES(0,'H0',100,0,100,0,2,50);"
        "CREATE TABLE GameSpeeds(ID,Type,ResearchPercent);INSERT INTO GameSpeeds VALUES(0,'S0',100);"
        "CREATE TABLE Eras(ID,Type,ResearchPercent);INSERT INTO Eras VALUES(0,'E0',100);"
        "CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES"
        "('NUM_OR_TECH_PREREQS',1),('TECH_COST_TOTAL_KNOWN_TEAM_MODIFIER',0),"
        "('TECH_COST_KNOWN_PREREQ_MODIFIER',0),('NUM_CITIES_COST_MOD_SCALING',100),"
        "('RESEARCH_AGREEMENT_MOD',0),('UNIT_MAINTENANCE_GAME_MULTIPLIER',8),"
        "('UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR',7),('INITIAL_FREE_OUTSIDE_UNITS',2),"
        "('INITIAL_OUTSIDE_UNIT_GOLD_PERCENT',50),"
        "('AI_RESEARCH_WEIGHT_BASE_MOD',0),('AI_RESEARCH_WEIGHT_MOD_PER_TURN_LEFT',0);");
    const auto defines=db.cache_global_defines(GlobalDefineProfile::bnw);
    const auto rules=load_research_rules(db,profile);
    TechnologyRules techs;techs.profile=profile;techs.collection.slots=2;
    for(int i=0;i<2;++i){auto &tech=techs.entries[i];tech.integers={{"Cost",10000},{"EurekaPerMillion",0}};
        tech.booleans={{"Repeat",false},{"Disable",false}};
        tech.or_prerequisites.extent=1;tech.or_prerequisites.default_value=-1;}
    ResearchContext setup;PlayerResearchContext players;ScienceContext science;TreasuryContext treasury;
    MaintenanceContext maintenance;CityYieldContext cities;TreasuryRules tr;CityYieldRules cr;
    MaintenanceRules mr;ResearchTurnState turn_state;ResearchSelectionState selection_state;
    setup.profile=players.profile=science.profile=treasury.profile=maintenance.profile=cities.profile=
        tr.profile=cr.profile=mr.profile=turn_state.profile=selection_state.profile=profile;
    setup.speed=setup.start_era=setup.game_handicap=0;setup.world_percent=100;setup.game_turn=10;
    setup.players[0]={0,0,1,0,true};setup.team_members[0]=1;
    players.players[0]={};players.players[0].cities[0]={};
    players.major_players={0};players.civ_teams={0};players.teams[0].members={0};players.teams[0].alive_count=1;
    science.players[0]={};science.research_agreements[0]={};treasury.players[0].balance_times100=100000;
    cities.yield_slots=17;cities.no_happiness=true;cities.players[0].excess_happiness=0;
    auto &city=cities.players[0].cities[0];city.id=0;city.owner=0;city.yields[3].terrain=10;
    maintenance.players[0]={};maintenance.default_end_turn=maintenance.estimate_end_turn=500;maintenance.elapsed_turns=5;
    maintenance.team_eras[0]=0;mr.handicaps[0]={0,0,100,100,100,100,100,100,100,100,0};mr.era_building_percent[0]=100;
    turn_state.players[0].cities_founded=1;turn_state.players[0].founded_first_city=true;
    turn_state.players[0].leader_type="LEADER_TEST";
    std::vector<TeamTechnologies> teams;teams.emplace_back(profile,0,2);
    MaintenanceWorld economy{setup,players,cities,teams};Host host;
    ResearchProgressContext progress{players,setup,rules,techs,defines,nullptr,teams,host,-1,false};
    ResearchTurnContext turn{turn_state,progress,science,economy,treasury,maintenance,tr,cr,mr,host,-1,{}};
    LuaScriptHost scripts;scripts.complete_loading();LuaResearchSelectionServices selection_host(scripts,[](auto){});
    selection_state.players[0].civilization="CIV_TEST";selection_state.disabled_technologies["CIV_TEST"]={};
    ResearchSelectionContext selection{selection_state,turn_state,players,setup,techs,nullptr,
        teams,maintenance.team_eras,{0,0},selection_host,-1,false};
    ResearchAiWeights weights;weights.profile=profile;weights.players[0].weight_slots=2;
    SessionRandomStreams streams{SimulationRandom(RandomAlgorithm::bnw,{0x12345678U,31,5}),
        SimulationRandom(RandomAlgorithm::bnw,{0xffffffffU,0,9})};
    ResearchAiChoiceContext choice{selection,turn,weights,db,scripts,streams.gameplay};
    struct NativeCase {std::array<std::int64_t,2> weights;std::int64_t count,technology;std::uint64_t roll;};
    // Exact six weighted fixtures executed by the auditor's pinned original
    // retail bytes; oracle element100/101 maps to technology0/1 here.
    const std::array<NativeCase,6> native{{
        {{60000,40000},2,0,25608},
        {{32768,32768},2,0,0},
        {{32768,32767},2,1,48696},
        {{100,50},9,1,111},
        {{100,50},1,0,74},
        {{0,100},2,1,74},
    }};
    const auto prepare=[&](std::array<std::int64_t,2> values,std::int64_t count){
        players.players[0].queue.clear();turn_state.players[0].researching.clear();
        weights.players[0].weights={{0,values[0]},{1,values[1]}};
        db.apply_sql("UPDATE HandicapInfos SET TechNumOptionsConsidered="+std::to_string(count));
        streams.gameplay=SimulationRandom(RandomAlgorithm::bnw,{0xffffffffU,0,9});
    };
    for(const auto &q:native){
        prepare(q.weights,q.count);
        const auto actual_candidates=build_research_ai_candidates(selection,turn,weights,0,false);
        check(actual_candidates.choices.size()==2&&actual_candidates.choices[0].adjusted_weight==q.weights[0]&&
            actual_candidates.choices[1].adjusted_weight==q.weights[1],"F01 actual exponent-zero scorer changed the oracle weights");
        const auto map_before=streams.map.state();const auto gameplay_before=streams.gameplay.state();
        const auto result=choose_research_ai(choice,0);
        check(result.technology==q.technology&&result.weighted&&!result.teammate&&!result.script_override&&
            players.players[0].current_research()==q.technology,"F01 actual public BNW chooser differs from retail weighted fixture");
        check(streams.gameplay.state().seed==3191464396ULL&&streams.gameplay.state().calls==gameplay_before.calls+1&&
            streams.gameplay.state().resets==gameplay_before.resets&&streams.map.state()==map_before,
            "F01 actual chooser did not preserve one gameplay draw, resets and untouched map stream");
        auto roll=SimulationRandom(RandomAlgorithm::bnw,gameplay_before);
        const auto total=q.count==1?std::max(q.weights[0],q.weights[1]):q.weights[0]+q.weights[1];
        check(roll.draw(static_cast<std::uint16_t>(total))==q.roll,"F01 actual unchanged RNG differs from original retail roll fixture");
    }
    // The two original mismatches must be discriminating: the unchanged wide
    // API keeps its extension, while the native chooser narrows only its bound.
    auto wide=SimulationRandom(RandomAlgorithm::bnw,{0xffffffffU,0,9});
    check(wide.draw(100000)==74305&&wide.state().calls==1,"F01 accidentally narrowed the generic RNG extension");
    auto zero=SimulationRandom(RandomAlgorithm::bnw,{0xffffffffU,17,9});
    check(zero.draw(0)==0&&zero.state().seed==3191464396ULL&&zero.state().calls==18&&zero.state().resets==9,
        "F01 native narrowed-zero must still advance the real BNW stream");
    // With roll1 and equal weights1/1, native subtraction uses strict <0:
    // two equal entries retain this bounded two-item source order.
    prepare({1,1},2);const auto map_before=streams.map.state();
    check(choose_research_ai(choice,0).technology==1&&streams.gameplay.state().calls==1&&streams.map.state()==map_before,
        "F01 native strict boundary selected the first equal-weight entry");
    prepare({0,0},2);const auto no_weight_gameplay=streams.gameplay.state();
    check(choose_research_ai(choice,0).technology==-1&&streams.gameplay.state()==no_weight_gameplay&&streams.map.state()==map_before,
        "F01 BNW zero total invented a fallback or draw");
    // The adapter's supported native domain excludes signed32 overflow rather
    // than guessing the original wrapped/invalid-weight path.
    const auto reject=[&](std::array<std::int64_t,2> values,std::int64_t count){
        prepare(values,count);const auto gameplay_before=streams.gameplay.state();bool refused=false;
        try{(void)choose_research_ai(choice,0);}catch(const ResearchAiUnavailable&){refused=true;}
        check(refused&&streams.gameplay.state()==gameplay_before&&streams.map.state()==map_before,
            "F01 unsupported native int32/count domain mutated an RNG stream");
    };
    reject({60000,40000},0);reject({60000,40000},std::int64_t{INT32_MAX}+1);
    reject({std::int64_t{INT32_MAX}+1,1},2);reject({INT32_MAX,1},2);
}
}
int main(){try{run(civ5::content::TechnologyProfile::bnw);run(civ5::content::TechnologyProfile::vp542);
    run_bnw_native_bound_regression();
    std::cout<<"PASS live research AI turn binding; runtime_bits="<<sizeof(void*)*8<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
