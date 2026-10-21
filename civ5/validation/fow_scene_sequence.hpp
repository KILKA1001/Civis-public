#pragma once
#include <bcrypt.h>
#include <iomanip>

namespace {
std::string sequence_sha(std::span<const std::uint8_t> bytes) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE state = nullptr;
    std::array<std::uint8_t, 32> result{};
    try {
        check(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0,
              "Sequence SHA provider");
        check(BCryptCreateHash(algorithm, &state, nullptr, 0, nullptr, 0, 0) >= 0, "Sequence SHA state");
        check(bytes.size() <= UINT32_MAX && BCryptHashData(state, const_cast<PUCHAR>(bytes.data()),
                                                           static_cast<ULONG>(bytes.size()), 0) >= 0,
              "Sequence SHA input");
        check(BCryptFinishHash(state, result.data(), 32, 0) >= 0, "Sequence SHA output");
    } catch (...) {
        if (state)
            BCryptDestroyHash(state);
        if (algorithm)
            BCryptCloseAlgorithmProvider(algorithm, 0);
        throw;
    }
    BCryptDestroyHash(state);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    std::ostringstream text;
    for (auto byte : result)
        text << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
    return text.str();
}

struct FowSequenceImages {
    std::map<std::string, Bytes> images;
    void take(const char *name, std::span<const std::uint8_t> bytes) {
        check(!images.contains(name), "Duplicate sequence image");
        images.emplace(name, Bytes(bytes.begin(), bytes.end()));
    }
    void retain_failure(const fs::path &output, unsigned step) const {
        std::ostringstream report;
        report << "{\"schema\":\"civ5-incomplete-scene-images-v1\",\"step\":" << step
               << ",\"passed\":false,\"files\":{";
        bool first = true;
        for (const auto &[name, bytes] : images) {
            save(output / name, bytes);
            report << (first ? "" : ",") << '"' << name << "\":{\"bytes\":" << bytes.size()
                   << ",\"sha256\":\"" << sequence_sha(bytes) << "\"}";
            first = false;
        }
        report << "}}\n";
        blend_validation::text(output / "failed-images.json", report.str());
    }
    bool finish(const fs::path &output, unsigned step, bool retain_all = false) const {
        constexpr std::size_t pixels = 1280 * 960;
        check(images.size() == 12, "Complete sequence pass image inventory");
        std::ostringstream report;
        report << std::setprecision(17) << "{\"schema\":\"civ5-combined-scene-images-v1\",\"step\":" << step
               << ",\"pixels\":" << pixels << ",\"tolerance\":1e-5,\"files\":{";
        bool first = true;
        for (const auto &[name, bytes] : images) {
            check(bytes.size() == (name.starts_with("mask-") ? 1024 * 1024 * 4 : pixels * 16),
                  "Complete sequence image extent");
            report << (first ? "" : ",") << '"' << name << "\":{\"bytes\":" << bytes.size()
                   << ",\"sha256\":\"" << sequence_sha(bytes) << "\"}";
            first = false;
        }
        bool passed = images.at("mask-gl.rgba") == images.at("mask-d3d9.rgba");
        report << "},\"maskExact\":" << (passed ? "true" : "false") << ",\"pairs\":{";
        std::array<std::vector<bool>, 2> river_footprint, isolated;
        auto word = [](const Bytes &bytes, std::size_t index) {
            std::uint32_t result;
            std::memcpy(&result, bytes.data() + index * 4, 4);
            return result;
        };
        for (unsigned api = 0; api < 2; ++api) {
            const std::string suffix = api ? "-d3d9.f32" : "-gl.f32";
            const auto &water = images.at("water" + suffix), &river = images.at("river" + suffix),
                       &alone = images.at("river-only" + suffix);
            river_footprint[api].resize(pixels);
            isolated[api].resize(pixels);
            for (std::size_t i = 0; i < pixels; ++i)
                for (unsigned c = 0; c < 4; ++c) {
                    const auto at = i * 4 + c;
                    if (word(water, at) != word(river, at))
                        river_footprint[api][i] = true;
                    if (word(alone, at))
                        isolated[api][i] = true;
                }
        }
        const bool footprint_exact = river_footprint[0] == river_footprint[1] && isolated[0] == isolated[1];
        passed = passed && footprint_exact;
        first = true;
        for (const std::string stage : {"land", "water", "river", "scene", "river-only"}) {
            const auto &a = images.at(stage + "-gl.f32"), &b = images.at(stage + "-d3d9.f32");
            double maximum = 0;
            std::size_t differing = 0, alpha = 0, outside_alpha = 0;
            bool finite = true;
            for (std::size_t i = 0; i < pixels * 4; ++i) {
                const auto aw = word(a, i), bw = word(b, i);
                const float av = std::bit_cast<float>(aw), bv = std::bit_cast<float>(bw);
                finite = finite && std::isfinite(av) && std::isfinite(bv);
                maximum = std::max(maximum, std::abs(double(av) - double(bv)));
                differing += aw != bw;
                if (i % 4 == 3 && aw != bw) {
                    ++alpha;
                    outside_alpha += !river_footprint[0][i / 4];
                }
            }
            const bool exact_alpha = stage == "land" || stage == "water";
            const bool outside = stage == "river" || stage == "scene";
            const bool pair_passed =
                finite && maximum <= 1e-5 && (!exact_alpha || !alpha) && (!outside || !outside_alpha);
            passed = passed && pair_passed;
            report << (first ? "" : ",") << '"' << stage << "\":{\"maximum\":" << maximum
                   << ",\"differingComponents\":" << differing << ",\"alphaDifferences\":" << alpha
                   << ",\"outsideRiverAlphaDifferences\":" << outside_alpha
                   << ",\"finite\":" << (finite ? "true" : "false")
                   << ",\"passed\":" << (pair_passed ? "true" : "false") << '}';
            first = false;
        }
        std::size_t visible = 0, hidden = 0;
        for (std::size_t i = 0; i < pixels; ++i) {
            passed = passed && (!river_footprint[0][i] || isolated[0][i]);
            visible += river_footprint[0][i];
            hidden += isolated[0][i] && !river_footprint[0][i];
        }
        report << "},\"footprintsExact\":" << (footprint_exact ? "true" : "false")
               << ",\"riverPixels\":" << visible << ",\"hiddenRiverPixels\":" << hidden
               << ",\"passed\":" << (passed ? "true" : "false") << "}\n";
        blend_validation::text(output / "images.json", report.str());
        const std::array<unsigned, 9> retained{0, 180, 188, 261, 275, 276, 292, 300, 322};
        for (const auto &[name, bytes] : images)
            if (!passed || retain_all ||
                (name.starts_with("scene-") &&
                 std::find(retained.begin(), retained.end(), step) != retained.end()))
                save(output / name, bytes);
        return passed;
    }
};

