#include "civ5/runtime/bnw_pathfinder_cache_controls.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/graphics/bnw_deferred_fog_client.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool value,const char *why) {if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F &&f,const char *why) {
    try{f();}catch(const std::exception&){return;}throw std::runtime_error(why);
}
std::int64_t &scalar(saves::PlayerCoreObservation &p,std::string_view name) {
    for(auto &f:p.fields)if(f.name && std::string_view(f.name)==name && f.values.size()==1)return f.values[0];
    throw std::runtime_error("Missing test scalar");
}
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected selected BNW save, game root and optional SDK Expansion2 directory");
    if(argc==4)for(const auto &file:kBnwPathfinderCacheSourceFiles)
        check(identify_file(std::filesystem::path(argv[3])/file.name).sha256==file.sha256,
              "Pinned SDK cache control source bytes differ");
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(selected.preview.host && !selected.preview.blocker,"Selected BNW host unavailable");
    auto &host=*selected.preview.host;
    const auto &state=host.session().state();const auto player=host.active_player_id();
    auto continuation=host.session().source();continuation.source.container=saves::SaveContainerKind::civis;
    rejects([&]{make_bnw_pathfinder_cache_control_import_owner(continuation,host.identity());},
            "Civis continuation inferred unknown transient controls from original defaults");
    auto owner=make_bnw_pathfinder_cache_control_import_owner(host.session().source(),host.identity());
    check(owner.initialized && owner.scopes.empty() &&
          owner.initialization_order==std::vector<BnwActivationFinder>{
              BnwActivationFinder::common,BnwActivationFinder::ignore_units,
              BnwActivationFinder::route,BnwActivationFinder::water_route,
              BnwActivationFinder::influence,BnwActivationFinder::tactical},
          "Source six-finder control Initialize/setup order differs");
    for(std::size_t i=0;i<owner.controls.size();++i) {
        const auto &c=owner.controls[i];
        check(!c.mp_cache_safe() && !c.force_reset() && !c.data_identity() && c.data_change_invalidates()==(i<3),
              "CvAStar ctor/Initialize/map.setup control defaults differ");
    }
    auto unit_control=owner.at(BnwActivationFinder::common);
    check(!unit_control.set_mp_cache_safe(false) && !unit_control.force_reset(),"Unchanged setter invalidated source cache");
    check(!unit_control.set_mp_cache_safe(true) && unit_control.mp_cache_safe() && unit_control.force_reset(),
          "SetMPCacheSafe did not return previous state or mark actual reset");
    check(unit_control.set_mp_cache_safe(true) && unit_control.force_reset(),"Repeated true setter lost reset");
    check(unit_control.set_mp_cache_safe(false) && !unit_control.mp_cache_safe() && unit_control.force_reset(),
          "Restore incorrectly restored/cleared source ForceReset");
    check(!unit_control.permit_requested_reuse(true,false,-1,-1,0),"Source pending reset allowed reuse");
    auto data=owner.at(BnwActivationFinder::common);data.set_data(31);
    check(data.force_reset() && data.data_identity()==31,"SetData did not invalidate unit cache");
    auto route=owner.at(BnwActivationFinder::route);route.set_data(31);
    check(!route.force_reset() && route.data_identity()==31,"Route SetData introduced an absent source invalidation");
    // Retained prior states need not all be false. These are actual setters,
    // not direct fixture flag writes; nested scopes preserve source order.
    owner.at(BnwActivationFinder::common).set_mp_cache_safe(true);
    owner.at(BnwActivationFinder::route).set_mp_cache_safe(true);
    enter_bnw_pathfinder_cache_scope(state,owner,player);
    check(owner.scopes.size()==1 && owner.scopes[0].previous==std::array<bool,6>{true,false,false,false,true,false},
          "Source six-finder previous flag order differs");
    for(const auto &c:owner.controls)check(c.mp_cache_safe() && c.force_reset(),"Native scope enter did not update retained control");
    enter_bnw_pathfinder_cache_scope(state,owner,player);
    check(owner.scopes.size()==2 && owner.scopes.back().previous==std::array<bool,6>{true,true,true,true,true,true},
          "Nested source scope did not capture current controls");
    restore_bnw_pathfinder_cache_scope(state,owner,player);
    check(owner.scopes.size()==1,"Nested scope restore removed outer state");
    restore_bnw_pathfinder_cache_scope(state,owner,player);
    check(owner.scopes.empty(),"Native scope restore remained open");
    for(std::size_t i=0;i<owner.controls.size();++i)
        check(owner.controls[i].mp_cache_safe()==(i==0 || i==4) && owner.controls[i].force_reset(),
              "Restore did not preserve six previous flags and pending reset");
    rejects([&]{restore_bnw_pathfinder_cache_scope(state,owner,player);},"Restore without enter was accepted");
    auto unknown=owner;unknown.initialized=false;
    rejects([&]{enter_bnw_pathfinder_cache_scope(state,unknown,player);},"Unknown caches were inferred false");
    auto unproven=owner;unproven.initialization_order.clear();
    rejects([&]{enter_bnw_pathfinder_cache_scope(state,unproven,player);},"Unknown source setup order was accepted");
    auto wrong_map=state;wrong_map.world.map_header.guid[0]^=1;
    rejects([&]{enter_bnw_pathfinder_cache_scope(wrong_map,owner,player);},"Foreign map control binding was accepted");
    enter_bnw_pathfinder_cache_scope(state,owner,player);
    auto wrong_turn=state;++wrong_turn.setup.game_turn;wrong_turn.world.pregame->archive.integers.at("game_turn")=wrong_turn.setup.game_turn;
    rejects([&]{restore_bnw_pathfinder_cache_scope(wrong_turn,owner,player);},"Foreign turn restored an active source scope");
    check(owner.scopes.size()==1,"Rejected restore mutated retained scope");
    restore_bnw_pathfinder_cache_scope(state,owner,player);

    // Actual selected initial FOW feed, same contract as production camera.
    const auto art=saves::observe_saved_map_team_art(*state.world.pregame,*state.world.map_plots);
    const auto grid=state.world.map_header.grid;
    materials::FogOfWarState client(grid.width,grid.height,{},-1000.f,1000.f);
    const auto feed=scene::apply_saved_map_fog(grid,art,client);client.tick(0.f);
    auto scene_receipt=graphics::capture_bnw_renderer_scene_receipt(argv[1],identify_file(argv[1]).sha256,host.session().source(),art);
    check(scene_receipt.has_value(),"Selected actual scene receipt unavailable");
    auto projection=*host.session().source().snapshot.map_projection;
    check(graphics::reconcile_bnw_renderer_scene_projection(projection,*scene_receipt,state),"Actual renderer reconciliation failed");
    const auto baseline=graphics::capture_bnw_fog_client_baseline(state,host.identity(),*scene_receipt,feed,client,"selected-control-fow",false);
    BnwGameActivationOwner activation_owner{host.identity()};
    activation_owner.deferred_fog=make_bnw_deferred_fog_owner(baseline);
    activation_owner.city_connections=make_bnw_activation_connections_import_owner(host.session().source(),host.identity());
    activation_owner.pathfinder_cache=make_bnw_pathfinder_cache_control_import_owner(host.session().source(),host.identity());
    auto activation=state;for(auto &p:activation.world.all_players->players)scalar(p.core,"m_bTurnActive")=0;
    const auto player_handlers=make_bnw_game_activation_source_player_handlers(host.database(),host.identity(),player);
    std::vector<BnwTurnPhase> prefix;
    for(const auto phase:bnw_game_turn_order()){if(phase==BnwTurnPhase::game_activation)break;prefix.push_back(phase);}
    const auto executed=execute_bnw_game_activation_draft(activation,activation_owner,host.database(),host.identity(),prefix,player_handlers);
    check(!executed.draft && executed.completed_steps.size()==11 &&
          executed.completed_steps.back()==BnwGameActivationStep::city_connection_gold &&
          executed.blocker.find("happiness")!=std::string::npos &&
          activation_owner.pathfinder_cache->scopes.empty() && !activation_owner.deferred_fog->pending &&
          activation_owner.pending.empty() && scalar(activation.world.all_players->players.at(player).core,"m_bTurnActive")==0,
          "Actual source cache/FOW/economy prefix or rollback differs");
    for(const auto &c:activation_owner.pathfinder_cache->controls)
        check(!c.mp_cache_safe() && !c.force_reset(),"Rejected activation changed original cache controls");
    const auto active=execute_bnw_game_activation_draft(state,activation_owner,host.database(),host.identity(),prefix,player_handlers);
    check(!active.draft && active.completed_steps.empty() && active.blocker.find("active")!=std::string::npos,
          "Imported already-active preflight was bypassed");
    const BnwGameActivationService duplicate[]={
        {BnwGameActivationStep::pathfinder_cache_enter,[](LiveSessionState&,std::int64_t){return LiveActionResult{LiveActionOutcome::committed,"synthetic duplicate must reject"};}}
    };
    const auto doubled=execute_bnw_game_activation_draft(activation,activation_owner,host.database(),host.identity(),prefix,player_handlers,duplicate);
    check(!doubled.draft && doubled.completed_steps.empty() && doubled.blocker.find("second owner")!=std::string::npos,
          "Generic callback replaced actual cache owner");
    std::cout<<"PASS selected BNW actual retained cache controls/FOW/economy prefix, exact reset/restore, nesting and rollback; search remains blocked\n";
}
