// Validation of caller-coordinated live-context replacement. No lost WGL
// context or new independent native-renderer parity is implied by this check.
#include "material_world_validation.hpp"
#include "fog_of_war_scene.hpp"
#include "river_world_scene.hpp"
#include "camera_input_scene.hpp"

namespace {
Bytes scene_asset(const fs::path &root, const char *path, const char *name) {
    const auto source = archive(civ5::assets::AssetEnvironment(root), path);
    const auto found = source->find(civ5::assets::AssetKey(name));
    check(std::holds_alternative<civ5::assets::AssetHandle>(found), "Missing recovery source asset");
    return std::get<civ5::assets::AssetHandle>(found).read_bytes();
}
DistanceFogAssets scene_fog(const fs::path &root) {
    const civ5::assets::AssetEnvironment environment(root);
    const auto data = archive(environment, "Resource/Common/MiscData.fpk");
    const auto textures = archive(environment, "Resource/DX9/MiscTextures.fpk");
    return DistanceFogAssets::load([&](const char *path) {
        const auto &source = std::string_view(path) == DistanceFogAssets::color_path ? data : textures;
        const auto found = source->find(civ5::assets::AssetKey(fs::path(path).filename().string()));
        check(std::holds_alternative<civ5::assets::AssetHandle>(found), "Missing recovery distance fog");
        return std::get<civ5::assets::AssetHandle>(found);
    });
}
template<class T> Bytes raw(const std::vector<T> &values) {
    const auto *begin = reinterpret_cast<const std::uint8_t *>(values.data());
    return values.empty() ? Bytes{} : Bytes(begin, begin + values.size() * sizeof(T));
}
std::size_t different_pixels(const Bytes &a, const Bytes &b) {
    check(a.size() == b.size() && a.size() % 16 == 0, "Float32 image size");
    std::size_t count = 0;
    for (std::size_t at = 0; at < a.size(); at += 16)
        count += std::memcmp(a.data() + at, b.data() + at, 16) != 0;
    return count;
}
struct Capture {
    Bytes color, depth, mask;
};
struct ExternalScene {
    Control controls;
    SceneTarget target;
    GlDistanceFog fog;
    GlColorKeys keys;
    std::shared_ptr<GlTexture> reflection_depth;
    GlLandMaterialInputs land;
    GlWaterSurfaceInputs water;
    GlRiverSurfaceInputs river;
    ExternalScene(std::shared_ptr<GlContext> context, const DistanceFogAssets &fog_assets,
                  const ColorKeyAssets &key_assets)
        : controls(context), target(context, 1280, 960), fog(*context, fog_assets),
          keys(*context, key_assets),
          reflection_depth(std::make_shared<GlTexture>(GlTexture::render_target(
              *context, 2048, 2048, 1, RenderTextureFormat::depth24_stencil8))) {
        const auto neutral = neutral_scene();
        for (unsigned i = 1; i < 10; ++i) land.pixel[i] = controls.image(neutral[i]);
        land.pixel[2] = fog.ramp().id();
        const auto zero = controls.image({1, false, {0, 0, 0, 0}});
        water.pixel[8] = fog.ramp().id();
        water.pixel[9] = water.pixel[13] = zero;
        water.pixel[11] = keys.at(ColorKeySystem::terrain_fow, 0).id();
        river.pixel[9] = zero;
        river.pixel[11] = water.pixel[11];
        river.pixel[12] = keys.at(ColorKeySystem::rivers, 0).id();
    }
};
}