template <class LandRenderer>
void render_fow_scene_sequence(std::shared_ptr<GlContext> context, std::shared_ptr<const MaterialWorld> world,
                               LandRenderer &land, NativeWorld &original, GlWaterWorldRenderer &water,
                               water_validation::NativeWater &native_water, WaterSurfaceSettings settings,
                               WaterSurfaceFrame surface_frame, WaterSurfaceView view,
                               GlLandMaterialInputs land_images, GlWaterSurfaceInputs water_images,
                               std::array<IDirect3DBaseTexture9 *, 14> native_water_images,
                               const GlDistanceFog &fog, const fs::path &root, const fs::path &shaders,
                               const fs::path &reference, const fs::path &output, RiverWorldScene &rivers,
                               const fs::path &land_probes = {}) {
    check(!fs::exists(output), "Fresh combined scene sequence required");
    fs::create_directory(output);
    const auto expected_states = blend_validation::read(reference / "states.bin");
    const auto expected_deltas = blend_validation::read(reference / "tick-deltas.u32");
    const auto expected_clock = blend_validation::read(reference / "clock-states.bin");
    const auto ticks = expected_deltas.size() / 4;
    check(ticks > 0 && ticks <= 323 && expected_deltas.size() == ticks * 4 &&
              expected_clock.size() == 323 * 36,
          "Bounded complete original clock input");
    unsigned diagnostic_step = UINT32_MAX;
    if (!land_probes.empty()) {
        std::ifstream input(land_probes / "sequence-step.txt");
        check(bool(input >> diagnostic_step) && diagnostic_step < ticks,
              "Explicit bounded sequence diagnostic step required");
        std::string extra;
        check(!(input >> extra), "Single sequence diagnostic step required");
        blend_validation::text(output / "diagnostic-only.txt",
                               "CPU history retained; only GPU step " + std::to_string(diagnostic_step) +
                                   " rendered. This is not full-sequence acceptance.\n");
    }
    FogOfWarScene fow(context, world->terrain(), root);
    const auto snapshot_bytes = 40ull + 12ull * fow.state.width() * fow.state.height();
    check(expected_states.size() == ticks * snapshot_bytes, "Complete retained original visibility states");
    FowSceneTargets targets(context, original, shaders, reference / "0");
    const auto identity = [&] {
        std::vector<std::uint64_t> values{targets.gl.color,
                                          targets.gl.depth,
                                          targets.gl.fbo,
                                          fow.map.mask().id(),
                                          reinterpret_cast<std::uintptr_t>(targets.d3d9.color.value),
                                          reinterpret_cast<std::uintptr_t>(targets.d3d9.depth.value),
                                          reinterpret_cast<std::uintptr_t>(targets.fow.mask.value)};
        for (const auto &cloud : fow.map.clouds())
            values.push_back(cloud.id());
        for (const auto &cloud : targets.fow.clouds)
            values.push_back(reinterpret_cast<std::uintptr_t>(cloud.value));
        for (unsigned slot : {0u, 1u, 2u, 3u, 5u, 6u, 8u})
            values.push_back(rivers.gl.texture(slot).id());
        for (unsigned slot : {4u, 5u}) {
            values.push_back(water_images.pixel[slot]);
            values.push_back(reinterpret_cast<std::uintptr_t>(native_water_images[slot]));
        }
        for (const auto &shader : targets.fow.vs)
            values.push_back(reinterpret_cast<std::uintptr_t>(shader.value));
        for (const auto &shader : targets.fow.ps)
            values.push_back(reinterpret_cast<std::uintptr_t>(shader.value));
        return values;
    };
    const auto retained = identity();
    scene_land_reset(land);
    const auto lighting = terrain_lighting_defaults(X87Precision::double_precision);
    const WaterReflectionFrameSettings frame_settings{
        lighting.angles, lighting.shadow, {water.domain().scales[0], water.domain().scales[1]}};
    const auto rcp = [](float v) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(v))); };
    const auto rsqrt = [](float v) { return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v))); };
    camera_controller_storage::Bytes prior{};
    const std::uint32_t vtable = 0xb712a8, orientation = 1;
    std::memcpy(prior.data() + 4, &vtable, 4);
    std::memcpy(prior.data() + 0x10c, &orientation, 4);
    const auto controller = water.prepare_camera_startup(
        {1280, 960, 11}, camera_controller_storage::prior(prior), frame_settings, rcp, rsqrt);
    fs::create_directory(output / "camera-frame");
    WaterFrameClock clock;
    Writer states, deltas, clock_states, events;
    std::vector<std::uint64_t> resource_identities;
    unsigned completed = 0;
    run_camera_input_scene(
        water, controller, prior, frame_settings, rcp, rsqrt, output / "camera-frame", nullptr,
        [&](unsigned step, float zoom) {
            if (step >= ticks)
                return;
            clock.tick(double(step) / 60.);
            settings = advance_water_surface(settings, clock.delta);
            const auto delta = static_cast<float>(clock.delta);
            deltas.scalar(delta);
            check(std::memcmp(expected_deltas.data() + step * 4, &delta, 4) == 0,
                  "Original combined clock delta");
            Writer clock_frame;
            for (double v : {clock.elapsed, clock.delta, clock.previous}) {
                const auto bits = std::bit_cast<std::uint64_t>(v);
                clock_frame.word(static_cast<std::uint32_t>(bits));
                clock_frame.word(static_cast<std::uint32_t>(bits >> 32));
            }
            clock_frame.word(clock.ticks);
            clock_frame.scalar(settings.river_phase);
            clock_frame.scalar(settings.water_phase);
            check(std::equal(clock_frame.bytes.begin(), clock_frame.bytes.end(),
                             expected_clock.begin() + step * 36),
                  "Original combined timer and both water phases");
            clock_states.bytes.insert(clock_states.bytes.end(), clock_frame.bytes.begin(),
                                      clock_frame.bytes.end());
            if (step == 60 || step == 180 || step == 276 || step == 300)
                for (int x = 10; x < 20; ++x)
                    for (int y = 8; y < 15; ++y) {
                        const auto code = static_cast<std::uint8_t>(step == 60 || step == 300 ? 2
                                                                    : step == 180             ? 1
                                                                                              : 0);
                        const auto a = x - (y >> 1);
                        fow.state.visibility_event({a, y, -a - y}, code);
                        for (auto v : {step, unsigned(x), unsigned(y), unsigned(code)})
                            events.word(v);
                    }
            fow.state.tick(delta);
            Writer snapshot;
            for (auto v :
                 {unsigned(fow.state.width()), unsigned(fow.state.height()), 1u, unsigned(fow.state.dirty())})
                snapshot.word(v);
            snapshot.scalar(fow.state.elapsed());
            snapshot.scalar(fow.state.normalized_scale());
            for (auto v : fow.state.frame_scale())
                snapshot.scalar(v);
            for (const auto *values :
                 {&fow.state.visibility(), &fow.state.visibility_override(), &fow.state.culling()})
                for (auto v : *values)
                    snapshot.word(v);
            check(snapshot.bytes.size() == snapshot_bytes &&
                      std::equal(snapshot.bytes.begin(), snapshot.bytes.end(),
                                 expected_states.begin() + step * snapshot_bytes),
                  "Original combined retained visibility state");
            states.bytes.insert(states.bytes.end(), snapshot.bytes.begin(), snapshot.bytes.end());
            if (diagnostic_step != UINT32_MAX && step != diagnostic_step) {
                // Overlay storage is stateful even when the diagnostic omits GPU submissions.
                fow.prepare(water, fog, zoom);
                // Retain the real cache history even when only a later image
                // is requested for diagnostics.
                scene_land_prepare(land, water.camera_state(),
                                   output / std::to_string(step) / "scheduled-materials");
                scene_land_finish(land);
                return;
            }
            const auto folder = output / std::to_string(step);
            const auto source = reference / std::to_string(step);
            fs::create_directory(folder);
            fs::create_directory(folder / "water-geometry");
            water_validation::save(folder / "water-geometry/vertices.bin", water.mesh().vertices);
            water_validation::save(folder / "water-geometry/indices.bin", water.mesh().indices);
            water_validation::save(folder / "water-geometry/counts.bin", water.mesh().counts);
            water_validation::save(folder / "water-geometry/prefixes.bin", water.mesh().prefixes);
            Writer draws;
            for (const auto &draw : water.draws())
                for (auto v : {draw.view, draw.window, draw.selector, draw.first_index, draw.triangle_count})
                    draws.word(v);
            save(folder / "water-geometry/draws.bin", draws.bytes);
            for (const char *name :
                 {"vertices.bin", "indices.bin", "counts.bin", "prefixes.bin", "draws.bin"})
                check(blend_validation::read(folder / "water-geometry" / name) ==
                          blend_validation::read(source / "water-geometry" / name),
                      "Original combined water geometry before GPU submission");
            const auto globals = fog.frame(zoom);
            surface_frame.fog_color = globals.color;
            surface_frame.fog_parameters = globals.parameters;
            FowSequenceImages images;
            try {
            render_fow_world_scene(
                context, world, land, original, water, native_water, settings, surface_frame, view,
                land_images, water_images, native_water_images, fog, zoom, root, shaders, source, folder,
                land_probes, &fow, {}, &rivers, &targets,
                [&](const char *name, std::span<const std::uint8_t> bytes) { images.take(name, bytes); });
            } catch (...) {
                images.retain_failure(folder, step);
                throw;
            }
            const auto current_identity = identity();
            check(current_identity == retained, "Combined scene recreated owned resources");
            resource_identities.insert(resource_identities.end(), current_identity.begin(),
                                       current_identity.end());
            check(images.finish(folder, step, diagnostic_step != UINT32_MAX),
                  "Combined full scene images differ; all failed-frame pixels retained");
            ++completed;
            std::cout << "PASS combined retained scene " << step << '/' << ticks - 1 << '\n' << std::flush;
        },
        [&](auto events) { scene_land_camera_events(land, events); });
    check(completed == (diagnostic_step == UINT32_MAX ? ticks : 1), "Every declared combined frame rendered");
    save(output / "states.bin", states.bytes);
    save(output / "tick-deltas.u32", deltas.bytes);
    save(output / "clock-states.bin", clock_states.bytes);
    save(output / "visibility-events.bin", events.bytes);
    water_validation::save(output / "resource-identities.bin", resource_identities);
    blend_validation::text(output / "result.txt",
                           "PASS combined retained scene frames=" + std::to_string(completed) + "\n");
}
} // namespace
