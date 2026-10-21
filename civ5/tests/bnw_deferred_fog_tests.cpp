#include "civ5/graphics/bnw_deferred_fog_client.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace civ5;
using namespace civ5::runtime;
namespace {
void check(bool value,const char *why) {if(!value) throw std::runtime_error(why);}
template<class F> void rejects(F &&f,const char *why) {
    try {f();} catch(const std::exception &) {return;}
    throw std::runtime_error(why);
}
std::int64_t &player_scalar(saves::PlayerCoreObservation &core,std::string_view name) {
    for(auto &f:core.fields) if(f.name && std::string_view(f.name)==name && f.values.size()==1) return f.values[0];
    throw std::runtime_error("Missing test player scalar");
}
std::int64_t &visibility(LiveSessionState &source,std::uint64_t plot,std::uint64_t team) {
    for(auto &a:source.world.map_plots->plots.at(plot).arrays)
        if(a.name && std::string_view(a.name)=="visibility") return a.values.at(team);
    throw std::runtime_error("Missing test visibility array");
}
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected save and game root");
    auto bootstrap=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,"Selected BNW host is unavailable");
    auto &host=*bootstrap.preview.host;
    const auto &state=host.session().state();
    auto art=saves::observe_saved_map_team_art(*state.world.pregame,*state.world.map_plots);
    const auto grid=state.world.map_header.grid;
    // The exact production baseline path: this retained owner receives the
    // actual saved map events, then its culling grid ticks before capture.
    materials::FogOfWarState client(grid.width,grid.height,{},-1000.f,1000.f);
    const auto initial_feed=scene::apply_saved_map_fog(grid,art,client);client.tick(0.f);
    auto scene_receipt=graphics::capture_bnw_renderer_scene_receipt(argv[1],identify_file(argv[1]).sha256,
        host.session().source(),art);
    check(scene_receipt.has_value(),"Selected actual renderer scene receipt could not be captured");
    auto renderer_projection=*host.session().source().snapshot.map_projection;
    check(graphics::reconcile_bnw_renderer_scene_projection(renderer_projection,*scene_receipt,state),
          "Selected renderer/live projection could not be reconciled");
    auto baseline=graphics::capture_bnw_fog_client_baseline(state,host.identity(),*scene_receipt,
        initial_feed,client,"selected-camera-fow",false);
    auto owner=make_bnw_deferred_fog_owner(baseline);
    check(owner.queue_known && owner.published && owner.queued_plots.empty() && !owner.pending &&
          baseline.events_applied()==grid.size(),"Actual full fog baseline did not bind the queue");
    auto missing_tick=client;missing_tick.visibility_event_all(0);
    rejects([&]{graphics::capture_bnw_fog_client_baseline(state,host.identity(),*scene_receipt,initial_feed,missing_tick,"camera",false);},
        "Unapplied/stale initial fog baseline was accepted");
    rejects([&]{graphics::capture_bnw_fog_client_baseline(state,host.identity(),*scene_receipt,initial_feed,client,"camera",true);},
        "Unknown debug fog behavior was accepted");
    auto foreign=*scene_receipt;foreign.active_team^=1;
    rejects([&]{graphics::capture_bnw_fog_client_baseline(state,host.identity(),foreign,initial_feed,client,"camera",false);},
        "Foreign active-team renderer receipt was accepted");

    stage_bnw_deferred_fog_flush(state,owner);
    check(owner.pending && owner.pending->updates.empty(),"Known empty SDK queue was not staged");
    const auto original_visibility=client.visibility();const auto original_culling=client.culling();
    const auto empty=graphics::publish_bnw_deferred_fog(owner,client);
    check(empty.sequence()==1 && empty.events_applied()==0 && !owner.pending &&
          client.visibility()==original_visibility && client.culling()==original_culling && !client.dirty(),
          "Empty queue changed native fog or lacked a real matching client receipt");
    rejects([&]{graphics::publish_bnw_deferred_fog(owner,client);},"Fog publication replay was accepted");
    auto unknown=owner;unknown.queue_known=false;
    rejects([&]{stage_bnw_deferred_fog_flush(state,unknown);},"Unknown imported fog queue was treated as empty");

    auto changed=state;
    const auto &cache=bnw_live_plot_visibility_cache(state);
    std::uint64_t plot=grid.size();
    for(std::uint64_t i=0;i<grid.size();++i)
        if(baseline.snapshot().modes.at(i)==2 && cache.counts.at(i*cache.wire_slots+art.active_team)==1) {plot=i;break;}
    check(plot<grid.size(),"Selected one-viewer visible fog fixture changed");
    visibility(changed,plot,art.active_team)=0;rebuild_bnw_live_plot_visibility(changed);
    rejects([&]{stage_bnw_deferred_fog_flush(changed,owner);},"Visibility changed without tracked updateFog(true)");
    check(!owner.pending && owner.queued_plots.empty(),"Untracked visibility failure changed the owned queue");
    queue_bnw_deferred_fog(owner,plot);queue_bnw_deferred_fog(owner,plot);
    check(owner.queued_plots==std::vector<std::uint64_t>{plot},"SDK deferred queue duplicate suppression changed");
    stage_bnw_deferred_fog_flush(changed,owner);
    check(owner.pending && owner.pending->updates.size()==1 && owner.pending->updates[0].plot==plot &&
          owner.pending->updates[0].mode==1 && owner.queued_plots.empty(),"Source deferred fog event is stale or out of order");
    const auto pending_digest=owner.pending->digest;
    const auto good_client=client;client.visibility_event_all(0);
    const auto bad_visibility=client.visibility();
    rejects([&]{graphics::publish_bnw_deferred_fog(owner,client);},"Stale actual FOW client was silently accepted");
    check(owner.pending && owner.pending->digest==pending_digest && client.visibility()==bad_visibility,
          "Rejected publication lost its journal or changed actual native fog");
    client=good_client;
    auto tampered=*owner.pending;tampered.updates[0].mode=2;
    rejects([&]{graphics::consume_bnw_deferred_fog_batch(tampered,*owner.published,client);},"Tampered native fog event was accepted");
    const auto delivered=graphics::publish_bnw_deferred_fog(owner,client);
    const auto xy=grid.coordinates(plot);
    check(delivered.sequence()==2 && delivered.events_applied()==1 && !owner.pending &&
          client.visibility().at(xy[0]*grid.height+xy[1])==1 && client.dirty(),
          "GameplayFOWChanged did not update the retained native fog owner");
    client.tick(0.f);check(!client.dirty(),"Native fog culling did not rebuild after receipt delivery");
    auto changed_active=changed;changed_active.world.pregame->archive.integers.at("active_player")=1;
    rejects([&]{stage_bnw_deferred_fog_flush(changed_active,owner);},"Another active player's fog used the previous client binding");

    // Native activation must stage fog on its private owner and discard it
    // with the world when the next source boundary (pathfinder) is unavailable.
    auto activation_state=state;
    for(auto &p:activation_state.world.all_players->players) player_scalar(p.core,"m_bTurnActive")=0;
    BnwGameActivationOwner activation_owner{host.identity()};
    activation_owner.deferred_fog=make_bnw_deferred_fog_owner(baseline);
    const auto game_order=bnw_game_turn_order();
    std::vector<BnwTurnPhase> prefix(game_order.begin(),std::find(game_order.begin(),game_order.end(),BnwTurnPhase::game_activation));
    auto players=make_bnw_game_activation_source_player_handlers(host.database(),host.identity(),art.active_player);
    const auto before_rng=activation_state.random.gameplay.state();
    auto activation=execute_bnw_game_activation_draft(activation_state,activation_owner,host.database(),host.identity(),prefix,players);
    check(!activation.draft && activation.completed_steps.size()==8 &&
          activation.completed_steps[4]==BnwGameActivationStep::active_player_deferred_fog &&
          activation.completed_steps.back()==BnwGameActivationStep::all_units_unprocessed &&
          activation.blocker.find("pathfinder_cache_enter")!=std::string::npos &&
          !activation_owner.deferred_fog->pending && activation_owner.pending.empty() &&
          activation_state.random.gameplay.state()==before_rng,
          "Native fog activation was skipped or a rejected private turn published its journal");
    std::cout << "PASS selected real FOW baseline/feed/receipt + tracked queue, native event, retry and activation rollback guards\n";
}
