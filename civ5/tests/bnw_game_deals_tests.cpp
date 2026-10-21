#include "civ5/runtime/bnw_game_deals.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool value,const char *why) {
    if(!value) throw std::runtime_error(why);
}
LiveSessionState fixture() {
    saves::SavedSessionSnapshot world;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    world.game_state.layout=bnw;
    world.history.layout=bnw;
    world.history.deals.version=1;
    saves::DealObservation proposed;
    proposed.version=3;
    proposed.from_player=4;
    proposed.to_player=5;
    world.history.deals.groups[0].push_back(proposed);
    saves::DealObservation historical;
    historical.version=2;
    historical.final_turn=7;
    world.history.deals.groups[2].push_back(historical);
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
    auto state=bootstrap.preview.host->session().state();
    check(state.world.history.deals.groups[1].empty(),
          "Selected BNW save no longer exercises the empty-current-deals branch");
    const auto before=state.world.history.deals;
    const auto rng=state.random.gameplay.state();
    const auto result=make_bnw_game_deals_handler().run(state,-1);
    if(result.outcome!=LiveActionOutcome::committed)
        throw std::runtime_error("Selected BNW deal turn rejected: "+result.reason+
            " profile="+std::to_string(static_cast<int>(state.setup.profile))+
            " game="+std::to_string(static_cast<int>(state.world.game_state.layout))+
            " history="+std::to_string(static_cast<int>(state.world.history.layout))+
            " version="+std::to_string(state.world.history.deals.version.value_or(999))+
            " next="+std::to_string(state.world.history.deals.next_deal_id.value_or(999)));
    check(result.outcome==LiveActionOutcome::committed &&
          state.random.gameplay.state()==rng &&
          state.world.history.deals.groups[0].size()==before.groups[0].size() &&
          state.world.history.deals.groups[1].size()==before.groups[1].size() &&
          state.world.history.deals.groups[2].size()==before.groups[2].size(),
          "Selected BNW empty deal turn changed deals or RNG");
    std::cout<<"selected BNW current deals=0\n";
}
} // namespace

int main(int argc,char **argv) {
    check(argc==1 || argc==3,"Expected optional selected BNW save and game root");
    auto state=fixture();
    const auto handler=make_bnw_game_deals_handler();
    check(handler.phase==BnwTurnPhase::game_deals && handler.run &&
          !handler.after_commit,"BNW game-deals phase was not bound");
    const auto rng=state.random.gameplay.state();
    check(handler.run(state,-1).outcome==LiveActionOutcome::committed &&
          state.world.history.deals.groups[0].size()==1 &&
          state.world.history.deals.groups[0][0].from_player==4 &&
          state.world.history.deals.groups[2].size()==1 &&
          state.world.history.deals.groups[2][0].final_turn==7 &&
          state.random.gameplay.state()==rng,
          "Empty current deals changed proposed/historical deals or RNG");
    check(handler.run(state,0).outcome==LiveActionOutcome::rejected,
          "BNW game deals accepted a player phase");
    auto invalid=state;
    invalid.world.history.deals.groups[1].push_back(
        saves::DealObservation{});
    check(handler.run(invalid,-1).outcome==LiveActionOutcome::rejected &&
          invalid.world.history.deals.groups[1].size()==1,
          "Nonempty current deals were silently accepted or mutated");
    invalid=state;
    invalid.world.history.deals.version.reset();
    check(handler.run(invalid,-1).outcome==LiveActionOutcome::rejected,
          "Unverified deal version was accepted");
    invalid=state;
    invalid.world.history.layout=saves::GameStateLayout::vp542_corpus;
    check(handler.run(invalid,-1).outcome==LiveActionOutcome::rejected,
          "VP game history was accepted as BNW");
    // CvGameDeals::DoTurn with no item ending or resource-shortage query still
    // calls DoUpdateCurrentDealsList, preserving order across both lists.
    auto migrating=fixture();
    migrating.setup.game_turn=7;
    saves::DealObservation ended;
    ended.version=3;ended.cancelled=false;
    ended.from_player=4;ended.to_player=5;ended.final_turn=7;
    saves::DealObservation continuing=ended;
    continuing.final_turn=10;
    saves::TradedItemObservation lasting;
    lasting.version=2;lasting.type=0; // TRADE_ITEM_GOLD has no end effect.
    lasting.from_player=4;lasting.final_turn=-1;
    lasting.from_renewed=false;lasting.to_renewed=true;
    ended.items.push_back(lasting);
    continuing.items.push_back(lasting);
    migrating.world.history.deals.groups[1]={ended,continuing};
    const auto migration_rng=migrating.random.gameplay.state();
    check(handler.run(migrating,-1).outcome==LiveActionOutcome::committed &&
          migrating.world.history.deals.groups[0].size()==1 &&
          migrating.world.history.deals.groups[1].size()==1 &&
          migrating.world.history.deals.groups[1][0].final_turn==10 &&
          migrating.world.history.deals.groups[1][0].items[0].to_renewed==true &&
          migrating.world.history.deals.groups[2].size()==2 &&
          migrating.world.history.deals.groups[2][0].version==2 &&
          migrating.world.history.deals.groups[2][1].final_turn==7 &&
          migrating.world.history.deals.groups[2][1].items.size()==1 &&
          migrating.world.history.deals.groups[2][1].items[0].to_renewed==true &&
          migrating.random.gameplay.state()==migration_rng,
          "Safe nonempty BNW deal migration differs from source order");
    auto expiring=fixture();
    expiring.setup.game_turn=7;
    lasting.final_turn=7;
    ended.items={lasting};
    expiring.world.history.deals.groups[1]={ended};
    check(handler.run(expiring,-1).outcome==LiveActionOutcome::rejected &&
          expiring.world.history.deals.groups[1].size()==1 &&
          expiring.world.history.deals.groups[2].size()==1 &&
          expiring.world.history.deals.groups[1][0].items[0].to_renewed==true,
          "Expiring item was accepted or partially ended");
    auto resource=fixture();
    resource.setup.game_turn=7;
    lasting.type=3;lasting.final_turn=10; // TRADE_ITEM_RESOURCES
    continuing.items={lasting};
    resource.world.history.deals.groups[1]={continuing};
    check(handler.run(resource,-1).outcome==LiveActionOutcome::rejected &&
          resource.world.history.deals.groups[1].size()==1,
          "Resource shortage was silently skipped");
    // The second source scan breaks at PEACE_TREATY before a later resource.
    auto protected_resource=fixture();
    protected_resource.setup.game_turn=7;
    auto peace=lasting;peace.type=13; // TRADE_ITEM_PEACE_TREATY
    continuing.items={peace,lasting};
    protected_resource.world.history.deals.groups[1]={continuing};
    check(handler.run(protected_resource,-1).outcome==LiveActionOutcome::committed &&
          protected_resource.world.history.deals.groups[1].size()==1 &&
          protected_resource.world.history.deals.groups[1][0].items.size()==2,
          "Peace treaty did not short-circuit resource validity in source order");
    if(argc==3) selected(argv[1],argv[2]);
}
