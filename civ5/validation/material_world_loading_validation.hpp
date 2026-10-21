#pragma once

// Actual source-world workers and GPU leases, with original D3D9 shaders for
// every generated cache. No expected cache data enter the renderer.
namespace {
void validate_material_world_loading(
    std::shared_ptr<GlContext> context, const fs::path &root, PrimitiveInput &input,
    const std::array<fs::path, 4> &shaders, const fs::path &land_shaders,
    const LightingCubeAssets &cubes, std::span<const std::uint8_t> geometry,
    const GlMaterialWorldLighting &light, GlLandMaterialInputs scene, const fs::path &out) {
    const auto world = build_world(root, input, 0);
    const auto count = world->cache().patches.size();
    check(count == 1120, "Loading validation requires the complete established world");
    GlMaterialWorldRenderer renderer(context, world, cubes, {false, true, true},
        LightingCubeSampling::measured_coordinates, LandShadowArithmetic::measured_fma_t_x_z_y,
        LandMipSampling::d3d9_trilinear);
    const auto field = world->terrain().height_field();
    NativeWorld native(shaders, land_shaders, world->assets(), cubes, 64, geometry,
                       field.extract_a8_texture(0, 0), renderer.source_bindings());
    mip_validation::NativeMipReader reader(native.cache.device.value);
    const MaterialNormalSeed seed = [](int, int, float squared) {
        return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
    };
    const MaterialPatchSettings patch_settings{64, 0, 0, 0, X87Precision::double_precision};
    const std::array<MaterialCacheSchedulerView, 1> views{{{identity_matrix(), {}}}};
    MaterialCacheSchedulerSettings settings{};
    settings.projection = identity_matrix();
    settings.viewport_width = 1280;
    std::vector<const MaterialCacheLifecycleState *> owners;
    const auto reset = [&](unsigned capacity) {
        renderer.invalidate_generated_resources();
        renderer.enable_scheduling(std::array<std::uint32_t, 4>{capacity, 16, 16, 16},
                                    patch_settings, {64, 64});
        owners.clear();
        for (std::size_t id = 0; id < count; ++id) owners.push_back(&renderer.patch_state(id));
    };
    const auto stable = [&] {
        check(renderer.scheduler_state().pending.empty(), "Loading retained an ordinary pending queue");
        for (std::size_t id = 0; id < count; ++id) {
            const auto &s = renderer.patch_state(id);
            check(&s == owners[id] && renderer.scheduled_patch(id).state == owners[id] &&
                      s.staging == std::array<std::uintptr_t, 5>{},
                  "Loading replaced a patch owner or retained material staging");
        }
    };
    Writer gl_mips, d3d9_mips, steps, generated_order;
    std::size_t generations = 0, mip_pairs = 0, load_calls = 0;
    const auto generated = [&](std::size_t id, const GlMaterialWorldPrepared &job) {
        check(job.plan.passes.size() == 4 &&
                  renderer.patch_state(id).staging == std::array<std::uintptr_t, 5>{},
              "Loading generation must append and release its base material batch");
        native.execute(id, job, field.extract_a8_texture(job.height_table_slot % 16,
                                                        job.height_table_slot / 16), 64);
        const auto &texture = renderer.final_texture(id);
        check(texture.levels().size() == 7, "Loading requires all base mip levels");
        generated_order.word(static_cast<unsigned>(id));
        for (unsigned mip = 0; mip < 7; ++mip) {
            const auto raw = texture.read_mip_raw(mip);
            const auto original = reader.read(native.finals.at(id).value, mip);
            check(raw.size() * 2 == original.size(), "Loading mip extent differs from original");
            for (std::size_t pixel = 0; pixel < raw.size() / 2; ++pixel) {
                const unsigned packed = unsigned(raw[pixel * 2]) | (unsigned(raw[pixel * 2 + 1]) << 8);
                const unsigned codes[]{packed >> 11, (packed >> 5) & 63, packed & 31};
                for (unsigned channel = 0; channel < 3; ++channel) {
                    const unsigned bits = channel == 1 ? 6 : 5;
                    const float decoded = float((codes[channel] << (8 - bits)) |
                                                 (codes[channel] >> (2 * bits - 8))) / 255.f;
                    check(std::bit_cast<std::uint32_t>(decoded) ==
                              std::bit_cast<std::uint32_t>(original[pixel * 4 + channel]),
                          "Loaded RGB565 mip differs from original D3D9 shader output");
                }
                check(original[pixel * 4 + 3] == 1, "Original loading mip is unwritten");
            }
            gl_mips.blob(raw);
            d3d9_mips.blob(Bytes(reinterpret_cast<const std::uint8_t *>(original.data()),
                                 reinterpret_cast<const std::uint8_t *>(original.data()) + original.size() * 4));
            ++mip_pairs;
        }
        ++generations;
    };
    const auto load = [&](unsigned stamp) {
        const auto result = renderer.load_step(stamp, seed, seed, light, 4, true, generated);
        stable();
        check(result.prepared == result.completed && result.completed == result.generated &&
                  result.prepared <= 4,
              "Loading must serialize and complete up to four actual jobs per step");
        const auto &state = renderer.loading_state();
        for (auto value : {stamp, state.cursor, state.background_cursor, state.prepared_in_step,
                           state.finished, std::bit_cast<std::uint32_t>(state.progress),
                           static_cast<unsigned>(result.prepared)}) steps.word(value);
        ++load_calls;
        return result;
    };
    reset(1129);
    renderer.begin_loading(views, true, true);
    check(renderer.loading_state().foreground.size() == count && renderer.loading_state().background.empty(),
          "BlockOnLoad must retain the complete original world in order");
    bool rejected = false;
    try { renderer.schedule_frame(views, settings, 2, seed, seed, light, 4, true); }
    catch (const std::invalid_argument &) { rejected = true; }
    check(rejected && renderer.free_cache_count(64) == 1129, "Ordinary frame mutated an active load");
    for (unsigned step = 0; step < count / 4; ++step) {
        const auto result = load(step + 2);
        check(!result.finished && result.generated == 4 && renderer.loading_state().cursor == (step + 1) * 4,
              "Complete-world load changed the four-item or deferred-finished contract");
        if (step % 32 == 0) std::cout << "Loaded " << (step + 1) * 4 << " / " << count << '\n' << std::flush;
    }
    check(load(282).finished && load(283).generated == 0 && renderer.free_cache_count(64) == 9,
          "Completed load regenerated caches or lost a lease");
    std::vector<GLuint> retained;
    for (std::size_t id = 0; id < count; ++id) {
        const auto &state = renderer.patch_state(id);
        check(state.completed_state == 1 && state.pending_jobs == UINT32_MAX,
              "Preload did not preserve original state1/counter semantics");
        retained.push_back(renderer.final_texture(id).id());
    }
    // Original state1 -> state3 creates LAND geometry without consuming a
    // generation attempt, even with the recovered moving budget of zero.
    settings.attempt_budget = 0;
    const auto visible = renderer.schedule_frame(views, settings, 284, seed, seed, light, 4, true, generated);
    check(visible.prepared == count && visible.completed == count && visible.generated == 0,
          "Preloaded world did not become drawable with zero generation budget");
    stable();
    for (std::size_t id = 0; id < count; ++id)
        check(renderer.patch_state(id).completed_state == 3 && renderer.final_texture(id).id() == retained[id],
              "Visible state1 upgrade replaced its base texture");
    const auto view = overview_frame();
    std::vector<float> gl_scene;
    {
        SceneTarget target(context, 1280, 960);
        target.begin();
        for (std::size_t id = 0; id < count; ++id) renderer.draw(id, view, scene, 1280, 960);
        gl_scene = target.pixels();
        renderer.finish_frame();
    }
    Bytes native_scene;
    {
        NativeSceneTarget target(native, 1280, 960);
        target.begin();
        for (std::size_t id = 0; id < count; ++id) {
            const auto p = world->terrain().land().patch(id / world->cache().rows, id % world->cache().rows);
            const LandGrid grid(p.source_samples_x(), p.source_samples_y(), 1);
            const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(), grid.indices());
            native.land.bind(mesh, 1, renderer.land_constants(id, view), native.images(id, false),
                             native.heights.at(p.texture_table_slot()).value);
            native.land.submit(mesh);
        }
        native_scene = target.pixels();
    }
    check(native_scene.size() == gl_scene.size() * 4, "Loaded LAND image extent");
    double maximum = 0;
    std::size_t covered = 0;
    for (std::size_t i = 0; i < gl_scene.size(); ++i) {
        float original;
        std::memcpy(&original, native_scene.data() + i * 4, 4);
        check(std::isfinite(gl_scene[i]) && std::isfinite(original), "Nonfinite loaded LAND pixel");
        maximum = std::max(maximum, std::abs(double(gl_scene[i]) - original));
        if (i % 4 == 3) {
            check(gl_scene[i] == original, "Loaded LAND coverage differs");
            covered += original == 1;
        }
    }
    check(maximum <= 1e-5 && covered > 800000, "Loaded complete LAND scene differs from original");
    save(out / "loading-scene-gl.f32", Bytes(reinterpret_cast<const std::uint8_t *>(gl_scene.data()),
                                             reinterpret_cast<const std::uint8_t *>(gl_scene.data()) + gl_scene.size() * 4));
    save(out / "loading-scene-d3d9.f32", native_scene);

