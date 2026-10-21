#pragma once
// Included after the complete-world resource capture helpers. Inputs below are
// the real immutable world and the original D3D9 shader controls, not fixtures
// containing expected cache pixels or material streams.
namespace {
void validate_material_world_scheduler(std::shared_ptr<GlContext> context, const MaterialWorld &world,
                                       GlMaterialWorldRenderer &renderer, NativeWorld &native,
                                       const std::map<unsigned, Bytes> &windows, GlLandMaterialInputs scene,
                                       const GlMaterialWorldLighting &light, const MaterialNormalSeed &seed,
                                       const MaterialWorldResourceCapture &legacy, const Bytes &legacy_plans,
                                       const fs::path &out) {
    const auto count = world.cache().patches.size();
    check(count == 1120, "Scheduled validation requires the complete established world");
    renderer.invalidate_generated_resources();
    const MaterialPatchSettings patch_settings{64, 0, 0, 0, X87Precision::double_precision};
    const std::array<std::uint32_t, 4> counts{1121, 16, 16, 16};
    renderer.enable_scheduling(counts, patch_settings, {64, 64});
    std::vector<const MaterialCacheLifecycleState *> identities;
    for (std::size_t i = 0; i < count; ++i)
        identities.push_back(&renderer.patch_state(i));
    MaterialCacheSchedulerSettings settings{};
    settings.projection = identity_matrix();
    settings.viewport_width = 1280;
    settings.attempt_budget = count;
    const std::array<MaterialCacheSchedulerView, 1> views{{{identity_matrix(), {}}}};
    std::map<std::size_t, Com<IDirect3DTexture9>> higher;
    std::map<std::size_t, Bytes> base_plans;
    std::size_t generation_count = 0, mip_pairs = 0, scene_pairs = 0;
    double maximum = 0;
    mip_validation::NativeMipReader reader(native.cache.device.value);
    const auto check_mips = [&](const GlTexture &gl, IDirect3DTexture9 *original,
                                const std::string &prefix = {}) {
        for (unsigned mip = 0; mip < gl.levels().size(); ++mip) {
            const auto raw = gl.read_mip_raw(mip);
            const auto pixels = reader.read(original, mip);
            if (!prefix.empty()) {
                save(out / (prefix + "-" + std::to_string(mip) + "-gl.bin"), raw);
                save(out / (prefix + "-" + std::to_string(mip) + "-d3d9.f32"), float_bytes(pixels));
            }
            check(raw.size() * 2 == pixels.size(), "Scheduled mip extent differs from original");
            for (std::size_t i = 0; i < raw.size() / 2; ++i) {
                const auto packed = unsigned(raw[i * 2]) | (unsigned(raw[i * 2 + 1]) << 8);
                const unsigned codes[]{packed >> 11, (packed >> 5) & 63, packed & 31};
                for (unsigned channel = 0; channel < 3; ++channel) {
                    const unsigned bits = channel == 1 ? 6 : 5;
                    const float decoded =
                        float((codes[channel] << (8 - bits)) | (codes[channel] >> (2 * bits - 8))) / 255.f;
                    check(std::bit_cast<std::uint32_t>(decoded) ==
                              std::bit_cast<std::uint32_t>(pixels[i * 4 + channel]),
                          "Scheduled original RGB565 mip pixels differ");
                }
                check(pixels[i * 4 + 3] == 1, "Scheduled native mip is unwritten");
            }
            ++mip_pairs;
        }
    };
    const auto generated = [&](std::size_t index, const GlMaterialWorldPrepared &job) {
        check(renderer.patch_state(index).staging == std::array<std::uintptr_t, 5>{},
              "GPU generation preceded original material append/release");
        const bool high = job.plan.passes.size() == 5;
        check(high || job.plan.passes.size() == 4, "Unexpected scheduled pass count");
        Com<IDirect3DTexture9> target;
        if (high) {
            const auto width = renderer.patch_state(index).textures.higher_size;
            hr(native.cache.device->CreateTexture(width, width, 0,
                                                  D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP,
                                                  D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &target.value, nullptr),
               "Scheduled original higher texture");
            native.cache.resources[job.plan.passes[3].targets[0]] = target.value;
        } else {
            base_plans[index] = serialize_plan(job.plan);
        }
        native.execute(index, job, windows.at(job.height_table_slot), 64);
        check_mips(renderer.final_texture(index), native.finals.at(index).value);
        if (high) {
            higher.erase(index);
            higher.emplace(index, std::move(target));
            check_mips(renderer.higher_texture(index), higher.at(index).value,
                       "higher-" + std::to_string(generation_count) + "-" + std::to_string(index));
        }
        ++generation_count;
    };
    const auto stable = [&] {
        check(renderer.scheduler_state().pending.empty(), "Completed frame retained pending entries");
        for (std::size_t i = 0; i < count; ++i) {
            const auto &state = renderer.patch_state(i);
            check(&state == identities[i] && renderer.scheduled_patch(i).state == identities[i] &&
                      state.staging == std::array<std::uintptr_t, 5>{},
                  "Scheduled frame replaced its patch owner or retained staging");
        }
    };
    const auto frame = [&](std::uint32_t stamp, std::span<const MaterialCacheSchedulerView> visible) {
        auto result =
            renderer.schedule_frame(visible, settings, stamp, seed, seed, light, 4, true, generated);
        stable();
        check(result.completed == result.prepared, "Scheduled job did not complete");
        return result;
    };
    const auto scene_capture = [&](std::string_view name, std::uint32_t stamp) {
        const auto view = overview_frame();
        Bytes gl_image, native_image;
        {
            SceneTarget target(context, 1280, 960);
            target.begin();
            for (std::size_t i = 0; i < count; ++i)
                renderer.draw(i, view, scene, 1280, 960);
            gl_image = float_bytes(target.pixels());
            renderer.finish_frame();
        }
        {
            NativeSceneTarget target(native, 1280, 960);
            target.begin();
            for (std::size_t i = 0; i < count; ++i) {
                const auto state = renderer.patch_state(i).completed_state;
                if (state == 0 || state == 1 || state == 4)
                    continue;
                const auto p = world.terrain().land().patch(i / world.cache().rows, i % world.cache().rows);
                const LandGrid grid(p.source_samples_x(), p.source_samples_y(), 1);
                const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(), grid.indices());
                auto images = native.images(i, false);
                if (state == 5)
                    images[0] = higher.at(i).value;
                native.land.bind(mesh, 1, renderer.land_constants(i, view), images,
                                 native.heights.at(p.texture_table_slot()).value);
                native.land.submit(mesh);
                check(renderer.scheduled_patch(i).eviction_stamp == stamp,
                      "Accepted draw did not record its current frame stamp");
            }
            native_image = target.pixels();
        }
        check(gl_image.size() == native_image.size(), "Scheduled complete scene extent");
        for (std::size_t i = 0; i < gl_image.size(); i += 4) {
            float a, b;
            std::memcpy(&a, gl_image.data() + i, 4);
            std::memcpy(&b, native_image.data() + i, 4);
            check(std::isfinite(a) && std::isfinite(b), "Nonfinite scheduled LAND scene");
            maximum = std::max(maximum, std::abs(double(a) - b));
            if (i % 16 == 12)
                check(a == b, "Scheduled LAND silhouettes differ");
        }
        check(maximum <= 1e-5, "Scheduled full LAND scene exceeds original tolerance");
        save(out / (std::string(name) + "-gl.f32"), gl_image);
        save(out / (std::string(name) + "-d3d9.f32"), native_image);
        ++scene_pairs;
        return gl_image;
    };
    const auto initial = frame(2, views);
    check(initial.generated == count && initial.prepared == count && renderer.free_cache_count(64) == 1,
          "Initial full scheduler did not generate every real base lease");
    Writer plans;
    for (std::size_t i = 0; i < count; ++i) {
        const auto &s = renderer.patch_state(i);
        check(s.completed_state == 3 && s.pending_jobs == UINT32_MAX && s.frame_stamp == 2,
              "Initial scheduler completion changed original counter/state semantics");
        plans.blob(base_plans.at(i));
    }
    check(plans.bytes == legacy_plans, "Scheduled base GPU plan differs from the explicit original plan");
    save(out / "scheduled-base-plans.bin", plans.bytes);
    const auto initial_capture =
        capture_material_resource_world(context, world, renderer, native, scene, out / "scheduled-base");
    check(initial_capture.gl_image == legacy.gl_image &&
              initial_capture.native_image == legacy.native_image &&
              initial_capture.gl_mips == legacy.gl_mips && initial_capture.native_mips == legacy.native_mips,
          "Scheduled base world differs from the complete established original world");
    std::cout << "PASS full scheduled base world, 1120 stable owners and original mip chains\n" << std::flush;
    settings.attempt_budget = 4;
    check(frame(3, views).generated == 4 && renderer.free_cache_count(512) == 12,
          "Higher scheduler did not reserve and generate four real 512 caches");
    const auto high_scene = scene_capture("scheduled-higher", 3);
    std::map<std::size_t, GLuint> retained;
    for (std::size_t i = 0; i < count; ++i)
        if (renderer.patch_state(i).completed_state == 5) {
            retained[i] = renderer.higher_texture(i).id();
            renderer.set_scheduled_patch(i, false);
        }
    settings.attempt_budget = 0;
    check(frame(4, views).generated == 0, "Hidden higher caches regenerated");
    for (const auto &[i, texture] : retained) {
        check(renderer.patch_state(i).completed_state == 4 && renderer.higher_texture(i).id() == texture,
              "Hidden higher cache was not retained as state4");
        renderer.set_scheduled_patch(i, true);
    }
    const auto restored = frame(5, views);
    check(restored.prepared == 4 && restored.generated == 0 &&
              scene_capture("scheduled-restored", 5) == high_scene,
          "State4 to state5 did not reuse the same generated higher pixels");
    settings.attempt_budget = 8;
    check(frame(6, views).generated == 8 && renderer.free_cache_count(512) == 4,
          "Higher pool pressure setup failed");
    settings.attempt_budget = 0;
    frame(7, {});
    std::array<unsigned, 6> states{};
    for (std::size_t i = 0; i < count; ++i)
        ++states.at(renderer.patch_state(i).completed_state);
    check(states[0] == 8 && states[1] == 1105 && states[4] == 7 && renderer.free_cache_count(64) == 9 &&
              renderer.free_cache_count(512) == 9,
          "Real pool eviction did not stop at the original greater-than-eight threshold");
    const auto rebuild = frame(8, views);
    check(rebuild.generated == 0 && rebuild.prepared == 1112,
          "Budget-free geometry restoration regenerated material pixels");
    scene_capture("scheduled-pressure", 8);
    settings.fast = true;
    settings.fast_selection_budget = 0;
    check(frame(9, views).generated == 8,
          "Fast base-only recovery did not regenerate the eight evicted patches");
    scene_capture("scheduled-recovered", 9);
    settings.fast = false;
    settings.attempt_budget = 1;
    std::size_t revision = 0;
    while (renderer.patch_state(revision).completed_state != 5)
        ++revision;
    renderer.set_scheduled_patch(revision, true, 0, UINT32_MAX, 100);
    const auto old_higher = renderer.higher_texture(revision).id();
    check(frame(100, views).generated == 1 && renderer.patch_state(revision).frame_stamp == 100 &&
              renderer.higher_texture(revision).id() != old_higher,
          "Live material revision did not replace its higher lease");
    scene_capture("scheduled-revision", 100);
    // A failed worker must leave no acquired lease or half-built packet
    // published; every state address survives the all-patch retry boundary.
    renderer.set_scheduled_patch(revision, true, 0, UINT32_MAX, 101);
    unsigned visits = 0;
    bool failed = false;
    try {
        renderer.schedule_frame(
            views, settings, 101, seed,
            [&](int x, int y, float squared) {
                if (++visits == 145)
                    throw std::runtime_error("Scheduled producer failure");
                return seed(x, y, squared);
            },
            light, 4, true, generated);
    } catch (const std::runtime_error &e) {
        failed = std::string_view(e.what()) == "Scheduled producer failure";
    }
    stable();
    check(failed && visits == 145, "Scheduled failure injection was not reached");
    for (std::size_t i = 0; i < count; ++i)
        check(renderer.patch_state(i).completed_state == 0 && !renderer.prepared(i),
              "Failed world worker left a published patch");
    for (unsigned i = 0; i < 4; ++i)
        check(renderer.free_cache_count(64u << i) == counts[i], "Failed world worker leaked a texture lease");
    settings.attempt_budget = count;
    check(frame(102, views).generated == count, "Scheduled full retry failed");
    const auto retried =
        capture_material_resource_world(context, world, renderer, native, scene, out / "scheduled-retry");
    check(retried.gl_image == legacy.gl_image && retried.native_image == legacy.native_image &&
              retried.gl_mips == legacy.gl_mips && retried.native_mips == legacy.native_mips,
          "Scheduled worker failure changed the regenerated full world");
    for (unsigned i = 0; i < 4; ++i)
        renderer.finish_frame();
    stable();
    // Exercise the original background requested1 path with real generation,
    // geometry invalidation and a later budget-free 1->3 rebuild.
    renderer.invalidate_generated_resources();
    renderer.enable_scheduling(std::array<std::uint32_t, 4>{9, 0, 0, 0}, patch_settings, {64, 64});
    identities.clear();
    for (std::size_t i = 0; i < count; ++i) {
        identities.push_back(&renderer.patch_state(i));
        renderer.set_scheduled_patch(i, i == 0);
    }
    settings.attempt_budget = 1;
    const auto background = frame(103, {});
    const auto background_texture = renderer.final_texture(0).id();
    check(background.generated == 1 && background.prepared == 1 &&
              renderer.patch_state(0).completed_state == 1 &&
              renderer.patch_state(0).geometry.packets[0] == 0 && renderer.free_cache_count(64) == 8 &&
              renderer.scheduler_state().background_cursor == 1,
          "Background cache did not complete requested1 and invalidate its geometry");
    renderer.draw(0, overview_frame(), scene, 1280, 960); // state1 must skip before looking for a packet
    settings.attempt_budget = 0;
    const auto foreground = frame(104, views);
    check(foreground.generated == 0 && foreground.prepared == 1 &&
              renderer.patch_state(0).completed_state == 3 && renderer.patch_state(0).geometry.packets[0] &&
              renderer.final_texture(0).id() == background_texture &&
              renderer.patch_state(0).pending_jobs == UINT32_MAX - 1,
          "Background-to-visible cache did not reuse pixels and rebuild its actual packet");
    scene_capture("scheduled-background", 104);
    std::ostringstream report;
    report << std::setprecision(17) << "{\"passed\":true,\"patches\":" << count
           << ",\"generated\":" << generation_count << ",\"mipPairs\":" << mip_pairs
           << ",\"mixedScenePairs\":" << scene_pairs << ",\"maximumError\":" << maximum
           << ",\"fullBaseCaptures\":2,\"stableOwners\":true,\"actualPoolLeases\":true,"
              "\"higherView5Executed\":true,\"backgroundGenerationAndReuse\":true,\"workerFailureRecovered\":"
              "true,\"singleViewOnly\":true,"
              "\"fullOriginalConnectedFrameExecuted\":false,\"fullGraphicsParity\":false}\n";
    blend_validation::text(out / "scheduler-result.json", report.str());
    std::cout
        << "PASS retained scheduler higher generation, eviction, reuse and full worker-failure recovery\n";
}
} // namespace
