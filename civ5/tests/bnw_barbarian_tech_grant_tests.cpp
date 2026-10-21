#include "civ5/runtime/bnw_barbarian_tech_grant.hpp"
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
LiveSessionState fixture() {
    saves::SavedSessionSnapshot world;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    world.game_state.layout=bnw;
    world.teams.emplace();
    world.teams->layout=bnw;
    world.teams->teams.resize(64);
    world.pregame.emplace();
    world.pregame->layout=bnw;
    auto &minor=world.pregame->archive.flag_arrays["minor_nation_civs"];
    minor.resize(64);
    minor[22]=true;
    ResearchContext setup;
    std::vector<TeamTechnologies> owned;
    for(std::int64_t id=0;id<64;++id) {
        const bool alive=id==0 || id==1 || id==22 || id==63;
        auto &team=world.teams->teams[static_cast<std::size_t>(id)];
        team.index=id;
        team.fields={{"id",id},{"alive_count",alive?1:0}};
        ResearchPlayerState player;
        player.team=id;
        player.alive=alive;
        setup.players.emplace(id,player);
        owned.emplace_back(content::TechnologyProfile::bnw,id,3);
    }
    owned[0].set_has_tech(0,true);
    owned[0].set_has_tech(2,true);
    owned[1].set_has_tech(1,true);
    owned[1].set_has_tech(2,true);
    owned[63].set_has_tech(0,true);
    return {std::move(world),std::move(setup),std::move(owned),
        PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},
        MaintenanceContext{},TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{5,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{7,0,0})}};
}
content::TechnologyRules rules() {
    content::TechnologyRules result;
    result.collection.rows=result.collection.slots=3;
    for(std::uint64_t tech=0;tech<3;++tech) {
        auto &item=result.entries[tech];
        item.base.type="TECH_TEST_"+std::to_string(tech);
        item.booleans["Repeat"]=false;
        item.booleans["MapVisible"]=false;
    }
    return result;
}
void selected(const std::filesystem::path &save,const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected BNW host unavailable");
    const auto &host=*bootstrap.preview.host;
    const auto &state=host.session().state();
    const auto before=state.technologies[63].nondefault_values();
    const auto random=state.random.gameplay.state();
    const auto gate=plan_bnw_barbarian_technology_gate(
        state,host.technologies(),host.defines());
    check(gate.alive_major_teams>0 && gate.grants.size()==1 && !gate.safe &&
          !gate.blocker.empty() && gate.grants[0].technology>=0 &&
          gate.grants[0].major_owners>=gate.threshold,
          "Selected Gandhi barbarian technology gate changed");
    check(state.technologies[63].nondefault_values()==before &&
          state.random.gameplay.state()==random,
          "Barbarian technology preflight mutated selected world");
    auto attempted=state;
    const auto result=run_bnw_barbarian_technology_gate(
        attempted,host.technologies(),host.defines());
    check(result.outcome==LiveActionOutcome::rejected &&
          attempted.technologies[63].nondefault_values()==before &&
          attempted.random.gameplay.state()==random,
          "Selected Gandhi barbarian grant escaped the mutation barrier");
    std::cout<<"selected BNW barbarian tech="<<gate.grants[0].technology
             <<" type="<<gate.grants[0].type
             <<" major owners="<<gate.grants[0].major_owners
             <<"/"<<gate.alive_major_teams
             <<" threshold="<<gate.threshold
             <<" effects="<<gate.grants[0].effects.size()<<'\n';
}
} // namespace

int main(int argc,char **argv) {
    check(argc==1 || argc==3,"Expected optional selected BNW save and game root");
    content::RulesDatabase database;
    database.apply_sql("CREATE TABLE Defines(Name TEXT,Value TEXT);"
                       "INSERT INTO Defines VALUES('BARBARIAN_TECH_PERCENT','75');");
    const auto defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto technologies=rules();
    auto state=fixture();
    const auto owned=state.technologies[63].nondefault_values();
    const auto gate=plan_bnw_barbarian_technology_gate(state,technologies,defines);
    check(gate.alive_major_teams==2 && gate.percent==75 && gate.threshold==1 &&
          gate.grants.size()==2 && !gate.safe &&
          gate.grants[0].technology==1 && gate.grants[0].major_owners==1 &&
          gate.grants[1].technology==2 && gate.grants[1].major_owners==2 &&
          gate.grants[0].effects[0]==BnwBarbarianTechEffect::team_technology_write &&
          gate.grants[0].effects[1]==BnwBarbarianTechEffect::team_set_has_tech_lua &&
          gate.grants[0].effects.back()==BnwBarbarianTechEffect::gameplay_technology_event &&
          state.technologies[63].nondefault_values()==owned &&
          run_bnw_barbarian_technology_gate(state,technologies,defines).outcome==
              LiveActionOutcome::rejected,
          "BNW barbarian tech order, threshold or nonmutating preflight changed");
    state.technologies[63].set_has_tech(1,true);
    state.technologies[63].set_has_tech(2,true);
    check(plan_bnw_barbarian_technology_gate(state,technologies,defines).safe &&
          run_bnw_barbarian_technology_gate(state,technologies,defines).outcome==
              LiveActionOutcome::committed,
          "No-grant BNW barbarian technology branch was rejected");
    state.setup.players[63].team=0;
    bool failed=false;
    try {(void)plan_bnw_barbarian_technology_gate(state,technologies,defines);}
    catch(const std::runtime_error &) {failed=true;}
    check(failed,"BNW barbarian technology accepted missing leader");
    if(argc==3) selected(argv[1],argv[2]);
}
