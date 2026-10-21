#pragma once
// Called while each API retains its independently generated material caches,
// current map and reflection target. Original CPU outputs only enter D3D9.
namespace {
struct FowSceneTargets {
    SceneTarget gl;
    NativeSceneTarget d3d9;
    fow_validation::NativeFow fow;
    FowSceneTargets(std::shared_ptr<GlContext> context, NativeWorld &original, const fs::path &shaders,
                    const fs::path &reference)
        : gl(context, 1280, 960), d3d9(original, 1280, 960),
          fow(shaders, reference / "fow", shaders.parent_path() / "assets", 2, 1280, 960, false, true,
              original.cache.device.value, d3d9.color.value, d3d9.depth.value) {}
};
using FowSceneImageSink = std::function<void(const char *, std::span<const std::uint8_t>)>;
template <class LandRenderer>
void render_fow_world_scene(std::shared_ptr<GlContext> context, std::shared_ptr<const MaterialWorld> world,
                            LandRenderer &land, NativeWorld &original, GlWaterWorldRenderer &water,
                            water_validation::NativeWater &native_water, const WaterSurfaceSettings &settings,
                            const WaterSurfaceFrame &surface_frame, WaterSurfaceView view,
                            GlLandMaterialInputs land_images, GlWaterSurfaceInputs water_images,
                            std::array<IDirect3DBaseTexture9 *, 14> native_water_images,
                            const GlDistanceFog &fog, float zoom, const fs::path &root,
                            const fs::path &shaders, const fs::path &reference, const fs::path &output,
                            const fs::path &land_probes = {}, FogOfWarScene *retained_fow = nullptr,
                            const fs::path &base_reference = {}, RiverWorldScene *rivers = nullptr,
                            FowSceneTargets *retained_targets = nullptr, const FowSceneImageSink &sink = {}) {
    scene_land_prepare(land, water.camera_state(), output / "scheduled-materials");
    std::unique_ptr<FogOfWarScene> owned_fow;
    if (!retained_fow)
        owned_fow = std::make_unique<FogOfWarScene>(context, world->terrain(), root);
    auto &fow = retained_fow ? *retained_fow : *owned_fow;
    fow.prepare(water, fog, zoom);
    const auto count = water.camera_state().view_count;
    fs::create_directories(output);
    const auto image = [&](const char *name, const auto &values) {
        const std::span<const std::uint8_t> bytes(reinterpret_cast<const std::uint8_t *>(values.data()),
                                                  values.size() * sizeof(values[0]));
        if (sink)
            sink(name, bytes);
        else
            blend_validation::write(output / name, bytes);
    };
    const auto exact = [&](const char *file, const auto &value) {
        const auto camera_file = std::string_view(file) == "camera.bin" ||
                                 std::string_view(file) == "bounds.bin" ||
                                 std::string_view(file) == "overlay.bin";
        const auto expected = blend_validation::read(
            (camera_file && !base_reference.empty() ? base_reference : reference) / file);
        const auto bytes =
            std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t *>(&value), sizeof(value));
        blend_validation::write(output / file, bytes);
        check(expected.size() == bytes.size() && std::equal(expected.begin(), expected.end(), bytes.begin()),
              (std::string("Connected original/C++ FOW scene differs: ") + file).c_str());
    };
    exact("camera.bin", water.camera_state());
    exact("bounds.bin", water.frame_state().bounds);
    exact("overlay.bin", fow.frame.overlays);
    exact("fow-scale.bin", fow.state.frame_scale());
    const auto native_frame = fow_validation::load_frame(reference / "fow");
    Writer commands, constants;
    commands.word(static_cast<std::uint32_t>(fow.frame.commands.passes.size()));
    commands.word(static_cast<std::uint32_t>(fow.frame.commands.draws.size()));
    for (const auto &p : fow.frame.commands.passes) {
        commands.word(static_cast<std::uint32_t>(p.kind));
        commands.word(p.begin);
        commands.word(p.end);
        for (float v : p.clear_color)
            commands.scalar(v);
    }
    for (const auto &d : fow.frame.commands.draws) {
        commands.word(d.geometry);
        commands.word(d.selector);
        commands.word(static_cast<std::uint32_t>(d.constants.size()));
        for (const auto &v : d.constants)
            for (float f : v)
                constants.scalar(f);
    }
    check(commands.bytes == blend_validation::read(reference / "fow/commands.bin") &&
              constants.bytes == blend_validation::read(reference / "fow/constants.bin"),
          "Connected C++ camera/height/overlay/FOW command chain differs from original");
    save(output / "commands.bin", commands.bytes);
    save(output / "constants.bin", constants.bytes);
    const auto frame_bytes = blend_validation::read(reference / "land-frame.bin"),
               fog_bytes = blend_validation::read(reference / "land-fog.bin"),
               water_bytes = blend_validation::read(reference / "water-constants.bin");
    check(frame_bytes.size() == count * 592 && fog_bytes.size() == count * 32 &&
              water_bytes.size() == count * sizeof(WaterConstantBlock),
          "Original scene constants extent");
    std::vector<TerrainFrameState> native_land(count);
    std::vector<WaterConstantBlock> native_constants(count), water_constants;
    Writer cpp_land, cpp_fog;
    for (unsigned i = 0; i < count; ++i) {
        std::memcpy(native_land[i].constants.data(), frame_bytes.data() + i * 592, 592);
        std::memcpy(native_land[i].fog.data(), fog_bytes.data() + i * 32, 32);
        std::memcpy(&native_constants[i], water_bytes.data() + i * sizeof(WaterConstantBlock),
                    sizeof(WaterConstantBlock));
        check(fow.land_frames[i].constants == native_land[i].constants &&
                  fow.land_frames[i].fog == native_land[i].fog,
              "Connected original LAND frame differs");
        for (const auto &v : fow.land_frames[i].constants)
            for (float f : v)
                cpp_land.scalar(f);
        for (const auto &v : fow.land_frames[i].fog)
            for (float f : v)
                cpp_fog.scalar(f);
        view.overlay = fow.frame.overlays[i];
        water_constants.push_back(water.constants(i, settings, surface_frame, view));
        check(water_constants.back() == native_constants[i],
              "Connected original water overlay constants differ");
    }
    check(cpp_land.bytes == frame_bytes && cpp_fog.bytes == fog_bytes,
          "Connected LAND frame floating-point bit patterns differ");
    save(output / "land-frame.bin", cpp_land.bytes);
    save(output / "land-fog.bin", cpp_fog.bytes);
    water_validation::save(output / "water-constants.bin", water_constants);
    check(blend_validation::read(output / "water-constants.bin") == water_bytes,
          "Connected water frame floating-point bit patterns differ");
    std::unique_ptr<FowSceneTargets> owned_targets;
    if (!retained_targets)
        owned_targets = std::make_unique<FowSceneTargets>(context, original, shaders, reference);
    auto &targets = retained_targets ? *retained_targets : *owned_targets;
    auto &target = targets.gl;
    auto &native_target = targets.d3d9;
    auto &native = targets.fow;
    fow.render(target, [&](const GlTexture &mask) {
        target.begin();
        land_images.pixel[4] = mask.id();
        land_images.pixel[5] = fow.map.clouds()[0].id();
        for (unsigned v = 0; v < count; ++v)
            for (auto i : scene_land_indices(land, *world, v))
                land.draw(i, fow.land[v], land_images, 1280, 960);
        image("land-gl.f32", target.pixels());
        water_images.pixel[10] = mask.id();
        for (unsigned i = 0; i < count; ++i)
            water.draw(i, water_constants[i], water_images, 1280, 960);
        image("water-gl.f32", target.pixels());
        if (rivers) {
            for (const auto &constants : water_constants)
                rivers->draw_gl(constants, mask.id());
            image("river-gl.f32", target.pixels());
        }
    });
    image("scene-gl.f32", target.pixels());
    auto mask_rgba = fow.map.mask().read_mip_raw(0);
    for (std::size_t i = 0; i < mask_rgba.size(); i += 4)
        std::swap(mask_rgba[i], mask_rgba[i + 2]); // Render-target raw transfer is BGRA.
    image("mask-gl.rgba", mask_rgba);
    native.render(native_frame, false, [&](IDirect3DTexture9 *mask) {
        native_target.begin();
        for (unsigned v = 0; v < count; ++v)
            for (auto i : scene_land_indices(land, *world, v)) {
                const auto p =
                    world->terrain().land().patch(i / world->cache().rows, i % world->cache().rows);
                const LandGrid grid(p.source_samples_x(), p.source_samples_y(), 1);
                const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(), grid.indices());
                auto images = scene_land_images(land, original, i);
                images[4] = mask;
                images[5] = native.clouds[0].value;
                original.land.bind(mesh, 1, land.land_constants(i, land_material_frame(native_land[v])),
                                   images, original.heights.at(p.texture_table_slot()).value);
                // Upload the independently executed original wrapper registers.
                hr(native.device->SetVertexShaderConstantF(0, native_land[v].constants[0].data(), 35),
                   "Original complete LAND scene VS frame");
                hr(native.device->SetPixelShaderConstantF(0, native_land[v].constants[16].data(), 21),
                   "Original complete LAND scene PS frame");
                hr(native.device->SetPixelShaderConstantF(21, native_land[v].fog[0].data(), 2),
                   "Original complete LAND fog frame");
                original.land.submit(mesh);
            }
        image("land-d3d9.f32", native_target.pixels());
        native_water_images[10] = mask;
        hr(native_water.device->BeginScene(), "Original FOW scene water begin");
        for (const auto &draw : water.draws()) {
            native_water_images[7] = original.heights.at(draw.window).value;
            native_water.bind(3, native_constants[draw.view], native_water_images);
            native_water.draw(water.mesh(), draw.first_index, draw.triangle_count);
        }
        hr(native_water.device->EndScene(), "Original FOW scene water end");
        image("water-d3d9.f32", native_target.pixels());
        if (rivers) {
            hr(native_water.device->BeginScene(), "Original scene river begin");
            for (const auto &constants : native_constants)
                rivers->draw_native(constants, mask);
            hr(native_water.device->EndScene(), "Original scene river end");
            image("river-d3d9.f32", native_target.pixels());
        }
    });
    image("scene-d3d9.f32", native_target.pixels());
    image("mask-d3d9.rgba", native.pixels(true));
    const auto checked_mask = native.pixels(true);
    if (mask_rgba != checked_mask) {
        // Failure-only observations; no extra GPU readback on a passing frame.
        image("mask-d3d9-checked.rgba", checked_mask);
        auto repeated_gl = fow.map.mask().read_mip_raw(0);
        for (std::size_t i = 0; i < repeated_gl.size(); i += 4)
            std::swap(repeated_gl[i], repeated_gl[i + 2]);
        image("mask-gl-repeated.rgba", repeated_gl);
        image("mask-d3d9-repeated.rgba", native.pixels(true));
    }
    check(mask_rgba == checked_mask, "Connected FOW mask pixels differ");
    if (rivers) {
        // Diagnostic complete river draw with clear depth. This cannot replace
        // the composed scene or its depth/visibility acceptance checks.
        target.begin();
        for (const auto &c : water_constants)
            rivers->draw_gl(c, fow.map.mask().id());
        image("river-only-gl.f32", target.pixels());
        native_target.begin();
        hr(native_water.device->BeginScene(), "Original isolated river begin");
        for (const auto &c : native_constants)
            rivers->draw_native(c, native.mask.value);
        hr(native_water.device->EndScene(), "Original isolated river end");
        image("river-only-d3d9.f32", native_target.pixels());
        rivers->diagnose(context, target, native_target, water_constants, native_constants,
                         fow.map.mask().id(), native.mask.value, output);
    }
    blend_validation::text(
        output / "scope.txt",
        "Independent camera/height/overlay/FOW/LAND/water CPU outputs compared before GPU submission.\n"
        "Own caches, current maps, reflection targets, authored LUT/fog/cloud textures on each API.\n"
        "Original FOW mask -> LAND/water/optional river scene -> visibility depth -> cloud passes; shared "
        "D24S8.\n"
        "Explicit visibility, delta 0.125, quality 2, terrain options and empty reflection object scene.\n"
        "Full game scheduler, UI and full graphics parity remain incomplete.\n");
    std::cout << "PASS connected original/C++ FOW world producer chain and GPU capture\n" << std::flush;
    if (!land_probes.empty()) {
        // Prefix observations run only after complete, unmodified shader captures.
        // They retain each API's own mask/cloud/cache/height/LUT inputs and full
        // perspective geometry. No diagnostic output is used by production.
        const auto probes_output = output / "land-probes";
        fs::create_directory(probes_output);
        std::vector<std::uint32_t> observation_points;
        if (fs::is_regular_file(land_probes / "points.u32")) {
            const auto bytes = blend_validation::read(land_probes / "points.u32");
            check(!bytes.empty() && bytes.size() % 4 == 0 && bytes.size() <= 1280 * 960 * 4,
                  "Bounded LAND observation point array");
            observation_points.resize(bytes.size() / 4);
            std::memcpy(observation_points.data(), bytes.data(), bytes.size());
            check(std::all_of(observation_points.begin(), observation_points.end(),
                              [](auto p) { return p < 1280 * 960; }),
                  "LAND observation viewport");
            blend_validation::write(probes_output / "points.u32", bytes);
        }
        const auto save_observed = [&](const fs::path &file, const auto &pixels) {
            const std::span<const std::uint8_t> bytes(reinterpret_cast<const std::uint8_t *>(pixels.data()),
                                                      pixels.size() * sizeof(pixels[0]));
            check(bytes.size() == 1280 * 960 * 16, "Full LAND diagnostic readback");
            if (observation_points.empty()) {
                blend_validation::write(file, bytes);
            } else {
                Bytes selected;
                for (auto point : observation_points)
                    selected.insert(selected.end(), bytes.begin() + point * 16,
                                    bytes.begin() + point * 16 + 16);
                blend_validation::write(file, selected);
            }
        };
        GlRenderApi gl(*context);
        std::unique_ptr<WaterPrecisionTarget> observed;
        if (fs::is_regular_file(land_probes / "second-mrt.txt"))
            observed = std::make_unique<WaterPrecisionTarget>(*context, gl, target.fbo, native.device.value,
                                                              1280, 960);
        for (const auto &entry : fs::directory_iterator(land_probes)) {
            if (entry.path().extension() != ".bin")
                continue;
            const auto name = entry.path().stem().string();
            const auto code = blend_validation::read(entry.path());
            check(code.size() % 4 == 0, "LAND FOW prefix alignment");
            std::vector<DWORD> words(code.size() / 4);
            std::memcpy(words.data(), code.data(), code.size());
            Com<IDirect3DPixelShader9> native_probe;
            hr(native.device->CreatePixelShader(words.data(), &native_probe.value), "LAND FOW prefix PS");
            const auto source_bytes = blend_validation::read(land_probes / (name + ".glsl"));
            std::string source(source_bytes.begin(), source_bytes.end());
            source.erase(std::remove(source.begin(), source.end(), '\r'), source.end());
            const bool hardware_sample = source.starts_with("// FOW sampling diagnostic profile: hardware\n");
            std::string sampling(material_sampling_glsl());
            const std::string ordinary = "precise vec4 result=first*(1.0-(level-low))+second*(level-low);";
            std::string replacement_sampling;
            if (source.starts_with("// FOW sampling diagnostic profile: fma\n"))
                replacement_sampling = "precise vec4 firstTerm=first*(1.0-(level-low));precise vec4 "
                                       "result=fma(second,vec4(level-low),firstTerm);";
            if (source.starts_with("// FOW sampling diagnostic profile: double\n"))
                replacement_sampling = "precise dvec4 "
                                       "mixed=dvec4(first)*double(1.0-(level-low))+dvec4(second)*double("
                                       "level-low);vec4 result=vec4(mixed);";
            if (source.starts_with("// FOW sampling diagnostic profile: lrp\n"))
                replacement_sampling =
                    "precise vec4 delta=second-first;precise vec4 result=fma(delta,vec4(level-low),first);";
            if (!replacement_sampling.empty())
                sampling.replace(sampling.find(ordinary), ordinary.size(), replacement_sampling);
            const auto version_end = source.find('\n', source.find("#version")) + 1;
            source.insert(version_end, sampling);
            for (unsigned slot : {0u, 1u, 2u, 3u, 4u, 5u, 8u, 9u}) {
                if (hardware_sample)
                    continue;
                const auto token = "texture(s" + std::to_string(slot) + ",";
                const auto replacement = "nativeMaterialImplicit(s" + std::to_string(slot) + ",";
                for (auto at = source.find(token); at != std::string::npos;
                     at = source.find(token, at + replacement.size()))
                    source.replace(at, token.size(), replacement);
            }
            blend_validation::text(probes_output / (name + ".glsl"), source);
            blend_validation::write(probes_output / (name + ".bin"), code);
            std::map<std::pair<int, int>, std::unique_ptr<LandProbeProgram>> programs;
            target.begin();
            if (observed)
                observed->clear();
            land_images.pixel[4] = fow.map.mask().id();
            land_images.pixel[5] = fow.map.clouds()[0].id();
            for (unsigned v = 0; v < count; ++v)
                for (auto i : scene_land_indices(land, *world, v)) {
                    const auto p =
                        world->terrain().land().patch(i / world->cache().rows, i % world->cache().rows);
                    const auto key = std::pair{p.source_samples_x(), p.source_samples_y()};
                    if (!programs.contains(key)) {
                        const LandGrid grid(key.first, key.second, 1);
                        const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(),
                                                    grid.indices());
                        programs.emplace(key,
                                         std::make_unique<LandProbeProgram>(*context, mesh, source, true));
                    }
                    gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                    gl.DepthMask(GL_FALSE);
                    land.draw(i, fow.land[v], land_images, 1280, 960);
                    gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                    gl.DepthMask(GL_TRUE);
                    programs.at(key)->draw(land.land_constants(i, fow.land[v]), static_cast<float>(i));
                }
            if (observed) {
                blend_validation::write(probes_output / (name + "-gl.f32"), target.pixels());
                if (name.ends_with("-mrt"))
                    save_observed(probes_output / (name + "-observed-gl.f32"), observed->gl_output());
            } else {
                save_observed(probes_output / (name + "-gl.f32"), target.pixels());
            }
            native_target.begin();
            for (unsigned v = 0; v < count; ++v)
                for (auto i : scene_land_indices(land, *world, v)) {
                    const auto p =
                        world->terrain().land().patch(i / world->cache().rows, i % world->cache().rows);
                    const LandGrid grid(p.source_samples_x(), p.source_samples_y(), 1);
                    const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(), grid.indices());
                    auto images = scene_land_images(land, original, i);
                    images[4] = native.mask.value;
                    images[5] = native.clouds[0].value;
                    original.land.bind(mesh, 1, land.land_constants(i, land_material_frame(native_land[v])),
                                       images, original.heights.at(p.texture_table_slot()).value,
                                       native_probe.value);
                    const LandVector patch_index{static_cast<float>(i), static_cast<float>(i),
                                                 static_cast<float>(i), static_cast<float>(i)};
                    hr(native.device->SetPixelShaderConstantF(13, patch_index.data(), 1),
                       "Diagnostic LAND patch index");
                    original.land.submit(mesh);
                }
            if (observed) {
                blend_validation::write(probes_output / (name + "-d3d9.f32"), native_target.pixels());
                if (name.ends_with("-mrt"))
                    save_observed(probes_output / (name + "-observed-d3d9.f32"), observed->native_output());
            } else {
                save_observed(probes_output / (name + "-d3d9.f32"), native_target.pixels());
            }
            std::cout << "PASS diagnostic LAND FOW prefix capture " << name << '\n' << std::flush;
        }
    }
    const auto water_probes = land_probes / "water";
    if (!land_probes.empty() && fs::is_directory(water_probes)) {
        const auto probes_output = output / "water-probes";
        fs::create_directory(probes_output);
        GlRenderApi gl(*context);
        water_images.pixel[10] = fow.map.mask().id();
        native_water_images[10] = native.mask.value;
        std::unique_ptr<WaterPrecisionTarget> observed;
        if (fs::is_regular_file(water_probes / "second-mrt.txt"))
            observed = std::make_unique<WaterPrecisionTarget>(*context, gl, target.fbo,
                                                              native_water.device.value, 1280, 960);
        std::vector<std::uint32_t> observation_points;
        if (fs::is_regular_file(water_probes / "points.u32")) {
            const auto bytes = blend_validation::read(water_probes / "points.u32");
            check(!bytes.empty() && bytes.size() % 4 == 0 && bytes.size() <= 1280 * 960 * 4,
                  "Bounded water observation point array");
            observation_points.resize(bytes.size() / 4);
            std::memcpy(observation_points.data(), bytes.data(), bytes.size());
            check(std::all_of(observation_points.begin(), observation_points.end(),
                              [](auto p) { return p < 1280 * 960; }),
                  "Water observation viewport");
            blend_validation::write(probes_output / "points.u32", bytes);
        }
        const auto save_observed = [&](const fs::path &file, const std::vector<float> &pixels) {
            if (observation_points.empty()) {
                blend_validation::write(file, pixels);
            } else {
                std::vector<float> selected;
                for (auto point : observation_points)
                    selected.insert(selected.end(), pixels.begin() + point * 4,
                                    pixels.begin() + point * 4 + 4);
                blend_validation::write(file, selected);
            }
        };
        std::array<std::vector<std::uint8_t>, 2> primary_controls;
        const auto save_primary = [&](const std::string &name, unsigned api, const auto &pixels) {
            const std::span<const std::uint8_t> bytes(reinterpret_cast<const std::uint8_t *>(pixels.data()),
                                                      pixels.size() * sizeof(pixels[0]));
            const std::string backend = api ? "d3d9" : "gl";
            const auto file = probes_output / (name + "-" + backend + ".f32");
            if (name == "full")
                primary_controls[api].assign(bytes.begin(), bytes.end());
            else if (primary_controls[api].size() == bytes.size() &&
                     std::equal(primary_controls[api].begin(), primary_controls[api].end(), bytes.begin())) {
                // Deduplicate only after reading and comparing this actual GPU
                // output. Links stay within one API's diagnostic captures.
                std::error_code error;
                fs::create_hard_link(probes_output / ("full-" + backend + ".f32"), file, error);
                if (!error)
                    return;
            }
            blend_validation::write(file, bytes);
        };
        for (const auto &entry : fs::directory_iterator(water_probes)) {
            if (entry.path().extension() != ".bin")
                continue;
            const auto name = entry.path().stem().string();
            const auto code = blend_validation::read(entry.path());
            check(code.size() % 4 == 0, "Water FOW prefix alignment");
            std::vector<DWORD> words(code.size() / 4);
            std::memcpy(words.data(), code.data(), code.size());
            Com<IDirect3DPixelShader9> native_probe;
            hr(native.device->CreatePixelShader(words.data(), &native_probe.value), "Water FOW prefix PS");
            const auto source_bytes = blend_validation::read(water_probes / (name + ".glsl"));
            std::string source(source_bytes.begin(), source_bytes.end());
            source.erase(std::remove(source.begin(), source.end(), '\r'), source.end());
            WaterProbe probe(*context, water.mesh(), source);
            blend_validation::text(probes_output / (name + ".glsl"), source);
            blend_validation::write(probes_output / (name + ".bin"), code);
            // Water-only observations on a fresh clear target. Full production
            // LAND/water/cloud comparisons above remain mandatory and separate.
            target.begin();
            if (observed)
                observed->clear();
            for (unsigned v = 0; v < count; ++v) {
                gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                gl.DepthMask(GL_FALSE);
                water.draw(v, water_constants[v], water_images, 1280, 960);
                gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                gl.DepthMask(GL_TRUE);
                for (const auto &draw : water.draws()) {
                    if (draw.view != v)
                        continue;
                    context->api().BindTextureUnit(7, water.height_texture(draw.window).id());
                    probe.draw(water_constants[v], 1280, 960, draw.first_index, draw.triangle_count);
                }
            }
            save_primary(name, 0, target.pixels());
            if (observed && name != "full")
                save_observed(probes_output / (name + "-observed-gl.f32"), observed->gl_output());
            native_target.begin();
            hr(native_water.device->BeginScene(), "Original FOW water observation begin");
            for (const auto &draw : water.draws()) {
                native_water_images[7] = original.heights.at(draw.window).value;
                native_water.bind(3, native_constants[draw.view], native_water_images);
                hr(native_water.device->SetPixelShader(native_probe.value), "Original FOW water prefix");
                native_water.draw(water.mesh(), draw.first_index, draw.triangle_count);
            }
            hr(native_water.device->EndScene(), "Original FOW water observation end");
            save_primary(name, 1, native_target.pixels());
            if (observed && name != "full")
                save_observed(probes_output / (name + "-observed-d3d9.f32"), observed->native_output());
            std::cout << "PASS diagnostic water FOW prefix capture " << name << '\n' << std::flush;
        }
    }
    scene_land_finish(land);
}
template <class LandRenderer>
void render_fow_world_camera(std::shared_ptr<GlContext> context, std::shared_ptr<const MaterialWorld> world,
                             LandRenderer &land, NativeWorld &original, GlWaterWorldRenderer &water,
                             water_validation::NativeWater &native_water,
                             const WaterSurfaceSettings &settings, WaterSurfaceFrame surface_frame,
                             WaterSurfaceView view, GlLandMaterialInputs land_images,
                             GlWaterSurfaceInputs water_images,
                             std::array<IDirect3DBaseTexture9 *, 14> native_water_images,
                             const GlDistanceFog &fog, const fs::path &root, const fs::path &shaders,
                             const fs::path &reference, const fs::path &output,
                             RiverWorldScene *rivers = nullptr) {
    check(!fs::exists(output), "Fresh FOW camera checkpoints required");
    fs::create_directory(output);
    const auto camera_output = output / "camera-frame";
    fs::create_directory(camera_output);
    fs::create_directory(output / "water-geometry");
    const auto lighting = terrain_lighting_defaults(X87Precision::double_precision);
    const auto &domain = water.domain();
    const WaterReflectionFrameSettings frame_settings{
        lighting.angles, lighting.shadow, {domain.scales[0], domain.scales[1]}};
    const auto rcp = [](float v) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(v))); };
    const auto rsqrt = [](float v) { return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v))); };
    camera_controller_storage::Bytes prior{};
    constexpr std::uint32_t original_vtable = 0xb712a8;
    constexpr std::int32_t orientation = 1;
    std::memcpy(prior.data() + 4, &original_vtable, 4);
    std::memcpy(prior.data() + 0x10c, &orientation, 4);
    const auto controller = water.prepare_camera_startup(
        {1280, 960, 11}, camera_controller_storage::prior(prior), frame_settings, rcp, rsqrt);
    constexpr std::array<unsigned, 7> checkpoints{0, 180, 188, 261, 275, 292, 322};
    unsigned captured = 0;
    run_camera_input_scene(
        water, controller, prior, frame_settings, rcp, rsqrt, camera_output, nullptr,
        [&](unsigned step, float zoom) {
            if (std::find(checkpoints.begin(), checkpoints.end(), step) == checkpoints.end())
                return;
            const auto geometry = output / "water-geometry" / std::to_string(step);
            fs::create_directory(geometry);
            water_validation::save(geometry / "vertices.bin", water.mesh().vertices);
            water_validation::save(geometry / "indices.bin", water.mesh().indices);
            water_validation::save(geometry / "counts.bin", water.mesh().counts);
            water_validation::save(geometry / "prefixes.bin", water.mesh().prefixes);
            Writer draws;
            for (const auto &draw : water.draws()) {
                draws.word(draw.view);
                draws.word(draw.window);
                draws.word(draw.selector);
                draws.word(draw.first_index);
                draws.word(draw.triangle_count);
            }
            save(geometry / "draws.bin", draws.bytes);
            for (const auto *file :
                 {"vertices.bin", "indices.bin", "counts.bin", "prefixes.bin", "draws.bin"})
                check(blend_validation::read(geometry / file) ==
                          blend_validation::read(reference / "water-geometry" / std::to_string(step) / file),
                      "Original/C++ camera water geometry or draw ranges differ before GPU submission");
            const auto globals = fog.frame(zoom);
            surface_frame.fog_color = globals.color;
            surface_frame.fog_parameters = globals.parameters;
            render_fow_world_scene(context, world, land, original, water, native_water, settings,
                                   surface_frame, view, land_images, water_images, native_water_images, fog,
                                   zoom, root, shaders, reference / std::to_string(step),
                                   output / std::to_string(step), {}, nullptr, {}, rivers);
            ++captured;
            std::cout << "PASS connected FOW camera checkpoint " << step
                      << "/322; views=" << water.camera_state().view_count << '\n'
                      << std::flush;
        });
    check(captured == checkpoints.size(), "All declared FOW camera checkpoints executed");
    blend_validation::text(
        output / "scope.txt",
        "323 retained camera input ticks; seven complete FOW/LAND/water/cloud checkpoints.\n"
        "Pan, wrap, yaw, pointer drag and zoom are primitive inputs; original camera outputs never enter "
        "C++.\n"
        "Each checkpoint starts a fresh FOW state/map and scene target with explicit fixed visibility.\n"
        "Each API retains its own world caches, current and empty-object reflection target.\n"
        "Full visibility scheduler, combined camera/time animation and resource lifecycle remain "
        "incomplete.\n");
}

