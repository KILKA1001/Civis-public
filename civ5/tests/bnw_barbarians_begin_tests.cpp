#include "civ5/runtime/bnw_barbarians_begin.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool value,const char *why) {
    if(!value) throw std::runtime_error(why);
}
LiveSessionState fixture() {
    saves::SavedSessionSnapshot world;
    world.game_state.layout=saves::GameStateLayout::expansion2_version1;
    world.map_header.layout=world.game_state.layout;
    world.map_header.grid={4,1};
    world.map_projection.emplace();
    world.map_projection->grid=world.map_header.grid;
    world.supporting_classes.emplace();
    auto &support=*world.supporting_classes;
    support.layout=world.game_state.layout;
    support.plot_count=4;
    support.barbarians_version=1;
    support.barbarians.resize(4);
    const std::int64_t counters[]={3,2,-4,-1};
    const std::int64_t improvements[]={7,8,-1,-1};
    for(std::uint64_t i=0;i<4;++i) {
        saves::SavedMapCellProjection cell;
        cell.index=cell.x=i;
        cell.y=0;
        cell.improvement_type=improvements[i];
        world.map_projection->cells.push_back(cell);
        support.barbarians[i].counter=counters[i];
        support.barbarians[i].units_spawned=9;
    }
    return {std::move(world),ResearchContext{}, {},PlayerResearchContext{},
        CityYieldContext{},BnwUnhappinessContext{},MaintenanceContext{},
        TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{5,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{7,0,0})}};
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected BNW host unavailable");
    auto &host=*bootstrap.preview.host;
    const auto camp=resolve_bnw_barbarian_camp_improvement(host.defines());
    auto state=host.session().state();
    check(state.world.supporting_classes && state.world.map_projection,
          "Selected BNW barbarian state unavailable");
    const auto before=state.world.supporting_classes->barbarians;
    const auto rng=state.random.gameplay.state().calls;
    const auto phase=make_bnw_barbarians_begin_handler(camp).run(state,-1);
    check(phase.outcome==LiveActionOutcome::committed &&
          state.random.gameplay.state().calls==rng,
          "Selected BNW barbarian begin failed or consumed RNG");
    std::size_t camps=0,cleared=0,cooldowns=0;
    for(std::size_t i=0;i<before.size();++i) {
        const auto old=before[i];
        const auto &now=state.world.supporting_classes->barbarians[i];
        auto expected=old.counter;
        auto units=old.units_spawned;
        if(expected>0) {
            if(*state.world.map_projection->cells[i].improvement_type!=camp) {
                expected=-1;units=-1;++cleared;
            } else {--expected;++camps;}
        } else if(expected< -1) {++expected;++cooldowns;}
        check(now.counter==expected && now.units_spawned==units,
              "Selected BNW barbarian counter source transition changed");
    }
    std::cout<<"selected BNW barbarian camps="<<camps
             <<" cleared="<<cleared<<" cooldowns="<<cooldowns<<'\n';
}
} // namespace

int main(int argc,char **argv) {
    check(argc==1 || argc==3,"Expected optional selected BNW save and game root");
    content::RulesDatabase database;
    database.apply_sql("CREATE TABLE Defines(Name TEXT,Value TEXT);"
                       "INSERT INTO Defines VALUES('BARBARIAN_CAMP_IMPROVEMENT','7');");
    const auto defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
    check(resolve_bnw_barbarian_camp_improvement(defines)==7,
          "BNW barbarian camp ID was not read from active rules");
    auto state=fixture();
    auto handler=make_bnw_barbarians_begin_handler(7);
    check(handler.phase==BnwTurnPhase::game_barbarians_begin && handler.run &&
          !handler.after_commit,"BNW barbarian phase was not bound");
    check(handler.run(state,0).outcome==LiveActionOutcome::rejected &&
          state.world.supporting_classes->barbarians[0].counter==3,
          "BNW barbarian game phase accepted a player turn");
    auto broken=state;
    broken.world.map_projection->cells[1].improvement_type.reset();
    check(handler.run(broken,-1).outcome==LiveActionOutcome::rejected &&
          broken.world.supporting_classes->barbarians[0].counter==3,
          "Unresolved BNW improvement leaked an earlier counter mutation");
    const auto rng=state.random.gameplay.state().calls;
    check(handler.run(state,-1).outcome==LiveActionOutcome::committed &&
          state.random.gameplay.state().calls==rng,
          "BNW barbarian begin rejected valid state or used RNG");
    const auto &barbs=state.world.supporting_classes->barbarians;
    check(barbs[0].counter==2 && barbs[0].units_spawned==9 &&
          barbs[1].counter==-1 && barbs[1].units_spawned==-1 &&
          barbs[2].counter==-3 && barbs[2].units_spawned==9 &&
          barbs[3].counter==-1 && barbs[3].units_spawned==9,
          "BNW barbarian begin differed from Expansion2 counter order");
    if(argc==3) selected(argv[1],argv[2]);
}
