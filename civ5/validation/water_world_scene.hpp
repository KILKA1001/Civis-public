#pragma once
#include "scheduled_material_scene.hpp"
#include "water_animation_scene.hpp"
#include "water_current_scene.hpp"
#include "water_reflection_scene.hpp"
#include "camera_controller_storage.hpp"
#include "camera_input_scene.hpp"
#include "fog_of_war_scene.hpp"
#include "river_world_scene.hpp"
#include "fow_world_scene.hpp"
#include "fow_scene_sequence.hpp"
// Complete LAND + ocean image under explicitly declared scene controls.
// Neither API consumes the other's rendered image or cache output here.
namespace {
civ5::assets::Bytes water_asset(const fs::path &root, const char *archive_name, const char *key) {
    const civ5::assets::AssetEnvironment environment(root);
    const auto source = archive(environment, archive_name);
    const auto found = source->find(civ5::assets::AssetKey(key));
    check(std::holds_alternative<civ5::assets::AssetHandle>(found), "Missing original water scene asset");
    return std::get<civ5::assets::AssetHandle>(found).read_bytes();
}
DistanceFogAssets load_distance_fog(const fs::path &root) {
    using namespace civ5::assets;
    const AssetEnvironment environment(root);
    const auto data = archive(environment, "Resource/Common/MiscData.fpk");
    const auto textures = archive(environment, "Resource/DX9/MiscTextures.fpk");
    return DistanceFogAssets::load([&](const char *path) {
        const auto &source = std::string_view(path) == DistanceFogAssets::color_path ? data : textures;
        const auto found = source->find(AssetKey(fs::path(path).filename().string()));
        check(std::holds_alternative<AssetHandle>(found), "Missing/ambiguous DistanceFog archive entry");
        return std::get<AssetHandle>(found);
    });
}
void save_distance_fog(const GlDistanceFog &fog, const fs::path &output) {
    fs::create_directory(output);
    save(output / "color.csv", fog.assets().color_bytes());
    save(output / "ramp.dds", fog.assets().ramp().original_bytes());
    const auto &state = fog.assets().state();
    water_validation::save(output / "startup.bin", std::array{state.density, state.strength, state.color[0],
                                                              state.color[1], state.color[2]});
    for (unsigned mip = 0; mip < fog.assets().ramp().mip_count(); ++mip) {
        const auto bytes = fog.ramp().read_mip_raw(mip);
        const auto source = fog.assets().ramp().payload(0, mip);
        check(std::equal(bytes.begin(), bytes.end(), source.begin(), source.end()),
              "DistanceFog GL mip bytes changed");
        save(output / ("ramp-mip" + std::to_string(mip) + ".bin"), bytes);
    }
}
unsigned water_owner_checks(std::shared_ptr<GlContext> context, std::shared_ptr<const TerrainWorld> terrain,
                            const civ5::assets::LeanData &waves, const civ5::assets::DdsData &color) {
    unsigned guards = 0;
    const auto rejects = [&](auto action) {
        bool rejected = false;
        try {
            action();
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        check(rejected, "Water world did not reject an invalid request");
        ++guards;
    };
    rejects(
        [&] { GlWaterWorldRenderer r(nullptr, terrain, waves, color, 3, X87Precision::double_precision); });
    rejects(
        [&] { GlWaterWorldRenderer r(context, nullptr, waves, color, 3, X87Precision::double_precision); });
    rejects(
        [&] { GlWaterWorldRenderer r(context, terrain, waves, color, 1, X87Precision::double_precision); });
    rejects(
        [&] { GlWaterWorldRenderer r(context, terrain, waves, color, 3, static_cast<X87Precision>(999)); });
    const auto owners = terrain.use_count();
    {
        GlWaterWorldRenderer fresh(context, terrain, waves, color, 3, X87Precision::double_precision);
        const auto &d = fresh.domain();
        const PerspectiveCamera camera{
            45,
            4.f / 3,
            1,
            10000,
            {1, 0, 0, 0, 0, .70710677f, .70710677f, 0, 0, -.70710677f, .70710677f, 0, 0, -2200, 2200, 1}};
        const WaterReflectionFrameSettings settings{
            {23.75f, 61.125f, 117.5f, 38.25f}, {.3f, -.7f, .25f}, {d.scales[0], d.scales[1]}};
        fresh.prepare_camera_frame(
            camera, settings, [](float x) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(x))); },
            [](float x) { return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x))); });
        const auto count = fresh.camera_state().view_count;
        check(count == 3 && terrain->source().map_mode == 1 && fresh.mesh().counts.size() == count * 256 &&
                  !fresh.draws().empty(),
              "First perspective frame did not publish all original wrapped views");
        ++guards;
    }
    {
        GlWaterWorldRenderer water(context, terrain, waves, color, 3, X87Precision::double_precision);
        check(terrain.use_count() == owners + 1, "Water world does not retain its immutable terrain");
        ++guards;
        rejects([&] { water.mesh(); });
        rejects([&] { water.draws(); });
        rejects([&] { water.current_map(); });
        rejects([&] { water.reflection_map(); });
        rejects([&] { water.render_reflection(); });
        rejects([&] { water.draw(0, {}, {}, 64, 64); });
        rejects([&] { water.height_texture(256); });
        rejects([&] { water.wave_texture(static_cast<civ5::assets::LeanMap>(99)); });
        const auto &d = water.domain();
        const Vector4 full{d.origin_x, d.origin_y, d.origin_x + d.span_x, d.origin_y + d.span_y};
        const std::array<Vector4, 1> views{full};
        water.prepare(views);
        const auto *mesh = &water.mesh();
        const auto *draws = water.draws().data();
        water.prepare_reflection(2, 0);
        const auto *reflection = &water.reflection_map();
        const auto fallback_id = reflection->texture().id();
        check(reflection->texture().read_mip_raw(0) == civ5::assets::Bytes(4096, 0),
              "World original reflection fallback");
        ++guards;
        rejects([&] { water.prepare_reflection(2, 1); });
        check(&water.reflection_map() == reflection, "Invalid shared depth replaced reflection map");
        ++guards;
        auto shared_depth = std::make_shared<GlTexture>(
            GlTexture::render_target(*context, 2048, 2048, 1, RenderTextureFormat::depth24_stencil8));
        water.prepare_reflection(2, 1, shared_depth);
        check(!context->api().IsTexture(fallback_id) && shared_depth.use_count() == 2,
              "World reflection target ownership");
        ++guards;
        reflection = &water.reflection_map();
        const auto reflection_id = reflection->texture().id();
        bool failed_child = false;
        try {
            water.prepare_reflection(1, 1, shared_depth,
                                     [] { throw std::runtime_error("Failed child draw"); });
        } catch (const std::runtime_error &) {
            failed_child = true;
        }
        check(failed_child && &water.reflection_map() == reflection &&
                  reflection->texture().read_mip_raw(0) == civ5::assets::Bytes(1024 * 1024 * 4, 0) &&
                  shared_depth.use_count() == 2,
              "Failed world reflection rebuild did not retain completed target");
        ++guards;
        water.render_reflection([&] {
            rejects([&] { water.prepare_reflection(0, 0); });
            rejects([&] { water.render_reflection(); });
        });
        check(water.reflection_map().texture().id() == reflection_id && &water.mesh() == mesh &&
                  water.draws().data() == draws,
              "Reflection frame replaced target or water geometry");
        ++guards;
        water.prepare_reflection(0, 0);
        check(!context->api().IsTexture(reflection_id) && shared_depth.use_count() == 1,
              "World fallback releases previous reflection target and shared depth");
        ++guards;
        rejects([&] { water.render_reflection(); });
        rejects([&] { water.prepare_reflection(0, 0, {}, [] {}); });
        water.prepare_current();
        const auto &grid = terrain->source().layout.worker_grid;
        check(water.current_map().constants()[0] == static_cast<float>(grid.width) &&
                  water.current_map().constants()[1] == static_cast<float>(grid.height),
              "Current map did not use its own worker grid");
        ++guards;
        const auto *current = &water.current_map();
        const auto pixels = current->texture().read_mip_raw(0);
        const auto is_texture = reinterpret_cast<PFNGLISTEXTUREPROC>(context->proc_address("glIsTexture"));
        const auto first_id = current->texture().id();
        for (unsigned i = 0; i < 3; ++i) {
            WaterCurrentSettings invalid;
            if (i == 0)
                invalid.flow_strength = std::numeric_limits<float>::quiet_NaN();
            if (i == 1)
                invalid.noise_strength = std::numeric_limits<float>::infinity();
            if (i == 2)
                invalid.noise_scale = -std::numeric_limits<float>::infinity();
            rejects([&] { water.prepare_current(invalid); });
            check(&water.current_map() == current && current->texture().read_mip_raw(0) == pixels &&
                      &water.mesh() == mesh && water.draws().data() == draws,
                  "Rejected current rebuild changed published resources");
            ++guards;
        }
        water.prepare_current({.37f, -.125f, 4.f}, true);
        check(water.current_map().texture().id() != first_id && !is_texture(first_id) &&
                  water.current_map().texture().read_mip_raw(0) != pixels && &water.mesh() == mesh &&
                  water.draws().data() == draws,
              "Current replacement lost its settings, retained old storage or changed geometry");
        ++guards;
        water.prepare_current();
        check(water.current_map().texture().read_mip_raw(0) == pixels,
              "Current default rebuild changed pixels");
        ++guards;
        const auto field = terrain->height_field();
        for (const auto &draw : water.draws())
            check(water.height_texture(draw.window).read_mip_raw(0) ==
                      field.extract_a8_texture(draw.window % 16, draw.window / 16),
                  "Water world height differs from its own source field");
        ++guards;
        const auto reject_prepare = [&](auto bounds) {
            rejects([&] { water.prepare(bounds); });
            check(&water.mesh() == mesh && water.draws().data() == draws,
                  "Rejected water rebuild replaced published geometry");
            ++guards;
        };
        std::array<Vector4, 5> too_many{};
        reject_prepare(too_many);
        auto nonfinite = views;
        nonfinite[0][0] = std::numeric_limits<float>::quiet_NaN();
        reject_prepare(nonfinite);
        auto outside = views;
        outside[0][2] += d.span_x;
        outside[0][3] += d.span_y;
        reject_prepare(outside);
        rejects([&] { water.draw(1, {}, {}, 64, 64); });
        const std::array<Vector4, 2> multi{{{0, 0, -1, -1}, full}};
        water.prepare(multi);
        check(!water.draws().empty() && std::all_of(water.draws().begin(), water.draws().end(),
                                                    [](const auto &draw) { return draw.view == 1; }),
              "Water view groups lost the empty leading view");
        ++guards;
        water.prepare({});
        check(water.mesh().vertices.empty() && water.mesh().indices.empty() && water.draws().empty(),
              "Zero water views retained stale geometry");
        ++guards;
        water.prepare(views);
        check(!water.draws().empty(), "Water geometry cannot be rebuilt after an empty view set");
        ++guards;
        rejects([&] { water.frame_state(); });
        rejects([&] { water.constants(0, {}, {}, {}); });
        const civ5::terrain::Matrix4 identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        WaterReflectionCamera camera{{}, 2, identity, {identity, identity, identity}};
        camera.frusta[0] = {Vector4{},
                            Vector4{},
                            Vector4{1, 0, 0, -full[0]},
                            Vector4{-1, 0, 0, full[2]},
                            Vector4{0, 1, 0, -full[1]},
                            Vector4{0, -1, 0, full[3]}};
        camera.frusta[1] = camera.frusta[0];
        camera.frusta[1][3][3] = (full[0] + full[2]) * .5f;
        camera.extra_views[0][12] = 100;
        const WaterReflectionFrameSettings settings{
            {23.75f, 61.125f, 117.5f, 38.25f}, {.3f, -.7f, .25f}, {d.scales[0], d.scales[1]}};
        const auto estimate = [](float squared) { return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared))); };
        water.prepare_frame(camera, settings, estimate);
        check(water.mesh().counts.size() == 512 && !water.draws().empty() &&
                  water.frame_state().matrices.reflection[0] != water.frame_state().matrices.reflection[1],
              "Prepared camera lost its independent wrapped-view matrices");
        ++guards;
        const auto *prepared_frame = &water.frame_state();
        const auto *camera_mesh = &water.mesh();
        const auto *camera_draws = water.draws().data();
        const auto retained_frame = *prepared_frame;
        const auto retained = [&] {
            check(&water.frame_state() == prepared_frame && &water.mesh() == camera_mesh &&
                      water.draws().data() == camera_draws &&
                      std::memcmp(prepared_frame, &retained_frame, sizeof(retained_frame)) == 0,
                  "Failed camera update changed the published frame or geometry");
            ++guards;
        };
        auto invalid_camera = camera;
        invalid_camera.view_count = 5;
        rejects([&] { water.prepare_frame(invalid_camera, settings, estimate); });
        retained();
        invalid_camera = camera;
        invalid_camera.frusta[0][2][0] = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { water.prepare_frame(invalid_camera, settings, estimate); });
        retained();
        rejects([&] { water.prepare_frame(camera, settings, {}); });
        retained();
        rejects([&] {
            water.prepare_frame(camera, settings, [](float) -> float {
                throw std::invalid_argument("Unavailable device estimate");
            });
        });
        retained();
        rejects([&] { water.prepare(too_many); });
        retained();
        WaterSurfaceFrame surface_frame{};
        surface_frame.fog_parameters[0] = .02f;
        WaterSurfaceView surface_view{};
        surface_view.shadow.fill(std::numeric_limits<float>::quiet_NaN());
        surface_view.reflection = surface_view.shadow;
        for (unsigned index = 0; index < 2; ++index) {
            const auto constants = water.constants(index, {}, surface_frame, surface_view);
            check(std::memcmp(constants.data() + 88, retained_frame.matrices.reflection[index].data(), 64) ==
                          0 &&
                      std::memcmp(constants.data() + 104, retained_frame.matrices.shadow[index].data(), 64) ==
                          0,
                  "Ocean constants did not use their own camera view's matrices");
            ++guards;
        }
        rejects([&] { water.constants(2, {}, surface_frame, surface_view); });
        camera.view_count = 0;
        water.prepare_frame(camera, settings, estimate);
        check(water.mesh().vertices.empty() && water.draws().empty() &&
                  std::memcmp(&water.frame_state().bounds, &retained_frame.bounds,
                              sizeof(retained_frame.bounds)) == 0,
              "Zero-view camera failed to retain the original inactive bounds");
        ++guards;
        rejects([&] { water.constants(0, {}, surface_frame, surface_view); });
        camera.view_count = 2;
        water.prepare_frame(camera, settings, estimate);
        check(!water.draws().empty(), "Camera frame could not recover from zero views");
        ++guards;
        water.prepare(views);
        rejects([&] { water.frame_state(); });
        rejects([&] { water.constants(0, {}, surface_frame, surface_view); });
        rejects([&] { water.camera_state(); });
        PerspectiveCamera perspective{
            45,
            4.f / 3,
            1,
            10000,
            {1, 0, 0, 0, 0, .70710677f, .70710677f, 0, 0, -.70710677f, .70710677f, 0, 0, -2200, 2200, 1}};
        const auto reciprocal = [](float determinant) {
            return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(determinant)));
        };
        water.prepare_camera_frame(perspective, settings, reciprocal, estimate);
        check(water.camera_state().view_count >= 1 && water.camera_state().view_count <= 3 &&
                  water.mesh().counts.size() == water.camera_state().view_count * 256 &&
                  !water.draws().empty(),
              "Original perspective camera and prepared ocean view counts disagree");
        ++guards;
        const auto *perspective_state = &water.camera_state();
        const auto *perspective_frame = &water.frame_state();
        const auto *perspective_mesh = &water.mesh();
        const auto perspective_copy = *perspective_state;
        const auto perspective_frame_copy = *perspective_frame;
        const auto retain_perspective = [&] {
            check(&water.camera_state() == perspective_state && &water.frame_state() == perspective_frame &&
                      &water.mesh() == perspective_mesh &&
                      std::memcmp(perspective_state, &perspective_copy, sizeof(perspective_copy)) == 0 &&
                      std::memcmp(perspective_frame, &perspective_frame_copy,
                                  sizeof(perspective_frame_copy)) == 0,
                  "Failed perspective update changed owned camera/frame/geometry");
            ++guards;
        };
        auto bad_perspective = perspective;
        bad_perspective.field_of_view = 0;
        rejects([&] { water.prepare_camera_frame(bad_perspective, settings, reciprocal, estimate); });
        retain_perspective();
        bad_perspective = perspective;
        bad_perspective.world.fill(0);
        rejects([&] { water.prepare_camera_frame(bad_perspective, settings, reciprocal, estimate); });
        retain_perspective();
        rejects([&] { water.prepare_camera_frame(perspective, settings, {}, estimate); });
        retain_perspective();
        rejects([&] { water.prepare_camera_frame(perspective, settings, reciprocal, {}); });
        retain_perspective();
        rejects([&] {
            water.prepare_camera_frame(
                perspective, settings,
                [](float) -> float { throw std::invalid_argument("Unavailable camera estimate"); }, estimate);
        });
        retain_perspective();
        rejects([&] { water.prepare_frame(invalid_camera, settings, estimate); });
        retain_perspective();
        CameraControllerState entering_controller{};
        entering_controller.wrap_orientation = 1;
        const auto motion_controller =
            initialize_camera(terrain->source().layout, terrain->source().map_mode, {1280, 960, 11},
                              entering_controller, X87Precision::double_precision);
        rejects([&] {
            water.prepare_camera_motion(motion_controller, {-1.f, 2, false, false, false, 0}, settings,
                                        reciprocal, estimate);
        });
        retain_perspective();
        unsigned motion_inverse_calls = 0;
        rejects([&] {
            water.prepare_camera_motion(
                motion_controller, {1.f / 60, 2, false, false, false, 0}, settings,
                [&](float det) {
                    ++motion_inverse_calls;
                    return reciprocal(det);
                },
                [](float) -> float {
                    throw std::invalid_argument("Unavailable motion reflection estimate");
                });
        });
        check(motion_inverse_calls >= 4, "Motion rejection must follow its callback and camera frame math");
        ++guards;
        retain_perspective();
        const CameraInputSettings pointer_settings{{1, 0}, false, true};
        const CameraPointerState pointer_state{1280, 960};
        auto bad_pointer = pointer_state;
        bad_pointer.width = 0;
        rejects([&] {
            water.prepare_camera_input(motion_controller, bad_pointer, {1.f / 60, 0, 0, pointer_settings, {}},
                                       settings, reciprocal, estimate);
        });
        retain_perspective();
        auto bad_pointer_settings = pointer_settings;
        bad_pointer_settings.drag.sensitivity = std::numeric_limits<float>::quiet_NaN();
        rejects([&] {
            water.prepare_camera_input(motion_controller, pointer_state,
                                       {1.f / 60, 0, 0, bad_pointer_settings, {}}, settings, reciprocal,
                                       estimate);
        });
        retain_perspective();
        const std::array<CameraInputEvent, 3> pointer_events{
            CameraPointerEvent{CameraPointerEventKind::move, 640, 480, 0},
            CameraPointerEvent{CameraPointerEventKind::left_down, 640, 480, 1},
            CameraPointerEvent{CameraPointerEventKind::move, 700, 500, 1}};
        unsigned input_estimates = 0;
        rejects([&] {
            water.prepare_camera_input(
                motion_controller, pointer_state, {1.f / 60, 0, 0, pointer_settings, pointer_events},
                settings,
                [&](float value) {
                    if (++input_estimates == 6)
                        throw std::invalid_argument("Unavailable drag intersection estimate");
                    return reciprocal(value);
                },
                estimate);
        });
        check(input_estimates == 6, "Input rejection must occur after camera rays and capture production");
        ++guards;
        retain_perspective();
        input_estimates = 0;
        rejects([&] {
            water.prepare_camera_input(
                motion_controller, pointer_state, {1.f / 60, 0, 0, pointer_settings, pointer_events},
                settings,
                [&](float value) {
                    ++input_estimates;
                    return reciprocal(value);
                },
                [](float) -> float { throw std::invalid_argument("Unavailable input reflection estimate"); });
        });
        check(input_estimates >= 15, "Input rejection must follow pointer, drag, controller and frame math");
        ++guards;
        retain_perspective();
        surface_frame.projection.fill(std::numeric_limits<float>::quiet_NaN());
        surface_view.view = surface_frame.projection;
        surface_view.camera.fill(std::numeric_limits<float>::quiet_NaN());
        for (unsigned index = 0; index < perspective_copy.view_count; ++index) {
            const auto c = water.constants(index, {}, surface_frame, surface_view);
            const auto vp = matrix_multiply(perspective_copy.views[index], perspective_copy.projection,
                                            X87Precision::double_precision, MatrixPath::sse);
            check(std::memcmp(c.data(), vp.data(), 64) == 0 &&
                      std::memcmp(c.data() + 84, perspective_copy.inverse_views[index].data() + 12, 12) == 0,
                  "Owned camera projection/view/position did not reach ocean constants");
            ++guards;
        }
        rejects([&] { water.constants(perspective_copy.view_count, {}, surface_frame, surface_view); });
        perspective.world[12] = 125;
        water.prepare_camera_frame(perspective, settings, reciprocal, estimate);
        check(std::memcmp(&water.camera_state(), &perspective_copy, sizeof(perspective_copy)) != 0 &&
                  !water.draws().empty(),
              "Perspective camera failed to advance after rejected updates");
        ++guards;
        water.prepare_frame(camera, settings, estimate);
        rejects([&] { water.camera_state(); });
        water.prepare_camera_frame(perspective, settings, reciprocal, estimate);
        water.prepare(views);
        rejects([&] { water.camera_state(); });
        rejects([&] { water.frame_state(); });
    }
    check(terrain.use_count() == owners, "Water world leaked its terrain owner");
    ++guards;
    context->check("Water world source ownership and transactional rebuild checks");
    return guards;
}
template <class LandRenderer>
void render_water_scene(std::shared_ptr<GlContext> context, std::shared_ptr<const MaterialWorld> world,
                        LandRenderer &land, NativeWorld &original, Control &controls,
                        GlLandMaterialInputs land_scene, const LandMaterialConstants &camera_controls,
                        const fs::path &root, const fs::path &shaders, const fs::path &folder,
                        bool authored_sky, bool constructor_defaults, bool animate,
                        const fs::path &current_shaders = {}, const fs::path &current_fixture = {},
                        const fs::path &reflection_fixture = {}, bool computed_frame = false,
                        bool computed_camera = false, bool startup_camera = false, bool motion_camera = false,
                        bool input_camera = false, bool original_lighting = false,
                        const GlDistanceFog *distance_fog = nullptr, const fs::path &fow_shaders = {},
                        const fs::path &fow_reference = {}, const fs::path &fow_land_probes = {},
                        const fs::path &fow_animation_reference = {}, const GlColorKeys *color_keys = nullptr,
                        const fs::path &fow_camera_reference = {}, const fs::path &river_shaders = {},
                        const fs::path &river_reference = {}, const fs::path &sequence_reference = {}) {
    const auto waves = civ5::assets::LeanData::parse(
        water_asset(root, "Resource/Common/TerrainModels.fpk", "waterbumps16.lean"));
    const auto color = civ5::assets::DdsData::parse(
        water_asset(root, "Resource/DX9/Upgrade1Textures.fpk", "waterdepthcolor_new.dds"));
    const std::shared_ptr<const TerrainWorld> terrain(world, &world->terrain());
    GlWaterWorldRenderer water(context, terrain, waves, color, 3, X87Precision::double_precision);
    const auto &d = water.domain();
    auto camera = camera_controls;
    float normalized_zoom = 0;
    std::vector<LandMaterialConstants> land_cameras;
    const std::array<Vector4, 1> bounds{
        {{d.origin_x, d.origin_y, d.origin_x + d.span_x, d.origin_y + d.span_y}}};
    if (computed_camera) {
        check(computed_frame, "Perspective camera requires the connected water frame");
        PerspectiveCamera perspective{
            45,
            4.f / 3,
            1,
            10000,
            {1, 0, 0, 0, 0, .70710677f, .70710677f, 0, 0, -.70710677f, .70710677f, 0, 0, -2200, 2200, 1}};
        WaterReflectionFrameSettings frame_settings{
            {23.75f, 61.125f, 117.5f, 38.25f}, {.3f, -.7f, .25f}, {d.scales[0], d.scales[1]}};
        if (original_lighting) {
            const auto lighting = terrain_lighting_defaults(X87Precision::double_precision);
            frame_settings.angles = lighting.angles;
            frame_settings.shadow_direction = lighting.shadow;
        }
        std::vector<float> reciprocal, reciprocal_sqrt;
        const auto rcp = [&](float determinant) {
            const auto seed = _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(determinant)));
            reciprocal.push_back(determinant);
            reciprocal.push_back(seed);
            return seed;
        };
        const auto rsqrt = [&](float squared) {
            const auto seed = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
            reciprocal_sqrt.push_back(squared);
            reciprocal_sqrt.push_back(seed);
            return seed;
        };
        const auto output = folder / "camera-frame";
        fs::create_directory(output);
        if (startup_camera) {
            const CameraStartupSettings startup{1280, 960, 11};
            camera_controller_storage::Bytes prior{};
            constexpr std::uint32_t original_vtable = 0xb712a8;
            std::memcpy(prior.data() + 4, &original_vtable, 4);
            if (motion_camera || input_camera) {
                constexpr std::int32_t orientation = 1;
                std::memcpy(prior.data() + 0x10c, &orientation, 4);
            }
            auto controller = water.prepare_camera_startup(startup, camera_controller_storage::prior(prior),
                                                           frame_settings, rcp, rsqrt);
            if (motion_camera) {
                save(output / "initialized-controller.bin",
                     camera_controller_storage::encode(controller, prior));
                water_validation::save(
                    output / "initialized-state.bin",
                    std::array{WaterCameraFrameState{water.camera_state(), water.frame_state()}});
                std::vector<CameraMotionInput> steps;
                for (unsigned i = 0; i < 250; ++i)
                    steps.push_back({1.f / 60, 2, false, false, false, 0});
                for (unsigned i = 0; i < 12; ++i)
                    steps.push_back({1.f / 60, 16, false, false, false, 0});
                for (unsigned i = 0; i < 30; ++i)
                    steps.push_back({1.f / 60, 6, true, true, true, 0});
                steps.push_back({1.f / 60, 0, true, true, false, 0});
                for (unsigned i = 0; i < 30; ++i)
                    steps.push_back({1.f / 60, 0, true, false, false, 0});
                Writer commands, events;
                commands.bytes = {'C', '5', 'M', 'O', 'T', '0', '0', '1'};
                commands.word(static_cast<std::uint32_t>(steps.size()));
                std::vector<camera_controller_storage::Bytes> controllers;
                std::vector<WaterCameraFrameState> frames;
                std::vector<Vector4> zoom;
                unsigned gesture_index = 0;
                for (const auto &step : steps) {
                    commands.word(std::bit_cast<std::uint32_t>(step.delta_seconds));
                    for (auto v :
                         {std::uint32_t(step.commands), std::uint32_t(step.sample_inertia),
                          std::uint32_t(step.inertia_enabled), std::uint32_t(step.dragging), step.input_mode})
                        commands.word(v);
                    commands.word(static_cast<std::uint32_t>(step.dragging));
                    if (step.dragging) {
                        const float i = static_cast<float>(gesture_index++);
                        const Vector3 position{-650.f + 3.f * i, -400.f + 2.f * i, 430.f + .25f * i};
                        for (float f : position)
                            commands.word(std::bit_cast<std::uint32_t>(f));
                        controller =
                            set_camera_position(terrain->source().layout, terrain->source().map_mode,
                                                controller, position, X87Precision::double_precision);
                    }
                    const auto moved =
                        water.prepare_camera_motion(controller, step, frame_settings, rcp, rsqrt);
                    scene_land_camera_events(land, moved.events);
                    controller = moved.controller;
                    controllers.push_back(camera_controller_storage::encode(controller, prior));
                    frames.push_back({water.camera_state(), water.frame_state()});
                    zoom.push_back(moved.zoom);
                    events.word(static_cast<std::uint32_t>(moved.events.size()));
                    for (const auto &event : moved.events) {
                        events.word(static_cast<std::uint32_t>(event.kind));
                        if (event.kind == CameraMotionEventKind::wrapped)
                            events.word(event.wrap_direction);
                        if (event.kind == CameraMotionEventKind::camera_changed) {
                            for (float f : event.view)
                                events.word(std::bit_cast<std::uint32_t>(f));
                            for (float f : event.projection)
                                events.word(std::bit_cast<std::uint32_t>(f));
                        }
                    }
                }
                save(output / "motion-inputs.bin", commands.bytes);
                save(output / "motion-events.bin", events.bytes);
                water_validation::save(output / "motion-controllers.bin", controllers);
                water_validation::save(output / "motion-frames.bin", frames);
                water_validation::save(output / "motion-zoom.bin", zoom);
            }
            if (input_camera) {
                save(output / "initialized-controller.bin",
                     camera_controller_storage::encode(controller, prior));
                water_validation::save(
                    output / "initialized-state.bin",
                    std::array{WaterCameraFrameState{water.camera_state(), water.frame_state()}});
                controller = run_camera_input_scene(
                    water, controller, prior, frame_settings, rcp, rsqrt, output, &normalized_zoom, {},
                    [&](auto events) { scene_land_camera_events(land, events); });
            }
            perspective = controller.camera;
            water_validation::save(output / "startup.bin", std::array{startup});
            save(output / "prior-controller.bin", prior);
            save(output / "controller.bin", camera_controller_storage::encode(controller, prior));
            water_validation::save(output / "tilt.f32", std::array{controller.tilt_degrees});
        } else {
            water.prepare_camera_frame(perspective, frame_settings, rcp, rsqrt);
        }
        const auto &state = water.camera_state();
        std::cout << "Perspective camera views=" << state.view_count
                  << "; mode=" << terrain->source().map_mode
                  << "; source=" << terrain->source().layout.source_grid.width << 'x'
                  << terrain->source().layout.source_grid.height << '\n'
                  << std::flush;
        check(motion_camera || input_camera ? state.view_count >= 1 && state.view_count <= 3
                                            : state.view_count == (startup_camera ? 1u : 3u),
              "Connected perspective scene view count");
        Writer camera_inputs;
        for (unsigned view = 0; view < state.view_count; ++view) {
            auto land_camera = camera_controls;
            const auto view_projection = matrix_multiply(state.views[view], state.projection,
                                                         X87Precision::double_precision, MatrixPath::sse);
            for (unsigned row = 0; row < 4; ++row)
                std::copy_n(view_projection.begin() + row * 4, 4, land_camera.view_projection[row].begin());
            std::copy_n(state.inverse_views[view].begin() + 12, 3, land_camera.camera.begin());
            if (distance_fog)
                land_camera = distance_fog->land_constants(land_camera, normalized_zoom,
                                                           X87Precision::double_precision);
            const auto bytes = frame_bytes(land_camera);
            camera_inputs.bytes.insert(camera_inputs.bytes.end(), bytes.begin(), bytes.end());
            land_cameras.push_back(land_camera);
        }
        camera = land_cameras[0];
        water_validation::save(output / "camera.bin", std::array{perspective});
        water_validation::save(output / "settings.bin", std::array{frame_settings});
        water_validation::save(output / "rcp.bin", reciprocal);
        water_validation::save(output / "rsqrt.bin", reciprocal_sqrt);
        water_validation::save(output / "state.bin",
                               std::array{WaterCameraFrameState{state, water.frame_state()}});
        save(folder / "water-scene-land-camera.bin", camera_inputs.bytes);
    } else if (computed_frame) {
        const civ5::terrain::Matrix4 identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        // Explicit side planes of overview_frame: x in [-1408,1408] and
        // y+z in [-1024,1024]. Plane extraction/configuration is not claimed.
        WaterReflectionCamera reflection_camera{{}, 1, identity, {identity, identity, identity}};
        reflection_camera.frusta[0] = {Vector4{0, 1, -1, 2048}, Vector4{0, -1, 1, 2048},
                                       Vector4{1, 0, 0, 1408},  Vector4{-1, 0, 0, 1408},
                                       Vector4{0, 1, 1, 1024},  Vector4{0, -1, -1, 1024}};
        const WaterReflectionFrameSettings frame_settings{
            {23.75f, 61.125f, 117.5f, 38.25f}, {.3f, -.7f, .25f}, {d.scales[0], d.scales[1]}};
        std::vector<float> estimates;
        water.prepare_frame(reflection_camera, frame_settings, [&](float squared) {
            const auto seed = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
            estimates.push_back(squared);
            estimates.push_back(seed);
            return seed;
        });
        static_assert(sizeof(reflection_camera) == 644 && sizeof(frame_settings) == 36 &&
                      sizeof(WaterReflectionFrameState) == 1176);
        water_validation::save(folder / "water-frame-camera.bin", std::array{reflection_camera});
        water_validation::save(folder / "water-frame-settings.bin", std::array{frame_settings});
        water_validation::save(folder / "water-frame-rsqrt.bin", estimates);
        water_validation::save(folder / "water-frame-state.bin", std::array{water.frame_state()});
    } else {
        water.prepare(bounds);
    }
    if (land_cameras.empty())
        land_cameras.push_back(camera);
    const auto view_count = static_cast<unsigned>(land_cameras.size());
    std::cout << "PASS water scene source geometry\n" << std::flush;
    water_validation::NativeWater native(shaders, original.cache.device.value);
    auto rgba = native.lean(waves, civ5::assets::LeanMap::rgba16);
    auto luminance = native.lean(waves, civ5::assets::LeanMap::luminance16);
    auto native_color = native.upload(color, D3DFMT_A8R8G8B8);
    std::cout << "PASS water scene native source uploads\n" << std::flush;
    std::array<IDirect3DBaseTexture9 *, 14> images{};
    images[0] = rgba.value;
    images[1] = luminance.value;
    images[6] = native_color.value;
    GlWaterSurfaceInputs scene;
    std::vector<land_material_validation::Com<IDirect3DBaseTexture9>> native_controls;
    // Neutral controls are explicit, and are NOT presented as current game
    // FOW/overlay/reflection/WaterFogRamp or color-grading producers.
    for (unsigned slot : {4u, 5u, 8u, 9u, 10u, 11u, 13u}) {
        if (slot == 8 && distance_fog)
            continue;
        if (slot == 5 && !current_shaders.empty())
            continue;
        if (slot == 4 && !reflection_fixture.empty())
            continue;
        FloatImage f = slot == 11 ? identity_volume() : FloatImage{1, false, {0, 0, 0, 0}};
        if (slot == 5)
            f.values = {.5f, .5f, 0, 1};
        if (slot == 10)
            f.values = {1, 0, 1, 1};
        scene.pixel[slot] = controls.image(f);
        native_controls.push_back(original.land.image(f));
        images[slot] = native_controls.back().value;
        blend_validation::write(folder / ("water-scene-control" + std::to_string(slot) + ".f32"), f.values);
    }
    if (distance_fog) {
        check(input_camera && original.distance_fog &&
                  original.distance_fog->land.size() == land_cameras.size(),
              "Fog scene requires independent original camera/LAND/water state");
        scene.pixel[8] = distance_fog->ramp().id();
        images[8] = original.distance_fog->ramp.value;
        const auto global = distance_fog->frame(normalized_zoom);
        check(std::memcmp(&global, &original.distance_fog->global, 32) == 0,
              "Original fog camera globals differ");
        water_validation::save(folder / "distance-fog-global.bin", std::array{global});
        for (unsigned i = 0; i < land_cameras.size(); ++i)
            check(land_cameras[i].fog_color == original.distance_fog->land[i][0] &&
                      land_cameras[i].fog == original.distance_fog->land[i][1],
                  "Original LAND fog constants differ");
    }
    std::unique_ptr<WaterCurrentScene> currents;
    if (!current_shaders.empty()) {
        water.prepare_current();
        currents = std::make_unique<WaterCurrentScene>(context, water.current_map(), native.device.value,
                                                       current_shaders, current_fixture, folder);
        // Slot 5 is intentionally empty here; GlWaterWorldRenderer must supply
        // the owned target at every ocean draw. The native API keeps its own.
        scene.pixel[5] = 0;
        images[5] = currents->native.maps[1].value;
        std::cout << "PASS water scene owned current map\n" << std::flush;
    }
    std::unique_ptr<WaterReflectionScene> reflections;
    if (!reflection_fixture.empty()) {
        reflections = std::make_unique<WaterReflectionScene>(context, water, native.device.value,
                                                             reflection_fixture, folder);
        scene.pixel[4] = 0; // Production owner supplies the real target at every draw.
        images[4] = reflections->map.value;
        std::cout << "PASS water scene owned reflection map\n" << std::flush;
    }
    WaterSurfaceFrame frame{};
    for (unsigned row = 0; row < 4; ++row)
        std::copy(camera.view_projection[row].begin(), camera.view_projection[row].end(),
                  frame.projection.begin() + row * 4);
    frame.fog_parameters = {.002f, 0, 0, 0};
    if (distance_fog) {
        const auto global = distance_fog->frame(normalized_zoom);
        frame.fog_color = global.color;
        frame.fog_parameters = global.parameters;
    }
    WaterSurfaceView view{};
    view.view = view.overlay = view.shadow = view.reflection =
        civ5::terrain::Matrix4{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    std::copy_n(camera.camera.begin(), 3, view.camera.begin());
    // Original constructor receives ocean U16 13000. Lighting and phase here
    // are controlled inputs; WaterSky source decoding is a separate boundary.
    WaterSurfaceSettings settings{0,
                                  {1.2f, 1.1f, .9f, 32},
                                  {.4f, .5f, .7f, 16},
                                  {.3f, -.4f, .8660254f},
                                  {-.2f, .7f, .68556546f},
                                  {.3f, -.4f, .8660254f, .4f},
                                  {-.2f, .7f, .68556546f, .3f},
                                  13000.f * std::bit_cast<float>(0x37800080u),
                                  waves.gradient_scale(),
                                  .008f,
                                  .17f,
                                  .09f};
    if (authored_sky) {
        const auto sky_bytes = water_asset(root, "Resource/DX9/TerrainTextures.fpk", "watersky.dds");
        const auto pixels = water_sky_pixels(civ5::assets::DdsData::parse(sky_bytes));
        std::vector<float> estimates;
        const auto lighting = water_sky_lighting(pixels, [&](float squared) {
            const auto seed = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
            estimates.push_back(squared);
            estimates.push_back(seed);
            return seed;
        });
        settings =
            constructor_defaults ? initial_water_surface_settings(13000, lighting) : lighting.apply(settings);
        save(folder / "water-scene-sky.dds", sky_bytes);
        water_validation::save(folder / "water-scene-sky-pixels.bin", pixels);
        water_validation::save(folder / "water-scene-sky-rsqrt.bin", estimates);
    }
    std::vector<WaterConstantBlock> constants;
    for (unsigned index = 0; index < view_count; ++index)
        constants.push_back(computed_frame ? water.constants(index, settings, frame, view)
                                           : water.constants(settings, frame, view));
    water_validation::save(folder / "water-scene-constants.bin", constants);
    if (distance_fog)
        check(constants == original.distance_fog->water, "Original complete water fog constants differ");
    water_validation::save(folder / "water-scene-vertices.bin", water.mesh().vertices);
    water_validation::save(folder / "water-scene-indices.bin", water.mesh().indices);
    water_validation::save(folder / "water-scene-counts.bin", water.mesh().counts);
    water_validation::save(folder / "water-scene-prefixes.bin", water.mesh().prefixes);
    Writer metadata;
    unsigned height_windows = 0;
    for (const auto &draw : water.draws()) {
        metadata.word(draw.view);
        metadata.word(draw.window);
        metadata.word(draw.first_index);
        metadata.word(draw.triangle_count);
        const auto p = world->terrain().land().patch(int(draw.window % 16) * 14, int(draw.window / 16) * 14);
        check(
            water.height_texture(draw.window).read_mip_raw(0) ==
                land.height_texture(std::size_t(p.column()) * world->cache().rows + p.row()).read_mip_raw(0),
            "Ocean and LAND must consume identical world height windows");
        ++height_windows;
    }
    save(folder / "water-scene-draws.bin", metadata.bytes);
    scene_land_prepare(land, water.camera_state(), folder / "scheduled-materials");
    SceneTarget target(context, 1280, 960);
    NativeSceneTarget native_target(original, 1280, 960);
    target.begin();
    // Exercise both native mapping and restoration from a different incoming
    // clip origin. Viewport 1 is unrelated to these shaders and must survive.
    GLint prior_origin = 0, prior_depth = 0;
    std::array<float, 4> prior_viewport1{};
    context->api().GetIntegerv(GL_CLIP_ORIGIN, &prior_origin);
    context->api().GetIntegerv(GL_CLIP_DEPTH_MODE, &prior_depth);
    target.gl.GetFloati_v(GL_VIEWPORT, 1, prior_viewport1.data());
    target.gl.ClipControl(GL_UPPER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
    target.gl.ViewportIndexedf(1, .125f, .25f, 17, 19);
    unsigned raster_checks = 0;
    const auto check_raster_state = [&] {
        GLint origin = 0, depth = 0;
        std::array<float, 4> viewport0{}, viewport1{};
        context->api().GetIntegerv(GL_CLIP_ORIGIN, &origin);
        context->api().GetIntegerv(GL_CLIP_DEPTH_MODE, &depth);
        target.gl.GetFloati_v(GL_VIEWPORT, 0, viewport0.data());
        target.gl.GetFloati_v(GL_VIEWPORT, 1, viewport1.data());
        check(origin == GL_UPPER_LEFT && depth == GL_NEGATIVE_ONE_TO_ONE,
              "Native scene draw leaked clip mode");
        check(viewport0 == std::array<float, 4>{0, 0, 1280, 960},
              "Native scene draw leaked pixel-center viewport");
        check(viewport1 == std::array<float, 4>{.125f, .25f, 17, 19},
              "Native scene draw changed another viewport");
        raster_checks += 3;
    };
    for (unsigned camera_index = 0; camera_index < land_cameras.size(); ++camera_index)
        for (auto index : scene_land_indices(land, *world, camera_index))
            land.draw(index, land_cameras[camera_index], land_scene, 1280, 960);
    check_raster_state();
    blend_validation::write(folder / "water-scene-land-gl.f32", target.pixels());
    // Same depth surface as LAND, ordinary opaque write and LEQUAL depth.
    // Original packet consumer's complete water raster policy is not claimed.
    for (unsigned index = 0; index < view_count; ++index)
        water.draw(index, constants[index], scene, 1280, 960);
    check_raster_state();
    target.gl.ViewportIndexedf(1, prior_viewport1[0], prior_viewport1[1], prior_viewport1[2],
                               prior_viewport1[3]);
    target.gl.ClipControl(static_cast<GLenum>(prior_origin), static_cast<GLenum>(prior_depth));
    blend_validation::text(folder / "water-scene-raster-state.txt",
                           "PASS native LAND/water clip and viewport restoration checks=" +
                               std::to_string(raster_checks) + "\n");
    blend_validation::write(folder / "water-scene-gl.f32", target.pixels());
    native_target.begin();
    for (unsigned camera_index = 0; camera_index < land_cameras.size(); ++camera_index)
        for (auto index : scene_land_indices(land, *world, camera_index)) {
            const auto p =
                world->terrain().land().patch(index / world->cache().rows, index % world->cache().rows);
            const LandGrid grid(p.source_samples_x(), p.source_samples_y(), 1);
            const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(), grid.indices());
            auto native_camera = land_cameras[camera_index];
            if (distance_fog) {
                native_camera.fog_color = original.distance_fog->land[camera_index][0];
                native_camera.fog = original.distance_fog->land[camera_index][1];
            }
            original.land.bind(mesh, 1, land.land_constants(index, native_camera),
                               scene_land_images(land, original, index),
                               original.heights.at(p.texture_table_slot()).value);
            original.land.submit(mesh);
        }
    blend_validation::write(folder / "water-scene-land-d3d9.f32", native_target.pixels());
    hr(native.device->BeginScene(), "Complete world native ocean begin");
    for (const auto &draw : water.draws()) {
        images[7] = original.heights.at(draw.window).value;
        native.bind(3, distance_fog ? original.distance_fog->water.at(draw.view) : constants.at(draw.view),
                    images);
        native.draw(water.mesh(), draw.first_index, draw.triangle_count);
    }
    hr(native.device->EndScene(), "Complete world native ocean end");
    blend_validation::write(folder / "water-scene-d3d9.f32", native_target.pixels());
    scene_land_finish(land);
    std::ostringstream report;
    report << "Original shader selector=3; owned height windows=" << height_windows
           << "; ocean quads=" << water.mesh().vertices.size() / 4 << '\n'
           << "Actual world height + source LEAN/WaterColor; controlled "
           << (startup_camera ? "LUT/FOW/reflection.\n" : "camera/LUT/FOW/reflection.\n")
           << "Original complete water raster-state producer not executed; "
              "explicit LEQUAL and opaque writes.\n"
           << "No native rendered pixels used as production input. Full graphics "
              "parity remains false.\n";
    if (authored_sky)
        report << "Water light colors/directions/fake views from authored "
                  "WaterSky; host RSQRT estimates.\n";
    if (authored_sky && !constructor_defaults)
        report << "Specular powers and wave phases remain explicit controls.\n";
    if (constructor_defaults)
        report << "Original constructor surface defaults: gradient=5, "
                  "powers=2^11.75/2^5.75, "
                  "reflection amount=0.5, frequency=0.008, initial phases=0.\n";
    if (currents)
        report << "Current-map input is each API's own key5d output; only its "
                  "raster policy remains "
                  "controlled.\n";
    if (reflections)
        report << "Reflection slot4 uses each API's own original target/clear "
                  "pass; object scene is empty.\n";
    if (distance_fog)
        report << "DistanceFog CSV, eight authored ramp mips and normal-camera "
                  "zoom reach LAND/water; "
                  "D3D9 receives independently executed original fog and complete "
                  "water constants.\n";
    if (computed_camera) {
        report << "Owned original perspective camera produces projection, frusta "
                  "and views, then water "
                  "bounds/directions/reflection matrices.\n"
                  "LAND and water share its produced view-projection and "
                  "inverse-view camera position; ";
        if (input_camera) {
            report << "lens/pose come from original startup followed by 323 "
                      "pointer/wheel/controller ticks. "
                      "Pixel drags, wheel messages, inertia, pan, yaw and map "
                      "wrapping produce this final "
                      "camera; "
                      "the later water animation holds it fixed. UI routing, "
                      "retained wrap orientation, drag "
                      "settings";
            report << (original_lighting ? " remain explicit. Shadow direction and "
                                           "sun/reflection angles come from the "
                                           "original lighting startup.\n"
                                         : ", shadow direction and lighting "
                                           "configuration remain explicit.\n");
        } else if (motion_camera)
            report << "lens/pose come from original startup followed by 323 normal "
                      "camera ticks. "
                      "Pan, yaw, inertia and map wrapping produce this final camera; "
                      "the later water "
                      "animation holds it fixed. "
                      "Input signals, retained wrap orientation, shadow direction "
                      "and lighting configuration "
                      "remain explicit.\n";
        else if (startup_camera)
            report << "lens/pose come from original startup using screen/world "
                      "settings. "
                      "Shadow direction and lighting configuration remain explicit; "
                      "camera motion is not "
                      "executed.\n";
        else
            report << "lens/pose, shadow direction and lighting configuration are "
                      "explicit.\n";
    } else if (computed_frame)
        report << "Owned frame computes bounds, directions and reflection/shadow "
                  "matrices from explicit "
                  "overview planes/angles; host RSQRT estimates.\n";
    blend_validation::text(folder / "water-scene-scope.txt", report.str());
    std::cout << "PASS connected LAND and water scene\n" << std::flush;
    if (animate)
        render_water_animation(context, water, native, target, native_target, settings, frame, view, scene,
                               images, original, folder, bool(currents), computed_frame, view_count);
    if (!fow_reference.empty()) {
        check(distance_fog && input_camera, "FOW scene requires the original input camera and fog");
        check(color_keys && original.color_keys, "FOW water requires independently loaded ColorKeys");
        // Original 5CDD05 reads D80CB0 = ColorKey owner D80C38 + 78:
        // TerrainFOW, Generic. Packet +2E is water pixel sampler 11.
        scene.pixel[11] = color_keys->at(ColorKeySystem::terrain_fow, 0).id();
        images[11] = original.color_keys->at(1, 0);
        save(folder / "water-fow-color-key.bgra",
             color_keys->at(ColorKeySystem::terrain_fow, 0).read_mip_raw(0));
        std::unique_ptr<RiverWorldScene> rivers;
        if (!river_shaders.empty()) {
            rivers = std::make_unique<RiverWorldScene>(
                context, terrain, root, river_shaders, river_reference, folder / "rivers",
                original.cache.device.value, *color_keys, *original.color_keys,
                water.reflection_map().texture().id(), scene.pixel[9], images);
        }
        render_fow_world_scene(
            context, world, land, original, water, native, settings, frame, view, land_scene, scene, images,
            *distance_fog, normalized_zoom, root, fow_shaders, fow_reference, folder / "fow-scene",
            fow_animation_reference.empty() && sequence_reference.empty() ? fow_land_probes : fs::path{},
            nullptr, {}, rivers.get());
        if (!fow_animation_reference.empty())
            render_fow_world_animation(context, world, land, original, water, native, settings, frame, view,
                                       land_scene, scene, images, *distance_fog, normalized_zoom, root,
                                       fow_shaders, fow_reference, fow_animation_reference,
                                       folder / "fow-animation", fow_land_probes, rivers.get());
        if (!fow_camera_reference.empty()) {
            // The original input corpus starts with zero frame storage. Reusing
            // the completed camera would retain inactive view matrices from it.
            GlWaterWorldRenderer camera_water(context, terrain, waves, color, 3,
                                              X87Precision::double_precision);
            auto camera_images = scene;
            camera_images.pixel[4] = water.reflection_map().texture().id();
            camera_images.pixel[5] = water.current_map().texture().id();
            render_fow_world_camera(context, world, land, original, camera_water, native, settings, frame,
                                    view, land_scene, camera_images, images, *distance_fog, root, fow_shaders,
                                    fow_camera_reference, folder / "fow-camera", rivers.get());
        }
        if (!sequence_reference.empty()) {
            check(bool(rivers), "Combined sequence requires the connected river surface");
            GlWaterWorldRenderer sequence_water(context, terrain, waves, color, 3,
                                                X87Precision::double_precision);
            auto sequence_images = scene;
            sequence_images.pixel[4] = water.reflection_map().texture().id();
            sequence_images.pixel[5] = water.current_map().texture().id();
            render_fow_scene_sequence(context, world, land, original, sequence_water, native, settings, frame,
                                      view, land_scene, sequence_images, images, *distance_fog, root,
                                      fow_shaders, sequence_reference, folder / "fow-sequence", *rivers,
                                      fow_land_probes);
        }
    }
}
} // namespace
