#include "civ5/graphics/bnw_camera_fog_client.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
using namespace civ5;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F action){try{action();}catch(const std::exception &){return;}throw std::runtime_error("Required bridge rejection missing");}
std::int64_t &scalar(saves::PlayerCoreObservation &core,std::string_view name){
    for(auto &field:core.fields)if(field.name && std::string_view(field.name)==name && field.values.size()==1)return field.values[0];
    throw std::runtime_error("Missing actual selected player scalar");
}
bool native_step(runtime::BnwGameActivationStep step){
    using S=runtime::BnwGameActivationStep;
    return step==S::set_turn_active || step==S::publish_end_turn_dirty || step==S::clear_end_turn ||
        step==S::unit_attrition || step==S::active_player_deferred_fog || step==S::active_turn_count_log ||
        step==S::publish_player_turn_start || step==S::all_units_unprocessed;
}
void run(const char *save,const char *root){
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,"Actual selected BNW host is unavailable");
    auto &host=*bootstrap.preview.host;
    const auto initial=host.session().state();
    auto &state=host.session().mutable_state();state=initial;
    // Control-flow setup: exercise the source activation branch without claiming
    // this artificial deactivation or the unsupported later phases are GameCore.
    for(auto &player:state.world.all_players->players)scalar(player.core,"m_bTurnActive")=0;
    const auto grid=state.world.map_header.grid;
    const auto art=saves::observe_saved_map_team_art(*state.world.pregame,*state.world.map_plots);
    auto scene_receipt=graphics::capture_bnw_renderer_scene_receipt(save,runtime::identify_file(save).sha256,
        host.session().source(),art);
    check(scene_receipt.has_value(),"Actual source renderer receipt missing");
    auto source_projection=*host.session().source().snapshot.map_projection;
    check(graphics::reconcile_bnw_renderer_scene_projection(source_projection,*scene_receipt,state),
        "Actual selected scene could not be reconciled");
    auto context=std::make_shared<graphics::GlContext>();
    const auto assets=materials::FogOfWarAssets::load(0,{});
    graphics::GlFogOfWarMap map(context,assets);
    auto world=terrain::build_world_layout(static_cast<std::int32_t>(grid.width),
        static_cast<std::int32_t>(grid.height),terrain::game_world_grid_mode(state.world.map_header.wrap_x),{},
        [](float value){return 1.f/value;});
    const auto height=terrain::HeightOwnerParameters::from_world(world);
    materials::FogOfWarState native(grid.width,grid.height,{},world.bounds[0],world.bounds[3]);
    const auto feed=scene::apply_saved_map_fog(grid,art,native);native.tick(0.f);
    auto &activation=host.activation_owner();
    auto input=host.preview_input();
    check(input.activation_owner==&activation,"Production preview did not hand off its SAME stable execution owner");
    graphics::BnwCameraFogClient bridge(host.session(),activation,host.identity(),*scene_receipt,feed,native,map,"selected-execution-camera");
    const auto baseline=*activation.deferred_fog;
    check(&bridge.journal()==&*activation.deferred_fog && bridge.valid_handoff(),"Camera has a detached journal copy");
    rejects([&]{graphics::BnwCameraFogClient duplicate(host.session(),activation,host.identity(),*scene_receipt,feed,native,map,"duplicate");});
    ui::BnwClientSceneFrame client(host.session(),*scene_receipt,art.active_player,{});
    client.bind_native_fog(bridge.binding());
    const auto &cache=runtime::bnw_live_plot_visibility_cache(state);
    auto plot=grid.size();
    for(std::uint64_t i=0;i<grid.size();++i)
        if(bridge.journal().published->snapshot().modes.at(i)==2 &&
            cache.counts.at(i*cache.wire_slots+art.active_team)==1){plot=i;break;}
    check(plot<grid.size(),"Selected one-viewer visible source plot unavailable");
    terrain::CameraControllerState controller{};
    controller=terrain::initialize_camera(world,terrain::game_world_grid_mode(state.world.map_header.wrap_x),
        {64,64},controller,terrain::X87Precision::double_precision);
    // Frame the actual selected event plot, using the same cube->world->height
    // transform as fog_of_war_frame. An offscreen event legitimately need not
    // change the mask pixels of an unrelated camera view.
    const auto xy=grid.coordinates(plot);const int a=static_cast<int>(xy[0])-static_cast<int>(xy[1]>>1);
    const int y=static_cast<int>(xy[1]),c=-a-y;const auto &m=world.source_grid.second_matrix;
    const int u=a*m[0]+c*m[8]+y*m[4]+m[12],v=y*m[5]+c*m[9]+a*m[1]+m[13];
    terrain::Matrix4 world_inverse{};std::copy_n(world.transforms.begin()+16,16,world_inverse.begin());
    const auto point=terrain::vector4_transform({static_cast<float>(u),static_cast<float>(v),
        static_cast<float>(-u-v),1},world_inverse,terrain::X87Precision::double_precision,terrain::Vec4Path::sse);
    const float plot_x=point[0]-height.center_x,plot_y=point[1]*.866f-height.center_y;
    controller.camera.world[12]+=plot_x;controller.camera.world[13]+=plot_y;
    controller.target[0]+=plot_x;controller.target[1]+=plot_y;
    auto views=terrain::camera_frame(world,terrain::game_world_grid_mode(state.world.map_header.wrap_x),
        controller.camera,{},terrain::X87Precision::double_precision,[](float value){return 1.f/value;});
    terrain::ViewBounds bounds{};
    for(unsigned view=0;view<views.view_count;++view){
        bounds.raw_low[view]=bounds.clipped_low[view]={world.bounds[0],world.bounds[1],0};
        bounds.raw_high[view]=bounds.clipped_high[view]={world.bounds[3],world.bounds[4],127};
        bounds.raw_tile_high[view]=bounds.clipped_tile_high[view]={static_cast<int>(grid.width),static_cast<int>(grid.height)};
    }
    auto color=graphics::GlTexture::render_target(*context,64,64,1,graphics::RenderTextureFormat::rgba8);
    auto depth=graphics::GlTexture::render_target(*context,64,64,1,graphics::RenderTextureFormat::depth24_stencil8);
    materials::FogOfWarSceneFrame prepared{};
    const ui::ClientViewport viewport{64,64,64,64,96};
    const auto render=[&]{return graphics::BnwFogFrameRenderer::render(bridge.binding(),state,*scene_receipt,
        *client.frame(),controller,views,world,height,bounds,{},0,prepared,map,color,depth);};
    check(!bridge.consume_committed(),"Known idle execution journal fabricated a new fog event");
    client.begin(viewport,controller);client.accept_rendered_fog(render());
    const auto before_pixels=map.mask().read_mip_raw(0);
    const auto before_sequence=bridge.journal().published->sequence();
    state.plot_visibility.counts.at(plot*cache.wire_slots+art.active_team)=0;
    check(!client.current(),"Unnotified logical update retained rendered frame");
    rejects([&]{bridge.consume_committed();});
    // Explicit source event, never a cache-diff inferred list. All of the real
    // activation staging below operates on the same enclosing execution owner.
    runtime::queue_bnw_deferred_fog(bridge.journal(),plot);
    const auto order=runtime::bnw_game_turn_order();
    std::vector<runtime::BnwTurnPhase> prefix(order.begin(),std::find(order.begin(),order.end(),runtime::BnwTurnPhase::game_activation));
    auto source_players=runtime::make_bnw_game_activation_source_player_handlers(host.database(),host.identity(),art.active_player);
    auto rollback=runtime::execute_bnw_game_activation_draft(state,activation,host.database(),host.identity(),prefix,source_players);
    check(!rollback.draft && std::find(rollback.completed_steps.begin(),rollback.completed_steps.end(),
        runtime::BnwGameActivationStep::active_player_deferred_fog)!=rollback.completed_steps.end() &&
        rollback.blocker.find("pathfinder_cache_enter")!=std::string::npos &&
        !activation.deferred_fog->pending && activation.deferred_fog->queued_plots==std::vector<std::uint64_t>{plot} &&
        activation.deferred_fog->published->sequence()==before_sequence && map.mask().read_mip_raw(0)==before_pixels,
        "Rejected real activation prefix leaked its private native fog stage");
    // Clearly labeled control-flow services satisfy unsupported later boundaries.
    // The fog stage itself remains the real owned source implementation; the
    // services cannot override it, and this test is not full-turn parity evidence.
    std::vector<runtime::BnwGameActivationService> controls;
    using Step=runtime::BnwGameActivationStep;
    for(unsigned id=0;id<=static_cast<unsigned>(Step::active_player_turn_start);++id){
        const auto step=static_cast<Step>(id);
        if(native_step(step) || step==Step::player_do_turn)continue;
        controls.push_back({step,[](auto &,auto){return runtime::LiveActionResult{runtime::LiveActionOutcome::committed,{}};}});
    }
    std::vector<runtime::BnwTurnHandler> players;
    for(const auto phase:runtime::bnw_player_turn_order())if(phase!=runtime::BnwTurnPhase::player_achievements)
        players.push_back({phase,[](auto &,auto){return runtime::LiveActionResult{runtime::LiveActionOutcome::committed,{}};},{}});
    auto executed=runtime::execute_bnw_game_activation_draft(state,activation,host.database(),host.identity(),prefix,players,controls);
    if(!executed.draft)throw std::runtime_error("Control-flow draft blocked: "+executed.blocker);
    check(executed.draft->owner.deferred_fog && executed.draft->owner.deferred_fog->pending &&
        executed.draft->owner.deferred_fog->pending->updates.size()==1 &&
        executed.draft->owner.deferred_fog->pending->updates[0].plot==plot,
        "Real native activation did not stage its source event in the returned owner");
    // Simulate the enclosing caller's successful world+owner publication. The
    // stable live owner retains its nested address under legitimate assignment.
    auto *journal_address=&bridge.journal();
    state=std::move(executed.draft->state);activation=std::move(executed.draft->owner);
    check(&bridge.journal()==journal_address && bridge.valid_handoff() && !client.current(),
        "Committed activation owner handoff detached the native consumer or retained old proof");
    check(bridge.consume_committed() && !activation.deferred_fog->pending &&
        activation.deferred_fog->published->sequence()==before_sequence+1 && !native.dirty(),
        "Committed native activation batch did not reach the same CPU journal/state");
    check(!activation.pending.empty(),"Fog consumer acknowledged unrelated activation publications");
    client.invalidate();client.begin(viewport,controller);client.accept_rendered_fog(render());
    const auto after_pixels=map.mask().read_mip_raw(0);
    const auto changed_bytes=std::count_if(after_pixels.begin(),after_pixels.end(),
        [&,index=std::size_t{0}](auto byte)mutable{return byte!=before_pixels.at(index++);});
    std::cout<<"current="<<client.current()<<" sequence="<<client.fog_receipt()->sequence()
             <<" changed_mask_bytes="<<changed_bytes<<'\n';
    check(client.current() && client.fog_receipt()->sequence()==before_sequence+1 && changed_bytes>0,
        "Committed execution journal failed the real CPU-to-GL-to-pixel consumer path");
    check(!bridge.consume_committed(),"Consumed activation fog batch replayed");
    // A semantic static-world change is never admitted by an actual fog receipt.
    const auto saved_terrain=state.world.map_projection->cells[plot].terrain_type;
    state.world.map_projection->cells[plot].terrain_type=999;
    rejects([&]{bridge.consume_committed();});check(!client.current(),"Bridge bypassed static renderer scene rebuild");
    state.world.map_projection->cells[plot].terrain_type=saved_terrain;
    // Monotonicity rejects a backwards journal from the same valid native baseline.
    activation.deferred_fog=baseline;
    check(!bridge.valid_handoff() && !client.current(),"Backwards execution receipt was rebound to old GPU frame");
    // Once a reset/foreign/backward owner has been observed the client requires
    // detach/rebind. Identical unobserved aggregate recreation is a lifecycle
    // contract, not a property this value-type runtime owner can universally detect.
    activation.deferred_fog.reset();check(!bridge.valid_handoff(),"Missing outer journal dereferenced stale storage");
    activation.deferred_fog=baseline;
    check(!bridge.valid_handoff(),"Observed detached journal became usable without rebind");
    std::cout<<context->description()<<'\n';
    std::cout<<"PASS selected source execution journal -> actual native CPU -> GL mask pixels; rollback/ownership guards\n";
    std::cout<<"Later activation phases are control-flow fixtures; full-turn parity remains pending\n";
}
}
int main(int argc,char **argv){
    try{check(argc==3,"Expected selected save and actual game root");run(argv[1],argv[2]);return 0;}
    catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
