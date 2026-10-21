#pragma once
// Real camera -> retained scheduler -> native workers -> GPU cache generation.
// The water owner publishes the sole camera used by selection and both draws.
namespace {
void validate_material_world_camera_scheduler(
    const fs::path &root, const fs::path &camera_inputs, std::shared_ptr<GlContext> context,
    std::shared_ptr<const MaterialWorld> world, GlMaterialWorldRenderer &renderer, NativeWorld &native,
    const std::map<unsigned, Bytes> &windows, GlLandMaterialInputs scene,
    const GlMaterialWorldLighting &light, const MaterialNormalSeed &seed, const fs::path &out) {
    const auto count = world->cache().patches.size();
    check(count == 1120, "Camera scheduler requires the established complete world");
    const auto asset = [&](const char *archive_name, const char *key) {
        const civ5::assets::AssetEnvironment environment(root);
        const auto source = archive(environment, archive_name);
        const auto found = source->find(civ5::assets::AssetKey(key));
        check(std::holds_alternative<civ5::assets::AssetHandle>(found), "Camera water asset unavailable");
        return std::get<civ5::assets::AssetHandle>(found).read_bytes();
    };
    const auto waves =
        civ5::assets::LeanData::parse(asset("Resource/Common/TerrainModels.fpk", "waterbumps16.lean"));
    const auto color =
        civ5::assets::DdsData::parse(asset("Resource/DX9/Upgrade1Textures.fpk", "waterdepthcolor_new.dds"));
    const auto terrain = std::shared_ptr<const TerrainWorld>(world, &world->terrain());
    GlWaterWorldRenderer water(context, terrain, waves, color, 0, X87Precision::double_precision);
    const auto lighting = terrain_lighting_defaults(X87Precision::double_precision);
    const WaterReflectionFrameSettings water_settings{
        lighting.angles, lighting.shadow, {water.domain().scales[0], water.domain().scales[1]}};
    water_validation::save(out / "water-settings.bin", std::array{water_settings});
    const auto input_bytes = read(camera_inputs);
    WorldReader input{input_bytes};
    const auto signature = input.take(8);
    check(std::string(signature.begin(), signature.end()) == "C5SCGPU1", "Camera GPU primitive schema");
    const auto pose_count = input.word();
    check(pose_count >= 2 && pose_count <= 16, "Camera GPU pose count");
    const MaterialPatchSettings patch_settings{64, 0, 0, 0, X87Precision::double_precision};
    MaterialCacheSchedulerSettings settings{};
    // This deliberately wrong entering value must be replaced by the camera.
    settings.projection.fill(std::numeric_limits<float>::quiet_NaN());
    settings.viewport_width = 1280;
    settings.cull = true;
    std::map<std::size_t, Com<IDirect3DTexture9>> higher;
    std::vector<const MaterialCacheLifecycleState *> identities;
    mip_validation::NativeMipReader reader(native.cache.device.value);
    unsigned generation_count = 0, mip_pairs = 0, frame_count = 0, rejected = 0, fallback_count = 0;
    unsigned multi_view_frames = 0, unculled_rejections = 0;
    std::array<unsigned, 4> generated_widths{};
    double maximum = 0;
    Writer generation_records;
    const auto reset = [&](std::array<std::uint32_t, 4> counts) {
        renderer.invalidate_generated_resources();
        renderer.enable_scheduling(counts, patch_settings, {64, 64});
        higher.clear();
        identities.clear();
        for (std::size_t i = 0; i < count; ++i)
            identities.push_back(&renderer.patch_state(i));
    };
    reset({1129, 16, 16, 16});
    const auto snapshot = [&] {
        Writer bytes;
        for (unsigned width : {64u, 128u, 256u, 512u})
            bytes.word(static_cast<std::uint32_t>(renderer.free_cache_count(width)));
        bytes.word(renderer.scheduler_state().background_cursor);
        for (const auto &indices : renderer.scheduler_state().visible) {
            bytes.word(static_cast<unsigned>(indices.size()));
            for (auto index : indices)
                bytes.word(index);
        }
        bytes.word(static_cast<unsigned>(renderer.scheduler_state().eviction_candidates.size()));
        for (auto index : renderer.scheduler_state().eviction_candidates)
            bytes.word(index);
        for (std::size_t i = 0; i < count; ++i) {
            const auto &s = renderer.patch_state(i);
            const auto &p = renderer.scheduled_patch(i);
            check(&s == identities[i] && p.state == identities[i] &&
                      s.staging == std::array<std::uintptr_t, 5>{},
                  "Camera frame changed stable owners or retained staging");
            for (auto v : {s.completed_state, s.requested_state, s.frame_stamp, s.pending_jobs.load(),
                           s.textures.final_size, s.textures.higher_size, s.textures.desired_higher_size,
                           s.textures.final[0], s.textures.higher[0], p.eviction_stamp})
                bytes.word(v);
            for (float distance : p.plane_distances)
                bytes.scalar(distance);
            bytes.word(p.frustum_visible);
            bytes.word(p.submitted_view);
        }
        check(renderer.scheduler_state().pending.empty(), "Camera completion queue was not drained");
        return bytes.bytes;
    };
    const auto mips = [&](const GlTexture &texture, IDirect3DTexture9 *original, const char *role) {
        const auto size = texture.levels().front().width;
        for (unsigned mip = 0; mip < texture.levels().size(); ++mip) {
            const auto raw = texture.read_mip_raw(mip);
            const auto pixels = reader.read(original, mip);
            const auto prefix = "cache-" + std::to_string(generation_count) + "-" + role + "-" +
                                std::to_string(size) + "-" + std::to_string(mip);
            save(out / (prefix + "-gl.bin"), raw);
            save(out / (prefix + "-d3d9.f32"), float_bytes(pixels));
            check(raw.size() * 2 == pixels.size(), "Camera cache mip extent differs");
            for (std::size_t i = 0; i < raw.size() / 2; ++i) {
                const unsigned packed = unsigned(raw[i * 2]) | (unsigned(raw[i * 2 + 1]) << 8);
                const unsigned codes[]{packed >> 11, (packed >> 5) & 63, packed & 31};
                for (unsigned channel = 0; channel < 3; ++channel) {
                    const unsigned bits = channel == 1 ? 6 : 5;
                    const float decoded =
                        float((codes[channel] << (8 - bits)) | (codes[channel] >> (2 * bits - 8))) / 255.f;
                    check(std::bit_cast<std::uint32_t>(decoded) ==
                              std::bit_cast<std::uint32_t>(pixels[i * 4 + channel]),
                          "Camera cache differs from original RGB565 mip");
                }
                check(pixels[i * 4 + 3] == 1, "Camera original mip is unwritten");
            }
            ++mip_pairs;
        }
    };
    const auto generated = [&](std::size_t index, const GlMaterialWorldPrepared &job) {
        const auto &s = renderer.patch_state(index);
        const bool high = job.plan.passes.size() == 5;
        check(high || job.plan.passes.size() == 4, "Camera scheduled cache plan");
        Com<IDirect3DTexture9> target;
        if (high) {
            const auto width = s.textures.higher_size;
            hr(native.cache.device->CreateTexture(width, width, 0,
                                                  D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP,
                                                  D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &target.value, nullptr),
               "Camera original higher cache");
            native.cache.resources[job.plan.passes[3].targets[0]] = target.value;
        }
        native.execute(index, job, windows.at(job.height_table_slot), s.textures.final_size);
        mips(renderer.final_texture(index), native.finals.at(index).value, "base");
        const auto width = high ? s.textures.higher_size : s.textures.final_size;
        ++generated_widths[std::countr_zero(width) - 6];
        if (high) {
            higher.erase(index);
            higher.emplace(index, std::move(target));
            mips(renderer.higher_texture(index), higher.at(index).value, "higher");
            fallback_count += width < s.textures.desired_higher_size;
        }
        for (auto v : {frame_count, static_cast<unsigned>(index), s.textures.desired_higher_size,
                       s.textures.final_size, high ? width : 0u})
            generation_records.word(v);
        ++generation_count;
    };
    WaterCameraFrameState prior{};
    for (unsigned pose = 0; pose < pose_count; ++pose) {
        const auto profile = input.word(); // 0 ordinary, 1 restricted higher pool, 2 reject overlapping views
        check(profile <= 2, "Camera GPU pool profile");
        PerspectiveCamera camera{};
        camera.field_of_view = std::bit_cast<float>(input.word());
        camera.aspect_ratio = std::bit_cast<float>(input.word());
        camera.near_plane = std::bit_cast<float>(input.word());
        camera.far_plane = std::bit_cast<float>(input.word());
        for (auto &v : camera.world)
            v = std::bit_cast<float>(input.word());
        const auto folder = out / ("pose-" + std::to_string(pose));
        fs::create_directory(folder);
        water_validation::save(folder / "camera.bin", std::array{camera});
        water_validation::save(folder / "prior.bin", std::array{prior});
        std::vector<float> reciprocals, square_roots;
        const auto rcp = [&](float v) {
            const auto estimate = _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(v)));
            reciprocals.insert(reciprocals.end(), {v, estimate});
            return estimate;
        };
        const auto rsqrt = [&](float v) {
            const auto estimate = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v)));
            square_roots.insert(square_roots.end(), {v, estimate});
            return estimate;
        };
        water.prepare_camera_frame(camera, water_settings, rcp, rsqrt);
        const auto &frame = water.camera_state();
        prior = {frame, water.frame_state()};
        water_validation::save(folder / "state.bin", std::array{prior});
        water_validation::save(folder / "rcp.bin", reciprocals);
        water_validation::save(folder / "rsqrt.bin", square_roots);
        if (profile == 2) {
            check(frame.view_count > 1, "Overlap acceptance requires a real multi-view camera");
            const auto before = snapshot();
            auto invalid = frame;
            invalid.view_count = 5;
            bool failed = false;
            try {
                renderer.schedule_camera_frame(invalid, settings, frame_count + 2, seed, seed, light, 4,
                                               true, generated);
            } catch (const std::invalid_argument &) {
                failed = true;
            }
            check(failed && snapshot() == before, "Invalid camera mutated the scheduled world");
            ++rejected;
        }
        check(frame.view_count >= 1 && frame.view_count <= 4, "Complete active camera views required");
        if (frame.view_count > 1) {
            const auto before = snapshot();
            auto unculled = settings;
            unculled.cull = false;
            bool failed = false;
            try {
                const auto result = renderer.schedule_camera_frame(
                    frame, unculled, frame_count + 2, seed, seed, light, 4, true, generated);
                check(result.prepared == result.completed, "Unculled multi-view did not complete prepared work");
            } catch (const std::invalid_argument &) {
                failed = true;
            }
            if (failed) {
                check(snapshot() == before, "Unculled multi-view rejection changed world state");
                ++unculled_rejections;
            }
        }
        if (profile == 1)
            reset({1129, 16, 0, 0});
        std::vector<LandMaterialConstants> land_cameras(frame.view_count, overview_frame());
        Writer camera_constants;
        for (unsigned view = 0; view < frame.view_count; ++view) {
            auto &land_camera = land_cameras[view];
            const auto matrix = matrix_multiply(frame.views[view], frame.projection,
                                                X87Precision::double_precision, MatrixPath::sse);
            for (unsigned row = 0; row < 4; ++row)
                std::copy_n(matrix.begin() + row * 4, 4, land_camera.view_projection[row].begin());
            std::copy_n(frame.inverse_views[view].begin() + 12, 3, land_camera.camera.begin());
            const auto bytes = frame_bytes(land_camera);
            camera_constants.bytes.insert(camera_constants.bytes.end(), bytes.begin(), bytes.end());
        }
        save(folder / "land-camera.bin", camera_constants.bytes);
        for (unsigned pass = 0; pass < 2; ++pass) {
            settings.attempt_budget =
                (pose == 0 || profile == 1 || frame.view_count > 1) && pass == 0 ? count : 8;
            const auto result = renderer.schedule_camera_frame(frame, settings, frame_count + 2, seed, seed,
                                                               light, 4, true, generated);
            check(result.prepared == result.completed, "Camera scheduler did not complete prepared work");
            snapshot();
            Writer desired, state;
            const auto prefix = std::to_string(pass);
            std::size_t visible_count = 0;
            for (unsigned view = 0; view < frame.view_count; ++view) {
                Writer visible;
                for (auto index : renderer.scheduler_state().visible[view])
                    visible.word(index);
                const auto view_name = view == 0 ? "" : "-view-" + std::to_string(view);
                save(folder / (prefix + view_name + "-visible.bin"), visible.bytes);
                visible_count += renderer.scheduler_state().visible[view].size();
            }
            for (std::size_t i = 0; i < count; ++i) {
                const auto &s = renderer.patch_state(i);
                desired.word(s.textures.desired_higher_size);
                for (auto v : {s.completed_state, s.textures.final_size, s.textures.higher_size})
                    state.word(v);
            }
            save(folder / (prefix + "-desired.bin"), desired.bytes);
            save(folder / (prefix + "-patches.bin"), state.bytes);
            Bytes gl_image, native_image;
            {
                SceneTarget target(context, 1280, 960);
                target.begin();
                for (unsigned view = 0; view < frame.view_count; ++view)
                    for (auto index : renderer.scheduler_state().visible[view])
                        renderer.draw(index, land_cameras[view], scene, 1280, 960);
                gl_image = float_bytes(target.pixels());
                renderer.finish_frame();
            }
            {
                NativeSceneTarget target(native, 1280, 960);
                target.begin();
                for (unsigned view = 0; view < frame.view_count; ++view)
                    for (auto index : renderer.scheduler_state().visible[view]) {
                        const auto completed = renderer.patch_state(index).completed_state;
                        if (completed == 0 || completed == 1 || completed == 4)
                            continue;
                        const auto p =
                            terrain->land().patch(index / world->cache().rows, index % world->cache().rows);
                        const LandGrid grid(p.source_samples_x(), p.source_samples_y(), 1);
                        const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(),
                                                    grid.indices());
                        auto images = native.images(index, false);
                        if (completed == 5)
                            images[0] = higher.at(index).value;
                        native.land.bind(mesh, 1, renderer.land_constants(index, land_cameras[view]), images,
                                         native.heights.at(p.texture_table_slot()).value);
                        native.land.submit(mesh);
                    }
                native_image = target.pixels();
            }
            save(folder / (prefix + "-gl.f32"), gl_image);
            save(folder / (prefix + "-d3d9.f32"), native_image);
            check(gl_image.size() == native_image.size(), "Camera scene extent differs");
            std::size_t covered = 0;
            double error = 0;
            for (std::size_t at = 0; at < gl_image.size(); at += 4) {
                float a, b;
                std::memcpy(&a, gl_image.data() + at, 4);
                std::memcpy(&b, native_image.data() + at, 4);
                check(std::isfinite(a) && std::isfinite(b), "Nonfinite camera scene pixel");
                error = std::max(error, std::abs(double(a) - b));
                if (at % 16 == 12) {
                    check(a == b, "Camera scene silhouettes differ");
                    covered += a == 1;
                }
            }
            maximum = std::max(maximum, error);
            check(covered > 1000 && error <= 1e-5, "Camera LAND image exceeds original tolerance/coverage");
            std::cout << "PASS camera pose " << pose << " pass " << pass << " visible " << visible_count
                      << " views " << frame.view_count << " generated " << result.generated << " error "
                      << error << '\n'
                      << std::flush;
            ++frame_count;
            multi_view_frames += frame.view_count > 1;
        }
    }
    input.end();
    save(out / "generation-records.bin", generation_records.bytes);
    check(std::all_of(generated_widths.begin(), generated_widths.end(), [](unsigned n) { return n > 0; }) &&
              fallback_count > 0 && rejected == 1,
          "Camera suite did not reach every cache size, fallback and invalid-view boundary");
    std::ostringstream report;
    report << std::setprecision(17) << "{\"passed\":true,\"frames\":" << frame_count
           << ",\"poses\":" << pose_count << ",\"generated\":" << generation_count
           << ",\"mipPairs\":" << mip_pairs << ",\"maximumError\":" << maximum
           << ",\"fallbackGenerations\":" << fallback_count << ",\"rejectedCameras\":" << rejected
           << ",\"multiViewFrames\":" << multi_view_frames
           << ",\"unculledMultiViewRejections\":" << unculled_rejections << ",\"generatedWidths\":["
           << generated_widths[0] << ',' << generated_widths[1] << ',' << generated_widths[2] << ','
           << generated_widths[3]
           << "],\"sharedWaterCamera\":true,\"stableOwners\":true,\"fullCompositionCompared\":false,"
              "\"fullOriginalConnectedFrameExecuted\":false,\"fullGraphicsParity\":false}\n";
    blend_validation::text(out / "camera-scheduler-result.json", report.str());
}
} // namespace
