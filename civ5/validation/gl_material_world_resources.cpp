// Validate the production world renderer against original D3D9 shaders and actual Reset.
#include "material_world_validation.hpp"
#include "native_material_reset.hpp"
#include <iomanip>

namespace {
struct MaterialWorldResourceCapture {
    Bytes gl_image, native_image, gl_mips, native_mips;
    double maximum_error = 0;
    std::size_t covered_pixels = 0;
};
Bytes float_bytes(std::span<const float> values) {
    return Bytes(reinterpret_cast<const std::uint8_t *>(values.data()),
                 reinterpret_cast<const std::uint8_t *>(values.data()) + values.size_bytes());
}
MaterialWorldResourceCapture capture_material_resource_world(std::shared_ptr<GlContext> context, const MaterialWorld &world,
                                       GlMaterialWorldRenderer &renderer, NativeWorld &native,
                                       GlLandMaterialInputs scene, const fs::path &folder) {
    fs::create_directory(folder);
    MaterialWorldResourceCapture result;
    const auto view = overview_frame();
    {
        SceneTarget target(context, 1280, 960);
        target.begin();
        for (std::size_t index = 0; index < world.cache().patches.size(); ++index)
            renderer.draw(index, view, scene, 1280, 960);
        result.gl_image = float_bytes(target.pixels());
        renderer.finish_frame();
    }
    {
        NativeSceneTarget target(native, 1280, 960);
        target.begin();
        for (std::size_t index = 0; index < world.cache().patches.size(); ++index) {
            const auto p = world.terrain().land().patch(index / world.cache().rows,
                                                       index % world.cache().rows);
            const LandGrid grid(p.source_samples_x(), p.source_samples_y(), 1);
            const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(), grid.indices());
            native.land.bind(mesh, 1, renderer.land_constants(index, view), native.images(index, false),
                             native.heights.at(p.texture_table_slot()).value);
            native.land.submit(mesh);
        }
        result.native_image = target.pixels();
    }
    save(folder / "scene-gl.f32", result.gl_image);
    save(folder / "scene-d3d9.f32", result.native_image);
    check(result.gl_image.size() == 1280 * 960 * 16 &&
              result.gl_image.size() == result.native_image.size(), "Complete extent image extent");
    for (std::size_t at = 0; at < result.gl_image.size(); at += 4) {
        float gl_value, native_value;
        std::memcpy(&gl_value, result.gl_image.data() + at, 4);
        std::memcpy(&native_value, result.native_image.data() + at, 4);
        check(std::isfinite(gl_value) && std::isfinite(native_value), "Nonfinite extent image");
        result.maximum_error = std::max(result.maximum_error,
                                         std::abs(double(gl_value) - double(native_value)));
        if (at % 16 == 12) {
            check(std::memcmp(result.gl_image.data() + at, result.native_image.data() + at, 4) == 0,
                  "Different extent image coverage/depth silhouette");
            check(gl_value == 0 || gl_value == 1, "Extent LAND alpha must be zero or one");
            result.covered_pixels += gl_value == 1;
        }
    }
    check(result.covered_pixels > 800000, "Extent LAND world has insufficient coverage");
    // Complete original shaders are captured before the diagnostic mip reader.
    Writer gl_mips, native_mips;
    mip_validation::NativeMipReader reader(native.cache.device.value);
    for (std::size_t index = 0; index < world.cache().patches.size(); ++index) {
        check(renderer.final_texture(index).levels().size() == 7, "All seven generated GL mips required");
        check(native.finals.at(static_cast<unsigned>(index))->GetAutoGenFilterType() == D3DTEXF_LINEAR,
              "Native final must retain automatic linear mips");
        for (unsigned mip = 0; mip < 7; ++mip) {
            gl_mips.blob(renderer.final_texture(index).read_mip_raw(mip));
            native_mips.blob(float_bytes(reader.read(native.finals.at(static_cast<unsigned>(index)).value,
                                                    mip)));
        }
    }
    result.gl_mips = std::move(gl_mips.bytes);
    result.native_mips = std::move(native_mips.bytes);
    save(folder / "all-patch-mips-gl.bin", result.gl_mips);
    save(folder / "all-patch-mips-d3d9.f32", result.native_mips);
    std::ostringstream measurements;
    measurements << std::setprecision(17) << "{\"maximumError\":" << result.maximum_error
                 << ",\"coveredPixels\":" << result.covered_pixels << ",\"tolerance\":1e-5}\n";
    blend_validation::text(folder / "image-comparison.json", measurements.str());
    check(result.maximum_error <= 1e-5, "Complete regenerated LAND image exceeds unchanged tolerance");
    return result;
}
} // namespace