    reset(1129);
    for (std::size_t id = 17; id < count; ++id) renderer.set_scheduled_patch(id, false);
    renderer.begin_loading(views, false, false);
    check(renderer.loading_state().foreground.size() == 17 && renderer.loading_state().background.size() == count - 17,
          "Initial background membership was not retained");
    for (unsigned step = 0; step < 5; ++step)
        check(!load(300 + step).finished, "Partial preload finished before the following call");
    check(load(305).finished, "Partial preload did not complete");
    settings.attempt_budget = 1;
    const auto background = renderer.schedule_frame({}, settings, 306, seed, seed, light, 4, true, generated);
    check(background.generated == 1 && renderer.patch_state(17).completed_state == 1 &&
              renderer.loading_state().background_cursor == 1 && renderer.scheduler_state().background_cursor == 1,
          "Ordinary scheduler lost the original loading background queue/cursor");
    stable();
    renderer.finish_frame();

    // Pool take searches larger buckets too. Exhaust every bucket here.
    renderer.invalidate_generated_resources();
    renderer.enable_scheduling(std::array<std::uint32_t, 4>{3, 0, 0, 0}, patch_settings, {64, 64});
    owners.clear();
    for (std::size_t id = 0; id < count; ++id) owners.push_back(&renderer.patch_state(id));
    renderer.begin_loading({}, true, false);
    check(load(400).generated == 3 && renderer.loading_state().cursor == count && !renderer.loading_state().finished,
          "Initial acquisition failure did not terminate the queue after its three successful jobs");
    check(load(401).finished && renderer.free_cache_count(64) == 0, "Exhausted loading did not finish");

