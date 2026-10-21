#include "material_cache_chain_native.hpp"
#include "civ5/graphics/gl_material_cache.hpp"
#include "civ5/graphics/gl_material_textures.hpp"
#include "civ5/graphics/gl_lighting_cubes.hpp"
#include "civ5/graphics/gl_material_shadows.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/terrain/height_addressing.hpp"
#include <iostream>
#include <limits>
#include <sstream>
#include <set>
#include "terrain_decal_gpu_fixture.hpp"

using namespace cache_chain_validation;
namespace fs = std::filesystem;
#include "material_cache_chain_io.hpp"
namespace {
LightingCubeAssets load_cubes(const fs::path &root) {
    AssetEnvironment environment(root);
    const auto archive = [&](const char *name) {
        auto result = environment.open_fpk_archive(name);
        require(std::holds_alternative<std::shared_ptr<FpkSource>>(result),
                "Exact lighting archive unavailable");
        return std::get<std::shared_ptr<FpkSource>>(result);
    };
    const auto base = archive("Resource/DX9/TerrainTextures.fpk");
    const auto upgrade = archive("Resource/DX9/Upgrade1Textures.fpk");
    return LightingCubeAssets::load([&](const LightingSource &request) {
        const auto name = request.native_path.substr(request.native_path.find_last_of('\\') + 1);
        const auto found = (name.ends_with("_2.dds") ? upgrade : base)->find(AssetKey(name));
        const auto *handle = std::get_if<AssetHandle>(&found);
        require(handle != nullptr, "Exact chain cube unresolved");
        return *handle;
    });
}
Bytes source_height(std::span<const std::uint8_t> input) {
    std::vector<std::int32_t> common;
    int width = 0, height = 0;
    unsigned count = 0;
    for (const auto &record : generation_validation::records(input))
        if (record.op == 5) {
            generation_validation::Reader r{record.input};
            const generation_validation::Source source(r);
            if (count++ == 0) {
                width = source.width;
                height = source.height;
                common = source.heights;
            } else
                require(width == source.width && height == source.height && common == source.heights,
                        "Chain patches must share their source U16 field");
        }
    require(count == 3, "Three source patches required");
    std::vector<std::uint16_t> values;
    for (auto v : common) {
        require(v >= 0 && v <= 65535, "Chain source outside U16");
        values.push_back(static_cast<std::uint16_t>(v));
    }
    const civ5::terrain::HeightField field(civ5::terrain::HeightDomain(width, height), values);
    return field.extract_a8_texture(0, 0);
}
std::vector<float> explicit_lut() {
    std::vector<float> out(16 * 16 * 16 * 4);
    for (unsigned i = 0; i < out.size(); i += 4) {
        const auto x = i / 4 % 16, y = i / 64 % 16, z = i / 1024;
        out[i] = (x + y) / 30.f;
        out[i + 1] = std::sqrt(y / 15.f) * .8f + .1f;
        out[i + 2] = z * z / 225.f;
        out[i + 3] = 1;
    }
    return out;
}
struct Volume {
    std::shared_ptr<GlContext> context;
    GLuint id = 0;
    Volume(std::shared_ptr<GlContext> c, std::span<const float> values) : context(std::move(c)) {
        const auto &api = context->api();
        api.CreateTextures(GL_TEXTURE_3D, 1, &id);
        try {
            const auto storage =
                reinterpret_cast<PFNGLTEXTURESTORAGE3DPROC>(context->proc_address("glTextureStorage3D"));
            storage(id, 1, GL_RGBA32F, 16, 16, 16);
            api.TextureSubImage3D(id, 0, 0, 0, 0, 16, 16, 16, GL_RGBA, GL_FLOAT, values.data());
            api.TextureParameteri(id, GL_TEXTURE_MAX_LEVEL, 0);
            std::vector<float> actual(values.size());
            api.GetTextureImage(id, 0, GL_RGBA, GL_FLOAT, static_cast<GLsizei>(values.size_bytes()),
                                actual.data());
            context->check("Explicit cache ColorCorrect upload");
            require(std::memcmp(actual.data(), values.data(), values.size_bytes()) == 0,
                    "Chain LUT upload identity");
        } catch (...) {
            api.DeleteTextures(1, &id);
            throw;
        }
    }
    ~Volume() { context->api().DeleteTextures(1, &id); }
};
struct FloatTarget {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint texture = 0, fbo = 0;
    unsigned size;
    FloatTarget(std::shared_ptr<GlContext> c, unsigned width)
        : context(std::move(c)), gl(*context), size(width) {
        context->api().CreateTextures(GL_TEXTURE_2D, 1, &texture);
        gl.CreateFramebuffers(1, &fbo);
        context->api().TextureStorage2D(texture, 1, GL_RGBA32F, size, size);
        gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, texture, 0);
        const GLenum buffer = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(fbo, 1, &buffer);
        require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "Chain float FBO");
    }
    ~FloatTarget() {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &fbo);
        context->api().DeleteTextures(1, &texture);
    }
    void begin() {
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl.Disable(GL_BLEND);
        const float clear[4]{};
        gl.ClearNamedFramebufferfv(fbo, GL_COLOR, 0, clear);
    }
    Bytes pixels() {
        Bytes bytes(std::size_t(size) * size * 16);
        context->api().GetTextureImage(texture, 0, GL_RGBA, GL_FLOAT, static_cast<GLsizei>(bytes.size()),
                                       bytes.data());
        context->check("Chain float control readback");
        return bytes;
    }
};
MaterialCachePassSource bindings() {
    MaterialCachePassSource s{};
    for (unsigned i = 0; i < 13; ++i)
        s.textures[i] = 100 + i;
    s.normal_vertex_buffer = 11;
    s.normal_index_buffer = 12;
    s.lighting_vertex_buffer = 13;
    s.material_vertex_buffer = 14;
    s.patch_index_buffer = 15;
    s.shader_bindings = {29, 401, 402, 403};
    for (unsigned i = 0; i < 5; ++i) {
        s.lighting.sunlight[i] = 300 + 2 * i;
        s.lighting.shadowlight[i] = 301 + 2 * i;
        s.lighting.color_correct[i] = 400 + i;
    }
    return s;
}
struct Palette {
    const MaterialAssets &assets;
    std::vector<float> scales;
    std::vector<std::uint32_t> diffuse, height, specular;
    std::array<std::uint32_t, 64> flags{};
    explicit Palette(const MaterialAssets &a)
        : assets(a), diffuse(a.rows().size()), height(a.rows().size()),
          specular(a.specular_resources().begin(), a.specular_resources().end()) {
        for (const auto &r : a.rows())
            scales.push_back(r.scale);
        for (int i = 0; i < static_cast<int>(a.expansion().source_rows().size()); ++i) {
            const std::array selected{i};
            const auto resource = a.selected_resources(selected);
            const auto row = a.expansion().source_rows()[i];
            diffuse[row] = resource[0];
            height[row] = resource[8];
        }
    }
    MaterialCachePassPalette get(std::span<const std::int32_t> selected) const {
        return {selected,
                assets.expansion().source_rows(),
                assets.expansion().source_modes(),
                scales,
                flags,
                diffuse,
                specular,
                height,
                0.f,
                2070.375f};
    }
};
unsigned preflight_guards(GlMaterialCacheRenderer &renderer, const MaterialCachePassPlan &plan,
                          const std::vector<GlMaterialCacheTarget> &resources,
                          const std::vector<GlMaterialCacheVolume> &volumes,
                          const GlMaterialCacheFrame &frame, const GlTexture &height_target,
                          const GlTexture &final_target) {
    const auto before_height = height_target.read_mip_raw(0), before_final = final_target.read_mip_raw(0);
    unsigned guards = 0;
    const auto rejects = [&](const MaterialCachePassPlan &p, const auto &r, const auto &v,
                             const GlMaterialCacheFrame &f) {
        bool caught = false;
        try {
            renderer.execute(p, r, v, f, false);
        } catch (const std::invalid_argument &) {
            caught = true;
        }
        require(caught, "Missing complete-plan preflight rejection");
        require(height_target.read_mip_raw(0) == before_height &&
                    final_target.read_mip_raw(0) == before_final,
                "Rejected cache plan modified earlier or final targets");
        ++guards;
    };
    const auto mutate = [&](auto change) {
        auto copy = plan;
        change(copy);
        rejects(copy, resources, volumes, frame);
    };
    mutate([](auto &p) { p.passes.pop_back(); });
    mutate([](auto &p) { p.passes.back().kind = MaterialCachePassKind::compress; });
    mutate([](auto &p) { p.passes.back().draws[0].selector = 1; });
    mutate([](auto &p) { p.passes.back().draws[0].shader_binding ^= 1; });
    mutate([](auto &p) { p.passes.back().draws[0].packet = 0xffffffff; });
    mutate([](auto &p) { p.passes.back().draws[0].constant_offset = 0xffffffff; });
    mutate([](auto &p) { p.passes.back().draws[0].constant_bytes = 16; });
    mutate([](auto &p) { p.constants.resize(12); });
    mutate([](auto &p) { p.passes.back().targets[1] = p.passes.back().targets[0]; });
    mutate([](auto &p) { p.passes.back().mip = 1; });
    mutate([](auto &p) { p.passes.back().depth_state = 1; });
    mutate([](auto &p) { p.passes.back().blend_state = 2; });
    mutate([](auto &p) { p.packets[p.passes.back().draws[0].packet].geometry[4] = 1; });
    mutate([](auto &p) { p.packets[p.passes.back().draws[0].packet].pixel_textures[4] = 0; });
    mutate([](auto &p) { p.packets[p.passes.back().draws[0].packet].pixel_samplers[8] = 0; });
    mutate([](auto &p) { p.packets[p.passes.back().draws[0].packet].pixel_textures[0] = 0; });
    mutate([](auto &p) { p.normal_constants[4] = 8; });
    auto duplicate = resources;
    duplicate.push_back(resources.front());
    rejects(plan, duplicate, volumes, frame);
    auto missing = resources;
    std::erase_if(missing, [&](const auto &r) { return r.handle == plan.passes.back().targets[0]; });
    rejects(plan, missing, volumes, frame);
    auto unavailable = resources;
    unavailable.back().texture = nullptr;
    rejects(plan, unavailable, volumes, frame);
    rejects(plan, resources, std::vector<GlMaterialCacheVolume>{}, frame);
    auto invalid_volumes = volumes;
    invalid_volumes[0].texture = final_target.id();
    rejects(plan, resources, invalid_volumes, frame);
    auto bad_frame = frame;
    bad_frame.material_coefficients[0] = std::numeric_limits<float>::quiet_NaN();
    rejects(plan, resources, volumes, bad_frame);
    return guards;
}
} // namespace
#include "material_cache_decal_chain.hpp"
int main(int argc, char **argv) {
    try {
        require(argc == 9 || argc == 10, "Usage: cache_chain generation-input asset-root blend-shaders normal-shaders "
                           "shadow-shaders lighting-shaders original-geometry fresh-output [decal-shaders]");
        const fs::path out(argv[8]);
        require(!fs::exists(out), "Fresh cache chain output required");
        const auto input = read(argv[1]), geometry = read(argv[7]);
        require(is_generation_input(input) && geometry.size() == 152, "Chain primitive input/geometry");
        const auto patches = generation_validation::gpu_patches(input);
        auto assets = std::make_shared<const MaterialAssets>(authored(argv[2]));
        const Palette palette(*assets);
        const auto cube_assets = load_cubes(argv[2]);
        const auto a8 = source_height(input);
        const auto lut = explicit_lut();
        const auto source = bindings();
        auto context = std::make_shared<GlContext>();
        GlMaterialTextures material_textures(context, assets);
        GlLightingCubes cubes(*context, cube_assets);
        const auto height = GlTexture::alpha8(*context, 1024, 1024, a8);
        Volume volume(context, lut);
        GlMaterialCacheFrame frame{
            {}, {1, .866f, .2f}, {.75f, 1, .5f}, {.125f, -.25f, .75f}, {1.f / 1024, -1.f / 1024}};
        assets->write_constants(2070.375f, frame.material_coefficients);
        std::vector<GlMaterialCacheTarget> inputs;
        for (unsigned i = 0; i <= assets->textures().size(); ++i)
            inputs.push_back({i, &material_textures.resource(static_cast<std::uint16_t>(i))});
        inputs.push_back({200, &height});
        std::vector<GlMaterialCacheVolume> volumes;
        for (unsigned i = 0; i < 5; ++i) {
            inputs.push_back({source.lighting.sunlight[i], &cubes.sunlight(i)});
            inputs.push_back({source.lighting.shadowlight[i], &cubes.shadowlight(i)});
            volumes.push_back({static_cast<std::uint16_t>(source.lighting.color_correct[i]), volume.id});
        }
        fs::create_directories(out);
        if (argc == 10) {
            run_decal_chain(argv,patches,context,*assets,palette,cube_assets,frame,inputs,volumes,geometry,a8,lut);
            return 0;
        }
        write(out / "source-height-a8.bin", a8);
        write(out / "explicit-lut.f32", lut);
        write(out / "coefficients.f32", frame.material_coefficients);
        write(out / "original-normal-geometry.bin", geometry);
        for (unsigned i = 0; i < patches.size(); ++i) {
            const auto &p = patches[i];
            const auto prefix = "patch-" + std::to_string(i);
            write(out / (prefix + "-material48.bin"), p.patch.streams.material48);
            write(out / (prefix + "-normal24.bin"), p.patch.streams.normal24);
            write(
                out / (prefix + "-indices.bin"),
                {reinterpret_cast<const std::uint8_t *>(p.patch.indices.data()), p.patch.indices.size() * 2});
            const auto selected = p.selected.sources();
            write(out / (prefix + "-selected.bin"),
                  {reinterpret_cast<const std::uint8_t *>(selected.data()), selected.size_bytes()});
        }
        std::ostringstream execution;
        execution << context->description() << '\n';
        for (unsigned size : {64u, 128u, 256u, 512u}) {
            NativeCacheChain native({argv[3], argv[4], argv[5], argv[6]}, *assets, cube_assets, lut, size,
                                    geometry, a8, source, 200, 201, 202);
            GlMaterialCacheTextures cache(context, material_cache_texture_plan(size, size, false),
                                          {native.automatic[0], native.automatic[2], true});
            unsigned mip_count = 1;
            for (unsigned n = size; n > 1; n /= 2)
                ++mip_count;
            auto final =
                GlTexture::render_target(*context, size, size, mip_count, RenderTextureFormat::rgb565);
            auto higher =
                GlTexture::render_target(*context, size, size, mip_count, RenderTextureFormat::rgba8);
            auto resources = inputs;
            for (unsigned i = 0; i < 13; ++i)
                resources.push_back({source.textures[i], &cache.texture(static_cast<MaterialCacheRole>(i))});
            resources.push_back({201, &final});
            resources.push_back({202, &higher});
            for (unsigned test = 0; test < 12; ++test) {
                const auto &computed = patches[test % 3];
                const auto material = computed.patch.mesh();
                const LightingPatchMesh lighting(material.columns(), material.rows(),
                                                 computed.patch.streams.normal24, material.indices());
                const unsigned samples = std::array{0u, 1u, 4u, 32u}[test % 4];
                MaterialCachePassSettings settings{size,
                                                   size,
                                                   samples,
                                                   test % 2 ? 5u : 0u,
                                                   false,
                                                   false,
                                                   false,
                                                   false,
                                                   civ5::terrain::X87Precision::double_precision};
                MaterialCachePassPatch patch{static_cast<unsigned>(material.columns()),
                                             static_cast<unsigned>(material.rows()),
                                             {64, 64},
                                             1u << (test % 5),
                                             200,
                                             0,
                                             {202, 0},
                                             {201, 0},
                                             0,
                                             0,
                                             0};
                const Bytes entering(material_cache_constant_bytes(settings), test % 2 ? 0xa5 : 0);
                const auto plan = material_cache_pass_plan(
                    settings, patch, source, palette.get(computed.selected.sources()), entering, {});
                GlMaterialCacheRenderer renderer(context, material, lighting, source,
                                                 LightingCubeSampling::measured_coordinates);
                if (test == 0) {
                    auto replacement_context = std::make_shared<GlContext>();
                    bool rejected = false;
                    try {
                        renderer.rebuild_context(context);
                    } catch (const std::invalid_argument &) {
                        rejected = true;
                    }
                    require(rejected, "Cache renderer accepted its creating context during recovery");
                    renderer.rebuild_context(replacement_context);
                    replacement_context->check("Cache renderer replacement context");
                    renderer.rebuild_context(context);
                }
                const bool conditional = (test / 2) % 2 != 0;
                renderer.execute(plan, resources, volumes, frame, conditional);
                native.execute(plan, material, lighting, frame, conditional);
                const auto name = std::to_string(size) + "-" + std::to_string(test);
                write(out / (name + "-plan.bin"), serialize_plan(plan));
                for (unsigned slot : {0u, 1u, 4u, 3u}) {
                    write(out / (name + "-" + std::to_string(slot) + "-gl.bin"),
                          cache.texture(static_cast<MaterialCacheRole>(slot)).read_mip_raw(0));
                    write(out / (name + "-" + std::to_string(slot) + "-d3d9.bin"),
                          native.pixels(source.textures[slot], 0));
                }
                write(out / (name + "-final-gl.bin"), final.read_mip_raw(0));
                write(out / (name + "-final-d3d9.bin"), native.pixels(201, 0));
                if (settings.view == 5) {
                    write(out / (name + "-higher-gl.bin"), higher.read_mip_raw(0));
                    write(out / (name + "-higher-d3d9.bin"), native.pixels(202, 0));
                }
                if (test == 0) {
                    const auto checks =
                        preflight_guards(renderer, plan, resources, volumes, frame,
                                         cache.texture(MaterialCacheRole::height_temporary), final);
                    execution << "preflightGuards=" << checks << "; size=" << size << '\n';
                }
                // Lighting isolation: replace GL inputs only after the complete
                // direct results above were exported. Native inputs stay on GPU.
                for (unsigned slot : {0u, 4u, 3u}) {
                    const auto &texture = cache.texture(static_cast<MaterialCacheRole>(slot));
                    const auto bytes = native.pixels(source.textures[slot], 0);
                    context->api().TextureSubImage2D(texture.id(), 0, 0, 0, size, size,
                                                     texture.plan().transfer_format,
                                                     texture.plan().transfer_type, bytes.data());
                    require(texture.read_mip_raw(0) == bytes, "Identical lighting isolation inputs");
                    texture.generate_mip_chain();
                }
                auto light_only = plan;
                light_only.passes = {plan.passes.back()};
                write(out / (name + "-lighting-float-d3d9.bin"),
                      native.float_control(light_only, material, lighting, frame));
                {
                    GlMaterialCacheRaster raster(context);
                    const std::array<GlMaterialCacheTarget, 1> target{{{201, &final}}};
                    raster.begin(plan.passes.back(), target, conditional);
                    GlMaterialLightingRenderer light(context, lighting, false,
                                                     LightingCubeSampling::measured_coordinates);
                    const auto draw = [&] {
                        light.draw(cache.texture(MaterialCacheRole::normal_temporary).id(),
                                   cache.texture(MaterialCacheRole::render_temporary).id(),
                                   cache.texture(MaterialCacheRole::shadow_temporary).id(), 0,
                                   cubes.sunlight(test % 5), cubes.shadowlight(test % 5), volume.id, size,
                                   size, frame.half_vector);
                    };
                    draw();
                    write(out / (name + "-lighting-same-input-gl.bin"), final.read_mip_raw(0));
                    FloatTarget floating(context, size);
                    floating.begin();
                    draw();
                    write(out / (name + "-lighting-float-gl.bin"), floating.pixels());
                    raster.end();
                }
                // Separate controls run only after the complete direct results
                // have been read. The first original shadow sample retains the
                // full plan's count reciprocal; no height readback is reuploaded.
                if (samples) {
                    auto single = plan;
                    single.passes = {plan.passes[2]};
                    single.passes[0].draws.resize(1);
                    native.execute(single, material, lighting, frame, false);
                    write(out / (name + "-shadow-single-d3d9.bin"), native.pixels(source.textures[3], 0));
                    write(out / (name + "-shadow-float-d3d9.bin"),
                          native.float_control(single, material, lighting, frame));
                    GlMaterialShadowRenderer shadow(context, material);
                    GlMaterialCacheRaster raster(context);
                    const std::array<GlMaterialCacheTarget, 1> target{
                        {{source.textures[3], &cache.texture(MaterialCacheRole::shadow_temporary)}}};
                    raster.begin(single.passes[0], target, false);
                    const auto parameters = material_shadow_parameters(
                        frame.world_scales, frame.sun_direction, frame.height_texture_scale,
                        patch.world_rebase, {0, 0}, samples);
                    shadow.draw(height.id(), parameters, size, size);
                    write(out / (name + "-shadow-single-gl.bin"), target[0].texture->read_mip_raw(0));
                    FloatTarget floating(context, size);
                    floating.begin();
                    shadow.draw(height.id(), parameters, size, size);
                    write(out / (name + "-shadow-float-gl.bin"), floating.pixels());
                    raster.end();
                }
                if (size == 256 && test == 11) {
                    auto wrong = plan;
                    wrong.passes[2].draws.clear();
                    renderer.execute(wrong, resources, volumes, frame, conditional);
                    write(out / "missing-shadow-negative.bin", final.read_mip_raw(0));
                }
                execution << "PASS executed " << name << "; patch=" << test % 3 << "; shadow=" << samples
                          << "; style=" << test % 5 << "; view=" << settings.view
                          << "; conditional=" << conditional << '\n';
                std::cout << "executed " << name << '\n';
            }
        }
        text(out / "execution.txt", execution.str());
        std::cout << "PASS executed 48 complete ordinary cache plans\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