#include "material_world_scheduler_validation.hpp"
#include "material_world_camera_scheduler_validation.hpp"

int main(int argc, char **argv) {
    fs::path out;
    bool output_created = false;
    try {
        const bool camera_scheduler = argc == 7 && std::string_view(argv[5]) == "--scheduler-camera";
        check(argc == 5 || (argc == 6 && std::string_view(argv[5]) == "--scheduler") || camera_scheduler,
              "Usage: civ5_gl_material_world_resources asset-root primitive-input reference-base fresh-output "
              "[--scheduler | --scheduler-camera camera-input]");
        const fs::path root(argv[1]), base(argv[3]);
        out = argv[4];
        check(!fs::exists(out), "Fresh extent output required");
        fs::create_directories(out);
        output_created = true;
        auto input = primitive_input(argv[2]);
        auto world = build_world(root, input, 0);
        const auto count = world->cache().patches.size();
        check(count > 0, "Complete nonempty material world required");
        const std::array<fs::path, 4> shaders{
            base / "material-blend-original-shaders-1", base / "material-normal-original-shaders-1",
            base / "material-shadow-original-shaders-1", base / "material-lighting-original-shaders-1"};
        const auto geometry = read(base / "cache-raster-release-3/original-normal-geometry.bin");
        const auto cubes = cube_assets(root);
        auto context = std::make_shared<GlContext>();
        blend_validation::text(out / "gl-device.txt", context->description());
        Control control(context);
        const auto neutral = neutral_scene();
        GlLandMaterialInputs scene;
        for (unsigned i = 1; i < 10; ++i)
            scene.pixel[i] = control.image(neutral[i]);
        const auto lighting = terrain_lighting_defaults(X87Precision::double_precision);
        const auto light = GlMaterialWorldLighting::from_terrain(lighting, [](float squared) {
            return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
        });
        const auto color_key_assets = color_key_validation::load(root);
        auto standalone_color_keys = std::make_unique<GlColorKeys>(*context, color_key_assets);
        bool color_key_null_rejected = false;
        try {
            standalone_color_keys->rebuild_context(nullptr);
        } catch (const std::invalid_argument &) {
            color_key_null_rejected = true;
        }
        check(color_key_null_rejected, "Standalone ColorKey owner accepted null recovery context");
        bool color_key_same_rejected = false;
        try {
            standalone_color_keys->rebuild_context(context);
        } catch (const std::invalid_argument &) {
            color_key_same_rejected = true;
        }
        check(color_key_same_rejected, "Standalone ColorKey owner accepted its creating context");
        const auto color_key_bytes = standalone_color_keys->at(ColorKeySystem::terrain, 0).read_mip_raw(0);
        auto color_key_replacement = std::make_shared<GlContext>();
        standalone_color_keys->rebuild_context(color_key_replacement);
        check(standalone_color_keys->belongs_to(*color_key_replacement) &&
                  standalone_color_keys->at(ColorKeySystem::terrain, 0).read_mip_raw(0) == color_key_bytes,
              "Standalone ColorKey recovery changed authored bytes");
        color_key_replacement->check("Standalone ColorKey replacement context");
        auto color_keys = std::make_shared<const GlColorKeys>(*context, color_key_assets);
        GlMaterialWorldRenderer renderer(context, world, cubes, {false, true, true},
                                          LightingCubeSampling::measured_coordinates,
                                          LandShadowArithmetic::measured_fma_t_x_z_y,
                                          LandMipSampling::d3d9_rgb565, color_keys);
        const auto field = world->terrain().height_field();
        std::map<unsigned, Bytes> windows;
        for (int x = 0; x < field.domain().texture_columns(); ++x)
            for (int y = 0; y < field.domain().texture_rows(); ++y)
                windows.emplace(x + 16 * y, field.extract_a8_texture(x, y));
        NativeWorld native(shaders, base / "land-material-original-shaders-1", world->assets(), cubes, 64,
                             geometry, windows.begin()->second, renderer.source_bindings(),
                             base / "color-keys-original-1", world->terrain().land_styles().masks,
                             base / "terrain-frame-scene-original-4");
        auto *const original_device = native.cache.device.value;
        D3DDEVICE_CREATION_PARAMETERS creation{};
        hr(original_device->GetCreationParameters(&creation), "Extent device creation parameters");
        check(creation.DeviceType == D3DDEVTYPE_HAL &&
                  (creation.BehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING),
              "Extent original shaders require an actual HAL/HWVP device");
        D3DADAPTER_IDENTIFIER9 adapter{};
        hr(native.cache.api->GetAdapterIdentifier(creation.AdapterOrdinal, 0, &adapter),
           "Extent device adapter identity");
        blend_validation::text(out / "d3d9-device.txt", std::string(adapter.Description) + '\n' +
                                                          adapter.Driver + "\nHAL/HWVP\n");

        check(count == 1120, "Complete established world required");
        const GlMaterialWorldCacheExtents uniform{64, 64}, mixed{512, 256};
        const MaterialPatchSettings settings{64, 0, 0, 0, X87Precision::double_precision};
        const auto seed = [](int, int, float squared) {
            return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
        };
        if (camera_scheduler) {
            validate_material_world_camera_scheduler(root, argv[6], context, world, renderer, native,
                                                      windows, scene, light, seed, out);
            return 0;
        }
        const auto prepare = [&](std::string_view name, GlMaterialWorldCacheExtents extents,
                                  NativeWorld &reference, bool legacy) {
            Writer plans;
            for (std::size_t index = 0; index < count; ++index) {
                auto job = legacy ? renderer.prepare(index, settings, seed, seed, light, 4, true)
                                  : renderer.prepare(index, settings, extents, seed, seed, light, 4, true);
                reference.execute(index, job, windows.at(job.height_table_slot), 64);
                plans.blob(serialize_plan(job.plan));
                check(renderer.final_texture(index).levels().front().width == 64,
                      "Quality targets must not change the requested final patch extent");
                if (index % 56 == 0)
                    std::cout << "prepared extent profile " << name << " patch " << index << '/' << count
                              << '\n' << std::flush;
            }
            save(out / (std::string(name) + "-plans.bin"), plans.bytes);
            return plans.bytes;
        };
        const auto target_snapshot = [&](GlMaterialWorldCacheExtents extents, const fs::path &path) {
            std::vector<GLuint> ids;
            Writer records;
            for (unsigned role = 0; role < 13; ++role) {
                const auto &texture = renderer.temporary_texture(extents, static_cast<MaterialCacheRole>(role));
                const auto &level = texture.levels().front();
                const auto expected = role < 7 ? extents.temporary_size : extents.cache_size;
                check(level.width == expected && level.height == expected,
                      "Shared target allocation differs from original quality extents");
                check(context->api().IsTexture(texture.id()), "Shared target is not a live GL texture");
                ids.push_back(texture.id());
                for (auto value : {role, texture.id(), level.width, level.height,
                                   static_cast<unsigned>(texture.levels().size()), texture.plan().internal_format})
                    records.word(value);
            }
            if (!path.empty())
                save(path, records.bytes);
            return ids;
        };
        const auto source_snapshot = [&] {
            std::map<GLuint, Bytes> sources;
            for (std::size_t index = 0; index < count; ++index) {
                const auto &height = renderer.height_texture(index);
                if (!sources.contains(height.id()))
                    sources.emplace(height.id(), height.read_mip_raw(0));
            }
            for (unsigned system = 0; system < 5; ++system)
                for (unsigned style = 0; style < 5; ++style) {
                    const auto &volume = color_keys->at(static_cast<ColorKeySystem>(system), style);
                    sources.emplace(volume.id(), volume.read_mip_raw(0));
                }
            return sources;
        };
        const auto legacy_plans = prepare("legacy", uniform, native, true);
        const auto sources = source_snapshot();
        const auto uniform_targets = target_snapshot(uniform, out / "uniform-targets.bin");
        const auto legacy = capture_material_resource_world(context, *world, renderer, native, scene, out / "legacy");
        if (argc == 6) {
            validate_material_world_scheduler(context, *world, renderer, native, windows, scene, light, seed,
                                                legacy, legacy_plans, out);
            return 0;
        }
        const auto explicit_plans = prepare("explicit-uniform", uniform, native, false);
        check(explicit_plans == legacy_plans, "Explicit equal extents changed legacy CPU plans");
        check(target_snapshot(uniform, {}) == uniform_targets, "Equal extents did not reuse the existing target set");
        const auto explicit_uniform = capture_material_resource_world(context, *world, renderer, native, scene,
                                                            out / "explicit-uniform");
        check(explicit_uniform.gl_image == legacy.gl_image && explicit_uniform.native_image == legacy.native_image &&
                  explicit_uniform.gl_mips == legacy.gl_mips && explicit_uniform.native_mips == legacy.native_mips,
              "Explicit equal extents changed legacy full image or any patch mip");
        std::cout << "PASS legacy and explicit equal extents, all patches and mips\n" << std::flush;
        NativeWorld mixed_native(shaders, base / "land-material-original-shaders-1", world->assets(), cubes,
                                   mixed.temporary_size, geometry, windows.begin()->second,
                                   renderer.source_bindings(), base / "color-keys-original-1",
                                   world->terrain().land_styles().masks, base / "terrain-frame-scene-original-4");
        const auto mixed_plans = prepare("mixed", mixed, mixed_native, false);
        check(mixed_plans != legacy_plans, "Different temporary extents must affect original pass constants");
        check(source_snapshot() == sources, "Changing quality extents replaced source owners or bytes");
        check(target_snapshot(uniform, {}) == uniform_targets, "A different extent pair corrupted the old target set");
        const auto mixed_targets = target_snapshot(mixed, out / "mixed-targets.bin");
        for (auto id : mixed_targets)
            check(std::find(uniform_targets.begin(), uniform_targets.end(), id) == uniform_targets.end(),
                  "Different live quality target sets alias one another");
        const auto mixed_capture = capture_material_resource_world(context, *world, renderer, mixed_native, scene,
                                                          out / "mixed");
        check(mixed_capture.gl_image != legacy.gl_image && mixed_capture.native_image != legacy.native_image,
              "Quality change must have an actual image effect in both APIs");
        const auto old_final = renderer.final_texture(0).id();
        const auto old_base = renderer.final_texture(0).read_mip_raw(0);
        unsigned rejected = 0;
        for (const auto bad : {GlMaterialWorldCacheExtents{0, 256}, {512, 0}, {16385, 256}, {512, 16385}}) {
            bool caught = false;
            unsigned seed_calls = 0;
            const auto unexpected_seed = [&](int x, int y, float squared) {
                ++seed_calls;
                return seed(x, y, squared);
            };
            try {
                renderer.prepare(0, settings, bad, unexpected_seed, unexpected_seed, light, 4, true);
            } catch (const std::invalid_argument &) {
                caught = true;
            }
            check(caught && seed_calls == 0 && renderer.final_texture(0).id() == old_final &&
                      renderer.final_texture(0).read_mip_raw(0) == old_base &&
                      target_snapshot(mixed, {}) == mixed_targets && target_snapshot(uniform, {}) == uniform_targets,
                  "Invalid extent request changed an existing prepared world");
            ++rejected;
        }
        auto job = renderer.prepare(0, settings, mixed, seed, seed, light, 4, true);
        mixed_native.execute(0, job, windows.at(job.height_table_slot), 64);
        check(target_snapshot(mixed, {}) == mixed_targets, "Repeated mixed extent preparation allocated a new target set");
        check(source_snapshot() == sources, "Repeated quality preparation changed source ownership");
        const auto repeated = capture_material_resource_world(context, *world, renderer, mixed_native, scene,
                                                    out / "mixed-repeated");
        check(repeated.gl_image == mixed_capture.gl_image && repeated.native_image == mixed_capture.native_image &&
                  repeated.gl_mips == mixed_capture.gl_mips && repeated.native_mips == mixed_capture.native_mips,
              "Replacing one mixed-extent patch changed the full image or cache pyramids");

        // Invalidate both allocated quality sets, then regenerate the mixed
        // world on the same GL context and the same original D3D9 device.
        std::ostringstream resets;
        auto *const mixed_device = mixed_native.cache.device.value;
        for (unsigned round = 1; round <= 2; ++round) {
            std::vector<GLuint> generated;
            for (std::size_t index = 0; index < count; ++index)
                generated.push_back(renderer.final_texture(index).id());
            for (unsigned role = 0; role < 13; ++role) {
                generated.push_back(renderer.temporary_texture(mixed, static_cast<MaterialCacheRole>(role)).id());
                if (round == 1)
                    generated.push_back(renderer.temporary_texture(uniform, static_cast<MaterialCacheRole>(role)).id());
            }
            check(generated.size() == count + (round == 1 ? 26 : 13),
                  "Every live quality target set must be invalidated");
            for (auto id : generated)
                check(context->api().IsTexture(id), "Expected live generated texture before invalidation");
            renderer.invalidate_generated_resources();
            renderer.invalidate_generated_resources();
            for (std::size_t index = 0; index < count; ++index)
                check(!renderer.prepared(index), "Invalidation left a prepared patch");
            for (auto id : generated)
                check(!context->api().IsTexture(id), "A generated texture survived invalidation");
            for (const auto &[id, bytes] : sources) {
                (void)bytes;
                check(context->api().IsTexture(id), "Invalidation deleted a source texture");
            }
            check(source_snapshot() == sources, "Invalidation changed source identities or bytes");
            bool rejected_draw = false;
            try {
                renderer.draw(0, overview_frame(), scene, 1280, 960);
            } catch (const std::out_of_range &) {
                rejected_draw = true;
            }
            check(rejected_draw, "Unprepared world draw must be rejected");
            for (const auto extents : {uniform, mixed}) {
                bool rejected_target = false;
                try {
                    (void)renderer.temporary_texture(extents, MaterialCacheRole(0));
                } catch (const std::out_of_range &) {
                    rejected_target = true;
                }
                check(rejected_target, "Invalidated shared target remains published");
            }
            const unsigned width = round == 1 ? 64 : 96, height = round == 1 ? 64 : 80;
            std::cout << "calling actual mixed-world D3D9 Reset " << round << '\n' << std::flush;
            const auto reset = reset_native_material(mixed_native, width, height);
            check(mixed_native.cache.device.value == mixed_device, "Reset replaced the original device");
            const auto name = "mixed-after-" + std::to_string(round);
            check(prepare(name, mixed, mixed_native, false) == mixed_plans,
                  "Regeneration changed mixed-extent CPU plans");
            check(source_snapshot() == sources, "Regeneration changed source owners or bytes");
            target_snapshot(mixed, out / (name + "-targets.bin"));
            const auto after = capture_material_resource_world(context, *world, renderer, mixed_native,
                                                                 scene, out / name);
            check(after.gl_image == mixed_capture.gl_image && after.native_image == mixed_capture.native_image &&
                      after.gl_mips == mixed_capture.gl_mips && after.native_mips == mixed_capture.native_mips,
                  "Regenerated mixed world changed a full image or any cache mip");
            if (round > 1)
                resets << ',';
            resets << "{\"round\":" << round << ",\"width\":" << width << ",\"height\":" << height
                   << ",\"hresult\":" << reset.result << ",\"defaultTextures\":" << reset.textures
                   << ",\"defaultSurfaces\":" << reset.surfaces << ",\"surfaceAliases\":" << reset.aliases
                   << ",\"managedTextures\":" << reset.managed << ",\"glGeneratedTextures\":" << generated.size()
                   << ",\"imagesAndMipsBitExact\":true}";
            std::cout << "PASS mixed-world regeneration round " << round << '\n' << std::flush;
        }

        // Physical GL recovery is a different contract from logical cache
        // invalidation: all authored/source textures, LUTs and height windows
        // must be recreated on a different context. Release the validation
        // handle first so the renderer owns the old ColorKey textures alone.
        color_keys.reset();
        auto recovered_context = std::make_shared<GlContext>();
        renderer.rebuild_context(recovered_context, cubes, &color_key_assets);
        check(!renderer.prepared(0), "Context recovery retained a generated patch");
        check(renderer.height_texture(0).belongs_to(*recovered_context),
              "Context recovery retained a height texture from the old context");
        const auto recovered_plans = prepare("recovered", mixed, mixed_native, false);
        check(recovered_plans == mixed_plans, "Context recovery changed the material CPU/cache plans");
        Control recovered_control(recovered_context);
        GlLandMaterialInputs recovered_scene;
        for (unsigned i = 1; i < 10; ++i)
            recovered_scene.pixel[i] = recovered_control.image(neutral[i]);
        const auto recovered_capture = capture_material_resource_world(
            recovered_context, *world, renderer, mixed_native, recovered_scene, out / "recovered");
        check(recovered_capture.gl_image == mixed_capture.gl_image &&
                  recovered_capture.native_image == mixed_capture.native_image &&
                  recovered_capture.gl_mips == mixed_capture.gl_mips &&
                  recovered_capture.native_mips == mixed_capture.native_mips &&
                  renderer.final_texture(0).belongs_to(*recovered_context) &&
                  recovered_context->api().IsTexture(renderer.final_texture(0).id()),
              "Context recovery changed the full scene or did not publish a live replacement texture");
        renderer.invalidate_generated_resources();

        auto reentrant_context = std::make_shared<GlContext>();
        bool reentrant_rejected = false;
        const auto reentrant_seed = [&](int x, int y, float squared) {
            renderer.rebuild_context(reentrant_context, cubes, &color_key_assets);
            return seed(x, y, squared);
        };
        try {
            (void)renderer.prepare(0, settings, mixed, reentrant_seed, reentrant_seed, light, 4, true);
        } catch (const std::invalid_argument &error) {
            reentrant_rejected = std::string_view(error.what()) ==
                                 "Context replacement cannot interrupt active material work";
        }
        check(reentrant_rejected && !renderer.prepared(0),
              "Context recovery was not rejected from an active material callback");
        std::cout << "PASS physical GL context recovery and source reupload\n" << std::flush;

        std::ostringstream report;
        report << "{\"schema\":\"civ5-material-world-resources-gpu-v1\",\"passed\":true,"
                  "\"patches\":1120,\"temporarySize\":512,\"commonCacheSize\":256,\"finalPatchSize\":64,"
                  "\"legacyAndExplicitUniformBitExact\":true,\"mixedTargetReuseVerified\":true,"
                  "\"sourceOwnersAndBytesRetained\":true,\"mixedQualityHasImageEffect\":true,"
                  "\"invalidExtentGuards\":" << rejected << ",\"fullLandImageTolerance\":1e-5,"
                  "\"originalShadersModified\":false,\"fullCompositionCompared\":false,"
                  "\"higherCacheView5Executed\":false,\"productionModuleLinked\":true,"
                  "\"graphicsResetExecuted\":true,\"physicalGlRecoveryExecuted\":true,"
                  "\"physicalGlRecoverySceneBitExact\":true,\"reentrantRecoveryRejected\":true,"
                  "\"fullGraphicsParity\":false,\"resetRounds\":["
               << resets.str() << "]}\n";
        blend_validation::text(out / "result.json", report.str());
        std::cout << "PASS production material world extents and two actual Reset/regeneration rounds\n";
        return 0;
    } catch (const std::exception &error) {
        if (output_created)
            blend_validation::text(out / "failure.txt", std::string(error.what()) + '\n');
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