template <class LandRenderer>
void render_fow_world_animation(
    std::shared_ptr<GlContext> context, std::shared_ptr<const MaterialWorld> world, LandRenderer &land,
    NativeWorld &original, GlWaterWorldRenderer &water, water_validation::NativeWater &native_water,
    WaterSurfaceSettings settings, const WaterSurfaceFrame &surface_frame, WaterSurfaceView view,
    GlLandMaterialInputs land_images, GlWaterSurfaceInputs water_images,
    std::array<IDirect3DBaseTexture9 *, 14> native_water_images, const GlDistanceFog &fog, float zoom,
    const fs::path &root, const fs::path &shaders, const fs::path &scene_reference, const fs::path &reference,
    const fs::path &output, const fs::path &land_probes = {}, RiverWorldScene *rivers = nullptr) {
    check(!fs::exists(output), "Fresh connected FOW animation output required");
    const auto expected_states = blend_validation::read(reference / "states.bin");
    const auto expected_deltas = blend_validation::read(reference / "tick-deltas.u32");
    FogOfWarScene fow(context, world->terrain(), root);
    const auto snapshot_bytes = 40ull + 12ull * fow.state.width() * fow.state.height();
    check(expected_states.size() == 241 * snapshot_bytes && expected_deltas.size() == 241 * 4,
          "Complete original FOW animation state/delta extent");
    fs::create_directory(output);
    WaterFrameClock clock;
    Writer states, deltas, events;
    for (unsigned tick = 0; tick <= 240; ++tick) {
        clock.tick(double(tick) / 60.);
        settings = advance_water_surface(settings, clock.delta);
        const auto delta = static_cast<float>(clock.delta);
        deltas.scalar(delta);
        check(std::memcmp(expected_deltas.data() + tick * 4, &delta, 4) == 0,
              "Original clock delta differs before FOW state tick");
        if (tick == 60 || tick == 180)
            for (int x = 10; x < 20; ++x)
                for (int y = 8; y < 15; ++y) {
                    const auto code = static_cast<std::uint8_t>(tick == 60 ? 2 : 1);
                    const auto cube_x = x - (y >> 1);
                    fow.state.visibility_event({cube_x, y, -cube_x - y}, code);
                    events.word(tick);
                    events.word(static_cast<std::uint32_t>(x));
                    events.word(static_cast<std::uint32_t>(y));
                    events.word(code);
                }
        fow.state.tick(delta);
        Writer snapshot;
        snapshot.word(static_cast<std::uint32_t>(fow.state.width()));
        snapshot.word(static_cast<std::uint32_t>(fow.state.height()));
        snapshot.word(1);
        snapshot.word(static_cast<std::uint32_t>(fow.state.dirty()));
        snapshot.scalar(fow.state.elapsed());
        snapshot.scalar(fow.state.normalized_scale());
        for (float v : fow.state.frame_scale())
            snapshot.scalar(v);
        for (const auto *values :
             {&fow.state.visibility(), &fow.state.visibility_override(), &fow.state.culling()})
            for (auto v : *values)
                snapshot.word(v);
        check(snapshot.bytes.size() == snapshot_bytes &&
                  std::equal(snapshot.bytes.begin(), snapshot.bytes.end(),
                             expected_states.begin() + tick * snapshot_bytes),
              "Connected original/C++ FOW animation state differs");
        states.bytes.insert(states.bytes.end(), snapshot.bytes.begin(), snapshot.bytes.end());
        if (tick % 60 == 0) {
            render_fow_world_scene(context, world, land, original, water, native_water, settings,
                                   surface_frame, view, land_images, water_images, native_water_images, fog,
                                   zoom, root, shaders, reference / std::to_string(tick),
                                   output / std::to_string(tick), tick >= 180 ? land_probes : fs::path{},
                                   &fow, scene_reference, rivers);
            std::cout << "PASS connected FOW animation checkpoint " << tick << "/240\n" << std::flush;
        }
    }
    save(output / "states.bin", states.bytes);
    save(output / "tick-deltas.u32", deltas.bytes);
    save(output / "visibility-events.bin", events.bytes);
    blend_validation::text(
        output / "scope.txt",
        "241 independent C++ clock/state ticks; explicit visibility events at 60/180; five full scene "
        "captures.\n"
        "Original retained state/arena, LAND and water frame outputs compared before each GPU capture.\n"
        "C++ FOW state/resources persist; native GPU helpers and scene targets are recreated per "
        "checkpoint.\n"
        "Camera/world remain stationary; full scheduler/resource lifecycle and graphics parity are "
        "incomplete.\n");
}
} // namespace
