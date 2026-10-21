#include "civ5/runtime/bnw_world_anchor_clear.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
std::int64_t value(const saves::PlotObservation &plot,std::string_view name) {
    for(const auto &field:plot.fields)
        if(field.name && std::string_view(field.name)==name) return field.value;
    throw std::runtime_error("Missing test plot field");
}
LiveSessionState fixture(std::int64_t anchor,std::int64_t data) {
    saves::SavedSessionSnapshot world;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    world.game_state.layout=bnw;
    world.map_header.layout=bnw;
    world.map_header.grid={1,1};
    world.map_plots.emplace();
    world.map_plots->layout=bnw;
    saves::PlotObservation plot;
    plot.index=plot.x=plot.y=0;
    plot.fields={{"world_anchor",anchor},{"world_anchor_data",data}};
    world.map_plots->plots.push_back(plot);
    world.map_projection.emplace();
    world.map_projection->grid=world.map_header.grid;
    saves::SavedMapCellProjection cell;
    cell.index=cell.x=cell.y=0;
    world.map_projection->cells.push_back(cell);
    return {std::move(world),ResearchContext{},std::vector<TeamTechnologies>{},
        PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},
        MaintenanceContext{},TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{5,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{7,0,0})}};
}
void synthetic() {
    auto state=fixture(-1,19);
    check(plan_bnw_world_anchor_clear(state,0).valid &&
          !plan_bnw_world_anchor_clear(state,0).removal,
          "No-anchor plot unexpectedly requires engine event");
    BnwWorldAnchorJournal none;
    check(stage_bnw_world_anchor_clear(state,0,none).outcome==
              LiveActionOutcome::committed &&
          value(state.world.map_plots->plots[0],"world_anchor_data")==-1 &&
          none.publish(state).status==BnwWorldAnchorPublicationStatus::complete,
          "No-anchor SetWorldAnchor failed to clear saved data");

    state=fixture(2,47);
    const auto plan=plan_bnw_world_anchor_clear(state,0);
    check(plan.valid && plan.removal && plan.removal->old_anchor==2 &&
          plan.removal->old_data==47 && plan.removal->data==-1 &&
          !plan.removal->add,
          "BNW SetWorldAnchor removal used old data instead of default -1");
    check(stage_bnw_world_anchor_clear(state,0,none).outcome==
              LiveActionOutcome::rejected &&
          value(state.world.map_plots->plots[0],"world_anchor")==2,
          "Missing engine emitter changed world anchor");
    int invoked=0;
    BnwWorldAnchorJournal failing([&](const auto &event){
        ++invoked;
        check(event.data==-1 && event.old_data==47,
              "Incorrect engine anchor payload");
        throw std::runtime_error("engine dispatch uncertain");
    });
    check(stage_bnw_world_anchor_clear(state,0,failing).outcome==
              LiveActionOutcome::committed && invoked==0,
          "World anchor stage published before game commit");
    check(failing.publish(state).status==BnwWorldAnchorPublicationStatus::uncertain &&
          failing.uncertain() && invoked==1 &&
          failing.publish(state).status==BnwWorldAnchorPublicationStatus::uncertain &&
          invoked==1,
          "Uncertain engine anchor event was replayed");
    state=fixture(128,0);
    check(!plan_bnw_world_anchor_clear(state,0).valid &&
          stage_bnw_world_anchor_clear(state,0,none).outcome==
              LiveActionOutcome::rejected,
          "Out-of-range source anchor was accepted");
}
void selected(const std::filesystem::path &save,const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected BNW host unavailable");
    const auto &source=bootstrap.preview.host->session().state();
    constexpr std::uint64_t index=635;
    const auto plan=plan_bnw_world_anchor_clear(source,index);
    check(plan.valid && plan.removal && plan.removal->plot==index &&
          plan.removal->old_anchor>=0 && plan.removal->data==-1,
          "Selected Gandhi plot 635 anchor changed");
    auto draft=source;
    std::vector<BnwWorldAnchorRemovalEvent> engine;
    BnwWorldAnchorJournal journal([&](const auto &event){engine.push_back(event);});
    const auto staged=stage_bnw_world_anchor_clear(draft,index,journal);
    check(staged.outcome==LiveActionOutcome::committed && engine.empty() &&
          journal.pending().size()==1 && journal.pending()[0]==*plan.removal &&
          value(draft.world.map_plots->plots[index],"world_anchor")==-1 &&
          value(draft.world.map_plots->plots[index],"world_anchor_data")==-1 &&
          value(source.world.map_plots->plots[index],"world_anchor")==
              plan.removal->old_anchor,
          "Selected Gandhi detached anchor stage was not transactional");
    check(journal.publish(source).status==BnwWorldAnchorPublicationStatus::rejected &&
          engine.empty(),
          "Engine anchor removal published against uncommitted source");
    auto wrong_map=draft;
    ++wrong_map.world.map_header.guid[0];
    check(journal.publish(wrong_map).status==BnwWorldAnchorPublicationStatus::rejected &&
          engine.empty(),
          "Engine anchor removal published against another map GUID");
    check(journal.publish(draft).status==BnwWorldAnchorPublicationStatus::complete &&
          engine.size()==1 && engine[0]==*plan.removal &&
          journal.publish(draft).status==BnwWorldAnchorPublicationStatus::complete &&
          engine.size()==1,
          "Selected Gandhi GameplayWorldAnchor event payload or once delivery changed");
    std::cout<<"selected BNW anchor plot="<<index<<" x="<<plan.removal->x
             <<" y="<<plan.removal->y<<" old="<<plan.removal->old_anchor
             <<" saved data="<<plan.removal->old_data
             <<" event data="<<plan.removal->data<<'\n';
}
} // namespace

int main(int argc,char **argv) {
    check(argc==1 || argc==3,"Expected optional selected BNW save and game root");
    synthetic();
    if(argc==3) selected(argv[1],argv[2]);
}
