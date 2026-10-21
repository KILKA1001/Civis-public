#pragma once
#include "civ5/graphics/bnw_rendered_fog_frame.hpp"
// Scene composition around production owners. The visibility pattern is an
// explicit diagnostic input; no gameplay visibility policy is inferred.
namespace {
FogOfWarAssets load_scene_fow(const fs::path &root) {
    using namespace civ5::assets;
    const AssetEnvironment environment(root);
    const auto source = archive(environment, "Resource/DX9/EffectTextures.fpk");
    return FogOfWarAssets::load(2, [&](const char *path) {
        const auto found = source->find(AssetKey(path));
        check(std::holds_alternative<AssetHandle>(found), "Missing/ambiguous authored FOW cloud");
        return std::get<AssetHandle>(found);
    });
}
struct FogOfWarScene {
    const TerrainWorld &world;
    FogOfWarAssets assets;
    FogOfWarState state;
    GlFogOfWarMap map;
    FogOfWarSceneFrame frame{};
    std::vector<TerrainFrameState> land_frames;
    std::vector<LandMaterialConstants> land;
    FogOfWarScene(std::shared_ptr<GlContext> context, const TerrainWorld &w, const fs::path &root,
                  bool diagnostic_pattern = true, const FogOfWarAssets* actual_assets = nullptr)
        : world(w), assets(actual_assets ? *actual_assets : load_scene_fow(root)),
          state(w.source().layout.source_grid.width, w.source().layout.source_grid.height, {},
                w.source().layout.bounds[0], w.source().layout.bounds[3]),
          map(context, assets) {
        if (diagnostic_pattern) {
            for (int x = 0; x < state.width(); ++x)
                for (int y = 0; y < state.height(); ++y) {
                    const auto visibility = (x / 7 + 2 * (y / 5)) % 3;
                    const auto a = x - (y >> 1);
                    state.visibility_event({a, y, -a - y}, static_cast<std::uint8_t>(2 - visibility));
                }
            state.tick(.125f);
        }
    }
    void prepare(const GlWaterWorldRenderer &water, const GlDistanceFog &fog, float zoom) {
        const auto &camera = water.camera_state();
        auto next = fog_of_war_scene_frame(world.source().layout, world.owner().parameters, state, camera,
                                           water.frame_state().bounds, frame.overlays, assets.state().quality,
                                           true, false, false, X87Precision::double_precision);
        frame = std::move(next);
        prepare_land(water,fog,zoom);
    }
    void prepare_land(const GlWaterWorldRenderer &water,const GlDistanceFog &fog,float zoom) {
        const auto &camera = water.camera_state();
        const auto lighting = terrain_lighting_defaults(X87Precision::double_precision);
        const auto &h = world.owner().parameters;
        const auto global = fog.frame(zoom);
        std::vector<TerrainFrameState> frames;
        std::vector<LandMaterialConstants> constants;
        for (unsigned i = 0; i < camera.view_count; ++i) {
            const TerrainFrameInput input{
                camera.views[i],
                camera.inverse_views[i],
                camera.projection,
                water.frame_state().matrices.shadow[i],
                frame.overlays[i],
                {h.scale_x, h.scale_y, h.scale_z},
                lighting.shadow,
                lighting.sun,
                {static_cast<unsigned>(state.width()), static_cast<unsigned>(state.height())},
                {1280, 960},
                state.frame_scale(),
                global.color,
                global.parameters[0],
                global.parameters[1],
                global.parameters[3],
                2048,
                0,
                1,
                {2, 1, 1, 1}};
            frames.push_back(terrain_frame(input, {}, X87Precision::double_precision, [](float v) {
                return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v)));
            }));
            constants.push_back(land_material_frame(frames.back()));
        }
        land_frames = std::move(frames);
        land = std::move(constants);
    }
    void render(SceneTarget &target, const std::function<void(const GlTexture &)> &scene) {
        map.render(frame.commands, *target.color_storage, *target.depth_storage, scene);
    }
    civ5::graphics::BnwFogRenderedFrameReceipt render_with_receipt(
        SceneTarget &target, const GlWaterWorldRenderer &water, const GlDistanceFog &fog,
        float zoom, const civ5::terrain::CameraControllerState &controller,
        const civ5::runtime::LiveSessionState &live,
        const civ5::runtime::SessionCheckpointIdentity &identity,
        const civ5::graphics::BnwRendererSceneReceipt &scene_receipt,
        const civ5::runtime::BnwDeferredFogOwner &owner,
        const civ5::ui::BnwClientFrame &client_frame,
        const std::function<void(const GlTexture &)> &scene,
        const civ5::graphics::BnwNativeFogFrameBinding *adopted=nullptr) {
        auto binding=civ5::graphics::BnwNativeFogFrameBinding{&owner,&state,&map,identity};
        if(adopted) {
            check(adopted->owner==&owner && adopted->native==&state && adopted->map==&map &&
                adopted->identity.payload_sha256==identity.payload_sha256 &&
                adopted->identity.rules_sha256==identity.rules_sha256,
                "FOW scene binding differs from the actual execution/native owners");
            binding=*adopted;
        }
        return civ5::graphics::BnwFogFrameRenderer::render(
            binding,live,scene_receipt,client_frame,controller,
            water.camera_state(),world.source().layout,world.owner().parameters,
            water.frame_state().bounds,frame.overlays,assets.state().quality,frame,map,
            *target.color_storage,*target.depth_storage,
            [&](const FogOfWarSceneFrame &) { prepare_land(water,fog,zoom); },scene);
    }

};
} // namespace
