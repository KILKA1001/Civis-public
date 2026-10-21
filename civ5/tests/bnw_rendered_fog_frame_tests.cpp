#include "bnw_client_scene_frame_fixture.hpp"
#include <iostream>
using namespace bnw_frame_fixture;
namespace {
void run_gpu() {
    auto live=session(); auto &state=live.mutable_state();
    runtime::rebuild_bnw_live_plot_visibility(state);
    auto scene_receipt=receipt(live);
    const runtime::SessionCheckpointIdentity identity{std::string(64,'2'),std::string(64,'3')};
    scene_receipt.restored_payload_sha256=identity.payload_sha256;
    scene_receipt.projection_reconciled=true;
    auto context=std::make_shared<graphics::GlContext>();
    const auto assets=materials::FogOfWarAssets::load(0,[](const char *)->assets::AssetHandle {
        throw std::runtime_error("Quality zero requested an authored cloud");
    });
    graphics::GlFogOfWarMap map(context,assets);
    auto world=terrain::build_world_layout(2,1,0,{},[](float value){return 1.f/value;});
    const auto height=terrain::HeightOwnerParameters::from_world(world);
    materials::FogOfWarState native(2,1,{},world.bounds[0],world.bounds[3]);
    const auto counts=scene::apply_saved_map_fog({2,1},
        saves::observe_saved_map_team_art(*state.world.pregame,*state.world.map_plots),native);
    native.tick(0.f);
    const auto baseline=graphics::capture_bnw_fog_client_baseline(state,identity,scene_receipt,
        counts,native,"real-gpu-test",false);
    auto journal=runtime::make_bnw_deferred_fog_owner(baseline);
    ui::BnwClientSceneFrame client(live,scene_receipt,7,{});
    client.bind_native_fog(journal,native,map,identity);
    terrain::CameraControllerState controller{};
    controller.camera={45.f,1.f,.1f,1000.f,terrain::identity_matrix()};
    controller.camera.world[14]=100;
    terrain::CameraFrameState views{};
    views=terrain::camera_frame(world,0,controller.camera,{},terrain::X87Precision::double_precision,
        [](float value){return 1.f/value;});
    terrain::ViewBounds bounds{};
    bounds.raw_low[0]=bounds.clipped_low[0]={-16,-16,0};
    bounds.raw_high[0]=bounds.clipped_high[0]={16,16,127};
    bounds.raw_tile_high[0]=bounds.clipped_tile_high[0]={2,1};
    auto color=graphics::GlTexture::render_target(*context,64,64,1,graphics::RenderTextureFormat::rgba8);
    auto depth=graphics::GlTexture::render_target(*context,64,64,1,graphics::RenderTextureFormat::depth24_stencil8);
    materials::FogOfWarSceneFrame prepared{};
    const ui::ClientViewport viewport{64,64,64,64,96};
    const auto render=[&](graphics::GlFogOfWarMap &target_map) {
        return graphics::BnwFogFrameRenderer::render({&journal,&native,&target_map,identity},state,
            scene_receipt,*client.frame(),controller,views,world,height,bounds,{},0,prepared,
            target_map,color,depth);
    };
    client.begin(viewport,controller);
    check(client.ready_to_draw() && !client.current() && !client.fog_receipt(),
        "CPU fog receipt accepted an unrendered GPU frame");
    const auto first=render(map); client.accept_rendered_fog(first);
    check(client.current() && client.fog_receipt()->sequence()==0 && map.render_revision()==1,
        "Real native baseline GL render was not accepted");
    const auto baseline_pixels=map.mask().read_mip_raw(0);
    check(!baseline_pixels.empty(),"Real mask storage was not readable");
    // The proof must belong to the exact consumer map, not a different map that
    // happens to share its logical owner, native state, camera and scene.
    graphics::GlFogOfWarMap foreign(context,assets);
    const auto foreign_proof=render(foreign);
    rejects([&]{client.accept_rendered_fog(foreign_proof);});
    check(client.current(),"Rejected foreign map destroyed the current real frame");
    map.render(prepared.commands,color,depth);
    check(!client.current() && !first.matches(*client.frame(),scene_receipt,state),
        "Another actual mask render retained an old GPU proof");
    client.accept_rendered_fog(render(map));
    const auto phase=*client.fog_receipt(); native.tick(.125f);
    check(!client.current() && !phase.matches(*client.frame(),scene_receipt,state),
        "Different fog animation phase retained a proof");
    client.accept_rendered_fog(render(map));
    bounds.raw_low[0][0]-=1;
    check(!client.current(),"Changed actual renderer bounds retained a proof");
    bounds.raw_low[0][0]+=1;
    client.accept_rendered_fog(render(map));
    state.plot_visibility.counts[3]=0; // actual derived cache only: static raw world unchanged
    check(!client.current(),"Unnotified/unapplied fog retained an old publication");
    rejects([&]{client.begin(viewport,controller);});
    runtime::queue_bnw_deferred_fog(journal,0);
    runtime::stage_bnw_deferred_fog_flush(state,journal);
    rejects([&]{client.begin(viewport,controller);});
    graphics::publish_bnw_deferred_fog(journal,native);
    check(native.dirty(),"Actual native journal application did not dirty FOW");
    rejects([&]{client.begin(viewport,controller);});
    native.tick(0.f); client.begin(viewport,controller);
    check(client.ready_to_draw() && !client.current(),"Applied CPU fog skipped its new real GPU draw");
    const auto applied=render(map); client.accept_rendered_fog(applied);
    check(client.current() && applied.sequence()==1 && applied.logical_revision()!=first.logical_revision() &&
          client.frame()->visibility.plots[0]==ui::WorldViewPlotFog::explored,
        "Actually applied and rendered fog update was rejected by static baseline");
    check(map.mask().read_mip_raw(0)!=baseline_pixels,
        "Published native visibility update did not change actual GL mask pixels");
    state.world.map_projection->cells[0].terrain_type=99;
    check(!client.current(),"Rendered fog proof permitted an incompatible raw static world");
    rejects([&]{client.begin(viewport,controller);});
    state.world.map_projection->cells[0].terrain_type=2;
    client.begin(viewport,controller);client.accept_rendered_fog(render(map));
    const auto old_revision=map.render_revision();
    auto recovered_context=std::make_shared<graphics::GlContext>();
    map.rebuild_context(recovered_context,assets);
    color=graphics::GlTexture::render_target(*recovered_context,64,64,1,graphics::RenderTextureFormat::rgba8);
    depth=graphics::GlTexture::render_target(*recovered_context,64,64,1,graphics::RenderTextureFormat::depth24_stencil8);
    check(map.render_revision()==old_revision+1 && !client.current(),
        "Recreated GPU mask retained proof from its old context resources");
    client.accept_rendered_fog(render(map));
    check(client.current(),"Actual render after map recovery failed to publish");
    const auto recovery=*client.fog_receipt();
    rejects([&]{
        graphics::BnwFogFrameRenderer::render({&journal,&native,&map,identity},state,scene_receipt,
            *client.frame(),controller,views,world,height,bounds,{},0,prepared,map,color,depth,{},
            [](const auto &){throw std::runtime_error("uncertain GPU composition");});
    });
    check(!client.current() && !recovery.matches(*client.frame(),scene_receipt,state),
        "Failed actual GPU composition retained a previous proof");
    // Mutations in LAND preparation are rejected before any mask write.
    const auto before=map.render_revision();
    rejects([&]{
        graphics::BnwFogFrameRenderer::render({&journal,&native,&map,identity},state,scene_receipt,
            *client.frame(),controller,views,world,height,bounds,{},0,prepared,map,color,depth,
            [&](const auto &){native.tick(.01f);});
    });
    check(map.render_revision()==before,"Changed preparation wrote a mask before rejection");
    std::cout<<context->description()<<'\n';
}
}
int main() {
    try {run_gpu();std::cout<<"BNW native rendered fog consumer tests passed\n";return 0;}
    catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