    reset(1129);
    renderer.begin_loading({}, false, true);
    check(load(500).finished && renderer.loading_state().progress == 1 && renderer.free_cache_count(64) == 1129,
          "Empty foreground allocated or failed to complete immediately");
    renderer.finish_frame();
    reset(1129);
    renderer.begin_loading({}, true, false);
    bool failed = false;
    try {
        renderer.load_step(600, seed, seed, light, 4, true,
            [](std::size_t, const GlMaterialWorldPrepared &) { throw std::runtime_error("injected loading consumer failure"); });
    } catch (const std::runtime_error &) { failed = true; }
    check(failed && renderer.free_cache_count(64) == 1129, "Failed loading did not return all actual leases");
    stable();
    for (std::size_t id = 0; id < count; ++id)
        check(renderer.patch_state(id).completed_state == 0 && renderer.patch_state(id).requested_state == 0,
              "Failed loading did not reset stable patch states");
    renderer.begin_loading({}, false, true);
    check(load(601).finished, "Renderer could not restart initial loading after failure");
    renderer.finish_frame();
    save(out / "loading-gl-mips.bin", gl_mips.bytes);
    save(out / "loading-d3d9-mips.f32", d3d9_mips.bytes);
    save(out / "loading-steps.bin", steps.bytes);
    save(out / "loading-generations.bin", generated_order.bytes);
    std::ostringstream report;
    report << std::setprecision(17) << "{\"passed\":true,\"generations\":" << generations
           << ",\"mipPairs\":" << mip_pairs << ",\"loadCalls\":" << load_calls
           << ",\"fullWorldPatches\":" << count << ",\"maximumSceneError\":" << maximum
           << ",\"coveredPixels\":" << covered
           << ",\"tolerance\":1e-5,\"gpuExecuted\":true,\"fullMaterialJobsExecuted\":true,"
              "\"zeroBudgetState1Upgrade\":true,\"backgroundQueueRetained\":true,"
              "\"poolExhaustionChecked\":true,\"consumerFailureRecoveryChecked\":true,"
              "\"savedSettingsRecovered\":false,\"livePreviewConnected\":false,\"fullGraphicsParity\":false}\n";
    blend_validation::text(out / "loading-comparison.json", report.str());
    std::cout << "PASS actual material loading: " << generations << " generations / " << mip_pairs
              << " original mip pairs / complete LAND scene\n" << std::flush;
}
} // namespace