int main(int argc, char **argv) {
    fs::path output;
    bool created = false;
    try {
        check(argc == 4, "Usage: civ5_gl_scene_context_recovery asset-root primitive-input fresh-output");
        const fs::path root(argv[1]);
        output = argv[3];
        check(!fs::exists(output), "Fresh context-recovery output required");
        fs::create_directories(output);
        created = true;
        auto input = primitive_input(argv[2]);
        auto world = build_world(root, input, 0);
        const std::shared_ptr<const TerrainWorld> terrain(world, &world->terrain());
        const auto cubes = cube_assets(root);
        const auto keys = color_key_validation::load(root);
        const auto fog_assets = scene_fog(root);
        const auto waves = civ5::assets::LeanData::parse(
            scene_asset(root, "Resource/Common/TerrainModels.fpk", "waterbumps16.lean"));
        const auto colors = civ5::assets::DdsData::parse(
            scene_asset(root, "Resource/DX9/Upgrade1Textures.fpk", "waterdepthcolor_new.dds"));
        const auto river_assets = load_river_world_assets(root);
        auto context = std::make_shared<GlContext>();
        blend_validation::text(output / "device.txt", context->description());
        GlMaterialWorldRenderer land(context, world, cubes, {false, true, true},
            LightingCubeSampling::measured_coordinates, LandShadowArithmetic::measured_fma_t_x_z_y,
            LandMipSampling::d3d9_rgb565, std::make_shared<const GlColorKeys>(*context, keys));
        GlWaterWorldRenderer water(context, terrain, waves, colors, 3, X87Precision::double_precision);
        GlRiverWorldRenderer river(context, terrain, river_assets);
        FogOfWarScene fow(context, *terrain, root);
        auto external = std::make_unique<ExternalScene>(context, fog_assets, keys);
        water.prepare_current({.17f, .09f, 3.25f}, true);
        water.prepare_reflection(2, 1, external->reflection_depth);
        const auto rcp = [](float x) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(x))); };
        const auto rsqrt = [](float x) { return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x))); };
        const auto lighting = terrain_lighting_defaults(X87Precision::double_precision);
        const auto light = GlMaterialWorldLighting::from_terrain(lighting, rsqrt);
        const auto &domain = water.domain();
        const WaterReflectionFrameSettings settings{
            lighting.angles, lighting.shadow, {domain.scales[0], domain.scales[1]}};
        CameraControllerState prior{};
        prior.wrap_orientation = 1;
        auto controller = water.prepare_camera_startup({1280, 960, 11}, prior, settings, rcp, rsqrt);
        CameraPointerState pointer{1280, 960};
        const CameraInputSettings defaults{{1, 0}, false, true};
        float zoom = 0;
        const auto steps = camera_scene_input_steps();
        // The accepted combined-scene corpus has visible rivers at this camera
        // checkpoint. Recompute it from pointer/button input, not stored poses.
        for (unsigned step = 0; step <= 266; ++step) {
            const auto &s = steps.at(step);
            const auto result = water.prepare_camera_input(
                controller, pointer, {s.dt, s.commands, s.input_mode, defaults, s.events},
                settings, rcp, rsqrt);
            controller = result.motion.controller;
            pointer = result.pointer;
            zoom = result.motion.zoom[0];
        }
        const auto sky = water_sky_pixels(civ5::assets::DdsData::parse(
            scene_asset(root, "Resource/DX9/TerrainTextures.fpk", "watersky.dds")));
        auto surface = initial_water_surface_settings(13000, water_sky_lighting(sky, rsqrt));
        const auto prepare_land = [&] {
            const auto seed = [&](int, int, float x) { return rsqrt(x); };
            for (std::size_t i = 0; i < world->cache().patches.size(); ++i)
                land.prepare(i, {64, 0, 0, 0, X87Precision::double_precision}, {512, 256},
                             seed, seed, light, 4, true);
        };
        prepare_land();
        std::cout << "Prepared " << world->cache().patches.size() << " LAND patches\n" << std::flush;
        const auto capture = [&](unsigned omit) {
            auto &e = *external;
            fow.prepare(water, e.fog, zoom);
            WaterSurfaceFrame frame{};
            // The production water owner has the same safe fallback used by
            // the established scene validator before the optional fog frame.
            frame.fog_parameters = {.002f, 0, 0, 0};
            const auto global = e.fog.frame(zoom);
            frame.fog_color = global.color;
            if (std::isfinite(global.parameters[0]) && global.parameters[0] != 0)
                frame.fog_parameters = global.parameters;
            frame.projection = water.camera_state().projection;
            const auto scene = [&](const GlTexture &mask) {
                e.target.begin();
                e.land.pixel[4] = mask.id();
                e.land.pixel[5] = fow.map.clouds()[0].id();
                e.water.pixel[10] = e.river.pixel[10] = mask.id();
                e.river.pixel[4] = water.reflection_map().texture().id();
                const auto count = water.camera_state().view_count;
                if (omit != 1)
                    for (unsigned view = 0; view < count; ++view)
                        for (std::size_t i = 0; i < world->cache().patches.size(); ++i)
                            land.draw(i, fow.land.at(view), e.land, 1280, 960);
                for (unsigned view = 0; view < count; ++view) {
                    WaterSurfaceView v{};
                    v.overlay = fow.frame.overlays[view];
                    v.view = water.camera_state().views[view];
                    v.shadow = water.frame_state().matrices.shadow[view];
                    v.reflection = water.frame_state().matrices.reflection[view];
                    std::copy_n(water.camera_state().inverse_views[view].begin() + 12, 3, v.camera.begin());
                    try {
                        const auto constants = water.constants(surface, frame, v);
                        if (omit != 2) water.draw(view, constants, e.water, 1280, 960);
                    } catch (const std::invalid_argument &error) {
                        std::ostringstream detail;
                        detail << error.what() << " fog=" << frame.fog_parameters[0] << ","
                               << frame.fog_parameters[1] << " camera="
                               << water.camera_state().inverse_views[view][14];
                        throw std::invalid_argument(detail.str());
                    }
                }
                if (omit != 3)
                    for (unsigned view = 0; view < count; ++view) {
                        WaterSurfaceView v{};
                        v.overlay = fow.frame.overlays[view];
                        v.view = water.camera_state().views[view];
                        v.shadow = water.frame_state().matrices.shadow[view];
                        v.reflection = water.frame_state().matrices.reflection[view];
                        std::copy_n(water.camera_state().inverse_views[view].begin() + 12, 3, v.camera.begin());
                        try {
                            river.draw(water.constants(surface, frame, v), e.river, 1280, 960);
                        } catch (const std::invalid_argument &error) {
                            std::ostringstream detail;
                            detail << error.what() << " river-fog=" << frame.fog_parameters[0] << ","
                                   << frame.fog_parameters[1] << " river-camera="
                                   << water.camera_state().inverse_views[view][14];
                            throw std::invalid_argument(detail.str());
                        }
                    }
            };
            if (omit == 4) {
                auto mask_only = fow.frame.commands;
                mask_only.draws.resize(mask_only.passes[0].end);
                mask_only.passes.resize(1);
                fow.map.render(mask_only, *e.target.color_storage, *e.target.depth_storage, scene);
            } else fow.render(e.target, scene);
            auto pixels = e.target.pixels();
            check(std::all_of(pixels.begin(), pixels.end(), [](float x){ return std::isfinite(x); }),
                  "Nonfinite recovered composition");
            land.finish_frame();
            return Capture{raw(pixels), e.target.depth_storage->read_mip_raw(0), fow.map.mask().read_mip_raw(0)};
        };
        std::array<Capture, 5> baseline;
        for (unsigned omit = 0; omit < baseline.size(); ++omit) baseline[omit] = capture(omit);
        std::array<std::size_t, 4> contributions;
        for (unsigned i = 0; i < contributions.size(); ++i) {
            contributions[i] = different_pixels(baseline[0].color, baseline[i + 1].color);
            check(contributions[i] > 0, "A composition layer made no visible contribution");
        }
        const auto mesh = water.mesh();
        const auto river_mesh = river.mesh();
        WaterSurfaceFrame comparison_frame{};
        comparison_frame.projection = water.camera_state().projection;
        comparison_frame.fog_parameters = {.002f, 0, 0, 0};
        std::vector<WaterConstantBlock> constants;
        for (unsigned view = 0; view < water.camera_state().view_count; ++view) {
            WaterSurfaceView v{};
            v.view = water.camera_state().views[view];
            v.shadow = water.frame_state().matrices.shadow[view];
            v.reflection = water.frame_state().matrices.reflection[view];
            std::copy_n(water.camera_state().inverse_views[view].begin() + 12, 3, v.camera.begin());
            constants.push_back(water.constants(surface, comparison_frame, v));
        }
        const auto current = water.current_map().texture().read_mip_raw(0);
        const auto reflection = water.reflection_map().texture().read_mip_raw(0);
        auto next = std::make_shared<GlContext>();
        auto next_external = std::make_unique<ExternalScene>(next, fog_assets, keys);
        unsigned guards = 0;
        const auto rejects = [&](auto action) {
            bool caught = false;
            try { action(); } catch (const std::invalid_argument &) { caught = true; }
            check(caught, "Missing scene recovery guard");
            ++guards;
        };
        rejects([&] { water.rebuild_context(nullptr, waves, colors); });
        rejects([&] { water.rebuild_context(context, waves, colors); });
        rejects([&] { water.rebuild_context(next, waves, colors); });
        rejects([&] { water.rebuild_context(next, waves, colors, external->reflection_depth); });
        auto small = std::make_shared<GlTexture>(
            GlTexture::render_target(*next, 64, 64, 1, RenderTextureFormat::depth24_stencil8));
        rejects([&] { water.rebuild_context(next, waves, colors, small); });
        rejects([&] { river.rebuild_context(nullptr, river_assets); });
        rejects([&] { river.rebuild_context(context, river_assets); });
        rejects([&] { fow.map.rebuild_context(nullptr, fow.assets); });
        rejects([&] { fow.map.rebuild_context(context, fow.assets); });
        unsigned callbacks = 0;
        water.prepare_camera_frame(controller.camera, settings, [&](float x) {
            rejects([&] { water.rebuild_context(next, waves, colors, next_external->reflection_depth); });
            ++callbacks;
            return rcp(x);
        }, rsqrt);
        check(callbacks > 0, "Camera recovery guard was not reached");
        water.render_reflection([&] {
            rejects([&] { water.rebuild_context(next, waves, colors, next_external->reflection_depth); });
        });
        fow.render(external->target, [&](const GlTexture &) {
            rejects([&] { fow.map.rebuild_context(next, fow.assets); });
        });
        const auto after_rejection = capture(0);
        check(after_rejection.color == baseline[0].color && after_rejection.depth == baseline[0].depth &&
              after_rejection.mask == baseline[0].mask, "Rejected recovery changed complete frame");
        // Preserve another context's framebuffer when callbacks return or
        // throw after switching contexts (including the one-pass FOW case).
        const auto foreign_callback = [&] {
            next->api();
            next_external->target.gl.BindFramebuffer(GL_FRAMEBUFFER, next_external->target.fbo);
        };
        const auto check_bindings = [&] {
            GLint bound = -1;
            next->api().GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound);
            check(bound == static_cast<GLint>(next_external->target.fbo), "Callback changed foreign framebuffer");
            context->api().GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound);
            check(bound == 0, "Callback did not release owning framebuffer");
        };
        auto mask_only = fow.frame.commands;
        mask_only.draws.resize(mask_only.passes[0].end);
        mask_only.passes.resize(1);
        for (bool fail : {false, true}) {
            bool thrown = false;
            try {
                water.render_reflection([&] { foreign_callback(); if (fail) throw std::runtime_error("child"); });
            } catch (const std::runtime_error &) { thrown = true; }
            check(thrown == fail, "Reflection callback exception propagation");
            check_bindings();
            thrown = false;
            try {
                fow.map.render(mask_only, *external->target.color_storage, *external->target.depth_storage,
                    [&](const GlTexture &) { foreign_callback(); if (fail) throw std::runtime_error("child"); });
            } catch (const std::runtime_error &) { thrown = true; }
            check(thrown == fail, "FOW callback exception propagation");
            check_bindings();
        }
        water.rebuild_context(next, waves, colors, next_external->reflection_depth);
        rejects([&] { water.reflection_map().texture(); });
        river.rebuild_context(next, river_assets);
        fow.map.rebuild_context(next, fow.assets);
        land.rebuild_context(next, cubes, &keys);
        check(water.mesh().vertices == mesh.vertices && water.mesh().indices == mesh.indices &&
              water.mesh().counts == mesh.counts && water.mesh().prefixes == mesh.prefixes &&
              water.mesh().capacity == mesh.capacity, "Replacement changed retained ocean mesh");
        check(river.mesh().vertices == river_mesh.vertices && river.mesh().indices == river_mesh.indices,
              "Replacement changed river geometry");
        for (unsigned view = 0; view < constants.size(); ++view) {
            WaterSurfaceView v{};
            v.view = water.camera_state().views[view];
            v.shadow = water.frame_state().matrices.shadow[view];
            v.reflection = water.frame_state().matrices.reflection[view];
            std::copy_n(water.camera_state().inverse_views[view].begin() + 12, 3, v.camera.begin());
            check(water.constants(surface, comparison_frame, v) == constants[view],
                  "Replacement changed camera/frame");
        }
        check(water.current_map().texture().read_mip_raw(0) == current, "Replacement changed current map");
        water.render_reflection();
        check(water.reflection_map().texture().read_mip_raw(0) == reflection, "Replacement changed reflection");
        external = std::move(next_external);
        context = next;
        prepare_land();
        for (unsigned omit = 0; omit < baseline.size(); ++omit) {
            const auto after = capture(omit);
            check(after.color == baseline[omit].color && after.depth == baseline[omit].depth &&
                  after.mask == baseline[omit].mask, "Context replacement changed full composition");
        }
        context->check("Complete replacement scene");
        std::ostringstream report;
        report << "{\"schema\":\"civ5-scene-context-replacement-v1\",\"complete\":true,"
               << "\"framesCompared\":5,\"width\":1280,\"height\":960,\"landPatches\":"
               << world->cache().patches.size() << ",\"riverVertices\":" << river.mesh().vertices.size()
               << ",\"rejectedOperations\":" << guards << ",\"changedPixelsWithoutLayer\":[";
        for (unsigned i = 0; i < contributions.size(); ++i) report << (i ? "," : "") << contributions[i];
        report << "],\"layerOrder\":[\"LAND\",\"ocean\",\"river\",\"FOW-depth-clouds\"],"
               << "\"fullCompositionBitExact\":true,\"foreignCallbackBindingsPreserved\":true,"
               << "\"unusableContextRecovery\":false,\"independentOriginalGpuComparison\":false,"
               << "\"fullGraphicsParity\":false}\n";
        blend_validation::text(output / "result.json", report.str());
        std::cout << report.str();
    } catch (const std::exception &error) {
        if (created) blend_validation::text(output / "failure.txt", error.what());
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
