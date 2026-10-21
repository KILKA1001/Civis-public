#include "civ5/graphics/gl_water_surface.hpp"
#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "water_surface_native.hpp"
#include <cmath>
#include <sstream>
#include <xmmintrin.h>

using namespace water_validation;
using namespace civ5::terrain;
namespace fs = std::filesystem;
#include "water_surface_probe.hpp"
#include "water_precision_target.hpp"
namespace {
void word(Bytes &bytes, unsigned at, unsigned value) {
    for (unsigned i = 0; i < 4; ++i)
        bytes.at(at + i) = std::uint8_t(value >> (i * 8));
}
DdsData control(unsigned width, unsigned height, unsigned format, std::span<const std::uint8_t> payload) {
    Bytes bytes(128);
    word(bytes, 0, 0x20534444);
    word(bytes, 4, 124);
    word(bytes, 8, 0x1007);
    word(bytes, 12, height);
    word(bytes, 16, width);
    word(bytes, 28, 1);
    word(bytes, 76, 32);
    word(bytes, 80, format == 28 ? 2 : 4);
    word(bytes, 84, format == 28 ? 0 : format);
    if (format == 28) {
        word(bytes, 88, 8);
        word(bytes, 104, 255);
    }
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return DdsData::parse(std::move(bytes), NumericDdsProfile::legacy_d3d9);
}
Bytes authored(const fs::path &root, const char *archive, const char *name) {
    const auto relative = std::string("Resource/") + archive;
    const auto opened = FpkSource::open(root / relative, relative);
    require(std::holds_alternative<std::shared_ptr<FpkSource>>(opened), "Original water FPK");
    const auto found = std::get<std::shared_ptr<FpkSource>>(opened)->find(AssetKey(name));
    require(std::holds_alternative<AssetHandle>(found), "Original water FPK resource");
    return std::get<AssetHandle>(found).read_bytes();
}
Bytes primitive(unsigned selector, const WaterSurfaceSettings &s, const WaterSurfaceFrame &f,
                const WaterSurfaceView &v, const Vector4 &bounds) {
    Bytes bytes{'C','5','W','G','P','U','0','1'};
    bytes.resize(32);
    const std::array<unsigned,6> words{12,12,0,53,1,selector};
    for(unsigned i=0;i<6;++i) word(bytes,8+i*4,words[i]);
    const auto append = [&](std::span<const float> values) {
        const auto *p=reinterpret_cast<const std::uint8_t *>(values.data());
        bytes.insert(bytes.end(),p,p+values.size_bytes());
    };
    append({&s.reflection_amount,1});append(s.specular_sun);append(s.specular_sky);
    append(s.sun_direction);append(s.sky_direction);append(s.fake_view_sun);append(s.fake_view_sky);
    for(const float value:{s.ocean_height,s.gradient_scale,s.frequency,s.river_phase,s.water_phase}) append({&value,1});
    append(f.projection);append(f.fog_color);append(f.fog_parameters);append({&f.shadow_fade,1});
    append(bounds);append(v.view);append(v.overlay);append(v.shadow);append(v.reflection);append(v.camera);append(v.prior_constants);
    return bytes;
}
Matrix4 identity() { return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; }
struct Control {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint fbo = 0, target = 0, lut = 0;
    explicit Control(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        auto &api = context->api();
        api.CreateTextures(GL_TEXTURE_2D, 1, &target);
        api.TextureStorage2D(target, 1, GL_RGBA32F, 64, 64);
        gl.CreateFramebuffers(1, &fbo);
        gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, target, 0);
        const GLenum buffer = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(fbo, 1, &buffer);
        require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Water FBO");
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl.Viewport(0, 0, 64, 64);
        for (auto flag : {GL_DEPTH_TEST, GL_BLEND, GL_CULL_FACE, GL_SCISSOR_TEST, GL_DITHER,
                          GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
            gl.Disable(flag);
    }
    ~Control() {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &fbo);
        context->api().DeleteTextures(1, &target);
        context->api().DeleteTextures(1, &lut);
    }
    void volume(std::span<const float> values, unsigned size) {
        auto &api = context->api();
        api.CreateTextures(GL_TEXTURE_3D, 1, &lut);
        const auto storage =
            reinterpret_cast<PFNGLTEXTURESTORAGE3DPROC>(context->proc_address("glTextureStorage3D"));
        storage(lut, 1, GL_RGBA32F, size, size, size);
        api.TextureSubImage3D(lut, 0, 0, 0, 0, size, size, size, GL_RGBA, GL_FLOAT, values.data());
        std::vector<float> got(values.size());
        api.GetTextureImage(lut, 0, GL_RGBA, GL_FLOAT, got.size() * 4, got.data());
        require(std::equal(got.begin(), got.end(), values.begin()), "GL LUT storage identity");
    }
    void begin() {
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        const float sentinel[]{187 / 255.f, 51 / 255.f, 119 / 255.f, 1};
        gl.ClearNamedFramebufferfv(fbo, GL_COLOR, 0, sentinel);
    }
    std::vector<float> end() {
        std::vector<float> result(64 * 64 * 4);
        context->api().GetTextureImage(target, 0, GL_RGBA, GL_FLOAT, result.size() * 4, result.data());
        context->check("Water GL output");
        return result;
    }
    // Read the currently bound production program, not a separately rebuilt
    // copy of its intended constants. Optimized-away arrays are explicit.
    void registers(const fs::path &out, const std::string &stem) {
        GLint program = 0;
        context->api().GetIntegerv(GL_CURRENT_PROGRAM, &program);
        require(program != 0, "Bound production water program");
        const auto get = reinterpret_cast<PFNGLGETUNIFORMFVPROC>(context->proc_address("glGetUniformfv"));
        for (const auto &[name, count] : {std::pair{"uV", 17u}, std::pair{"uP", 28u}}) {
            std::vector<float> values(count * 4);
            std::vector<unsigned> active(count);
            for (unsigned i = 0; i < count; ++i) {
                const auto key = std::string(name) + "[" + std::to_string(i) + "]";
                const auto location = gl.GetUniformLocation(program, key.c_str());
                active[i] = location >= 0;
                if (location >= 0)
                    get(program, location, values.data() + i * 4);
            }
            save(out / (stem + "-" + name + "-gl.bin"), values);
            save(out / (stem + "-" + name + "-active.bin"), active);
        }
        context->check("Actual water uniform readback");
    }
};
unsigned guards(Control &gpu, const WaterSurfaceMesh &mesh, const WaterConstantBlock &constants,
                const GlWaterSurfaceInputs &inputs) {
    unsigned checks = 0;
    const auto require_check = [&](bool ok, const char *why) {
        require(ok, why);
        ++checks;
    };
    const auto rejects = [&](auto action) {
        bool rejected = false;
        try {
            action();
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        require_check(rejected, "Missing water renderer input rejection");
        gpu.context->check("Water rejected input leaves GL clean");
    };
    rejects([&] { GlWaterSurfaceRenderer bad(nullptr, mesh, 3); });
    for (unsigned selector : {1u, 4u, 6u, 0xffffffffu})
        rejects([&] { GlWaterSurfaceRenderer bad(gpu.context, mesh, selector); });
    auto bad_mesh = mesh;
    bad_mesh.vertices[0][0] = std::numeric_limits<float>::infinity();
    rejects([&] { GlWaterSurfaceRenderer bad(gpu.context, bad_mesh, 3); });
    bad_mesh = mesh;
    bad_mesh.indices[0] = 0xffff;
    rejects([&] { GlWaterSurfaceRenderer bad(gpu.context, bad_mesh, 3); });
    bad_mesh = mesh;
    bad_mesh.indices.pop_back();
    rejects([&] { GlWaterSurfaceRenderer bad(gpu.context, bad_mesh, 3); });
    GlWaterSurfaceRenderer renderer(gpu.context, mesh, 3);
    const auto triangles = unsigned(mesh.indices.size() / 3);
    rejects([&] { renderer.draw(constants, inputs, 0, triangles, 0, 64); });
    rejects([&] { renderer.draw(constants, inputs, 0, triangles, 64, -1); });
    rejects([&] { renderer.draw(constants, inputs, 1, 1, 64, 64); });
    rejects([&] { renderer.draw(constants, inputs, unsigned(mesh.indices.size()), 1, 64, 64); });
    rejects([&] { renderer.draw(constants, inputs, 0, 0xffffffffu, 64, 64); });
    auto invalid = constants;
    invalid[0] = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { renderer.draw(invalid, inputs, 0, triangles, 64, 64); });
    rejects([&] { renderer.capture(invalid, false); });
    invalid = constants;
    invalid[44] = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { renderer.draw(invalid, inputs, 0, triangles, 64, 64); });
    rejects([&] { renderer.capture(invalid, false); });
    gpu.begin();
    for (unsigned slot : {0u, 1u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 13u}) {
        auto bad = inputs;
        bad.pixel[slot] = 0;
        rejects([&] { renderer.draw(constants, bad, 0, triangles, 64, 64); });
        bad.pixel[slot] = inputs.pixel[slot == 11 ? 0 : 11];
        rejects([&] { renderer.draw(constants, bad, 0, triangles, 64, 64); });
        if (slot != 11) {
            bad.pixel[slot] = gpu.target;
            rejects([&] { renderer.draw(constants, bad, 0, triangles, 64, 64); });
        }
    }
    auto bad = inputs;
    bad.pixel[6] = inputs.pixel[4];
    rejects([&] { renderer.draw(constants, bad, 0, triangles, 64, 64); });
    GLuint empty = 0;
    gpu.context->api().CreateTextures(GL_TEXTURE_2D, 1, &empty);
    bad = inputs;
    bad.pixel[0] = empty;
    rejects([&] { renderer.draw(constants, bad, 0, triangles, 64, 64); });
    gpu.context->api().DeleteTextures(1, &empty);
    gpu.begin();
    renderer.draw(constants, inputs, 0, triangles, 64, 64);
    const auto baseline = gpu.end();
    auto unused = constants;
    for (unsigned i : {32u, 33u, 34u, 35u, 40u, 41u, 42u, 43u, 45u,  46u,  47u, 49u,
                       50u, 51u, 53u, 54u, 55u, 59u, 63u, 87u, 137u, 138u, 139u})
        unused[i] = std::bit_cast<float>(0x7fc00000u + i);
    gpu.begin();
    renderer.draw(unused, inputs, 0, triangles, 64, 64);
    require_check(gpu.end() == baseline, "Untouched native constant components affected water pixels");
    auto no_unused = inputs;
    for (unsigned slot : {2u, 3u, 12u})
        no_unused.pixel[slot] = 0xffffffffu;
    gpu.begin();
    renderer.draw(constants, no_unused, 0, triangles, 64, 64);
    require_check(gpu.end() == baseline, "Unused water slots became requirements");
    GlWaterSurfaceRenderer diagnostic(gpu.context, mesh, 5);
    gpu.begin();
    diagnostic.draw(constants, {}, 0, triangles, 64, 64);
    ++checks;
    GlWaterSurfaceRenderer diffuse(gpu.context, mesh, 0);
    no_unused = inputs;
    no_unused.pixel[1] = no_unused.pixel[4] = 0;
    gpu.begin();
    diffuse.draw(constants, no_unused, 0, triangles, 64, 64);
    ++checks;
    GlWaterSurfaceRenderer no_quads(gpu.context, WaterSurfaceMesh{}, 3);
    no_quads.draw(constants, {}, 0, 0, 64, 64);
    require_check(no_quads.capture(constants, false).empty(), "Empty original water view must remain empty");
    GLuint previous = 0;
    gpu.gl.CreateTransformFeedbacks(1, &previous);
    gpu.gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, previous);
    for (bool discard : {false, true}) {
        if (discard)
            gpu.gl.Enable(GL_RASTERIZER_DISCARD);
        else
            gpu.gl.Disable(GL_RASTERIZER_DISCARD);
        renderer.capture(constants, true);
        GLint actual = 0;
        gpu.context->api().GetIntegerv(GL_TRANSFORM_FEEDBACK_BINDING, &actual);
        require_check(unsigned(actual) == previous &&
                          bool(gpu.gl.IsEnabled(GL_RASTERIZER_DISCARD)) == discard,
                      "Water capture failed to restore transform-feedback state");
    }
    gpu.gl.Disable(GL_RASTERIZER_DISCARD);
    gpu.gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
    gpu.gl.DeleteTransformFeedbacks(1, &previous);
    gpu.context->check("Water GPU guard cases complete");
    return checks;
}
unsigned pipeline_lifetime_checks(Control &gpu, const WaterSurfaceMesh &mesh,
                                  const WaterConstantBlock &constants, const GlWaterSurfaceInputs &inputs) {
    auto original = std::make_unique<GlWaterSurfaceRenderer>(gpu.context, mesh, 3);
    gpu.begin();
    original->draw(constants, inputs, 0, unsigned(mesh.indices.size()/3), 64, 64);
    const auto pixels = gpu.end();
    auto indexed = original->capture(constants, true);
    indexed.resize(3*22);
    GLint program = 0, current = 0;
    gpu.context->api().GetIntegerv(GL_CURRENT_PROGRAM, &program);
    auto retained = std::make_unique<GlWaterSurfaceRenderer>(*original, mesh);
    auto invalid = mesh;
    invalid.indices[0] = 0xffff;
    bool rejected = false;
    try { GlWaterSurfaceRenderer bad(*retained, invalid); }
    catch (const std::invalid_argument &) { rejected = true; }
    require(rejected, "Shared water shader accepted an invalid replacement mesh");
    auto subset = mesh;
    subset.indices.resize(3);
    GlWaterSurfaceRenderer last(*retained, subset);
    require(last.capture(constants, true) == indexed, "Shared water shader changed replacement geometry");
    gpu.context->api().GetIntegerv(GL_CURRENT_PROGRAM, &current);
    require(program == current, "Replacement water geometry recompiled the original program");
    original.reset();
    gpu.begin();
    retained->draw(constants, inputs, 0, unsigned(mesh.indices.size()/3), 64, 64);
    require(gpu.end() == pixels, "Destroyed initial renderer invalidated its retained water shader");
    retained.reset();
    require(last.capture(constants, true) == indexed, "Final water shader owner lost its mesh/program");
    gpu.context->api().GetIntegerv(GL_CURRENT_PROGRAM, &current);
    require(program == current, "Final water shader owner changed the shared program");
    gpu.context->check("Water shared shader lifetime checks");
    return 6;
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        require(argc == 4 || argc == 5 || argc == 7, "Usage: civ5_gl_water_surface asset-root original-shaders "
            "fresh-output [diagnostic-prefixes | --precision-probes directory variant-or-all]");
        const bool precision = argc == 7;
        int precision_variant = -1;
        if (precision) {
            require(std::wstring_view(argv[4]) == L"--precision-probes", "Water precision option");
            const std::wstring_view requested(argv[6]);
            if (requested != L"all") {
                require((requested.size() == 1 && requested[0] >= L'0' && requested[0] <= L'9') ||
                        requested == L"10" || requested == L"11", "Water precision variant 0..11 or all");
                precision_variant = std::stoi(std::wstring(requested));
            }
        }
        const fs::path root(argv[1]), out(argv[3]);
        require(!fs::exists(out), "Fresh water output");
        auto context = std::make_shared<GlContext>();
        Control gpu(context);
        NativeWater native(argv[2]);
        const auto lean = LeanData::parse(authored(root, "Common/TerrainModels.fpk", "waterbumps16.lean"));
        const auto color =
            DdsData::parse(authored(root, "DX9/Upgrade1Textures.fpk", "waterdepthcolor_new.dds"));
        require(color.format() == DdsFormat::masked_rgb && color.bits_per_pixel() == 32 &&
                    color.channel_masks() == std::array<std::uint32_t, 4>{0xff0000, 0xff00, 0xff, 0xff000000},
                "Original WaterColor format");
        GlWaterSurfaceInputs inputs;
        std::array<IDirect3DBaseTexture9 *, 14> pointers{};
        std::vector<GlTexture> textures;
        std::vector<Com<IDirect3DBaseTexture9>> native_textures;
        fs::create_directories(out);
        const auto save_shader = [&](const std::string &name, std::string_view source) {
            write(out / name, {reinterpret_cast<const std::uint8_t *>(source.data()), source.size()});
        };
        save_shader("water-vs.glsl", water_surface_vertex_shader());
        for(unsigned selector:{0u,2u,3u,5u}) {
            save_shader("water-ps"+std::to_string(selector)+".glsl",water_surface_fragment_shader(selector));
            save_shader("water-ps"+std::to_string(selector)+"-mip-diagnostic.glsl",water_mip_diagnostic_source(selector));
        }
        write(out / "waterbumps16.lean", lean.original_bytes());
        write(out / "waterdepthcolor_new.dds", color.original_bytes());
        for (unsigned slot : {0u, 1u}) {
            const auto map = slot == 0 ? LeanMap::rgba16 : LeanMap::luminance16;
            textures.push_back(GlTexture::lean(*context, lean, map));
            inputs.pixel[slot] = textures.back().id();
            auto tex = native.lean(lean, map);
            native_textures.emplace_back(std::exchange(tex.value, nullptr));
            pointers[slot] = native_textures.back().value;
        }
        {
            GlSampler sampler(*context);
            sampler.SamplerParameteri(sampler.sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
            sampler.SamplerParameteri(sampler.sampler,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            for(auto state:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R})sampler.SamplerParameteri(sampler.sampler,state,GL_REPEAT);
            for(auto state:{D3DSAMP_MINFILTER,D3DSAMP_MAGFILTER,D3DSAMP_MIPFILTER})checked(native.device->SetSamplerState(0,state,D3DTEXF_LINEAR),"LEAN linear control");
            for(auto state:{D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV,D3DSAMP_ADDRESSW})checked(native.device->SetSamplerState(0,state,D3DTADDRESS_WRAP),"LEAN wrap control");
            std::vector<float> samples;
            for(unsigned map=0;map<2;++map)
                for(float lod:{0.f,.125f,.3f,.7f,1.f,1.5f,3.1f,5.25f,6.7f,8.2f,9.f})
                    for(unsigned i=0;i<20;++i) {
                        const Vector4 coordinate{-1.31f+i*.19731f,-.91f+i*.13779f,0,lod};
                        const auto dx=native.sample(pointers[map],false,coordinate),gl=sampler.sample(textures[map],false,coordinate);
                        for(const auto &v:{Vector4{float(map),0,0,0},coordinate,dx,gl})samples.insert(samples.end(),v.begin(),v.end());
                    }
            save(out/"lean-linear-samples.bin",samples);
            for(auto state:{D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV,D3DSAMP_ADDRESSW})checked(native.device->SetSamplerState(0,state,D3DTADDRESS_CLAMP),"Restore WaterColor clamp control");
        }
        textures.emplace_back(*context, color, DdsSamplingColor::srgb);
        inputs.pixel[6] = textures.back().id();
        native_textures.push_back(native.upload(color, D3DFMT_A8R8G8B8));
        pointers[6] = native_textures.back().value;
        verify_native_2d(pointers[6], color);
        require(textures.back().read_level_raw(0, 0) ==
                    Bytes(color.payload(0, 0).begin(), color.payload(0, 0).end()),
                "Original WaterColor GL storage identity");
        {
            GlSampler sampler(*context);
            sampler.SamplerParameteri(sampler.sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            sampler.SamplerParameteri(sampler.sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            for (auto state : {D3DSAMP_MINFILTER, D3DSAMP_MAGFILTER, D3DSAMP_MIPFILTER})
                checked(native.device->SetSamplerState(0, state, D3DTEXF_LINEAR),
                        "WaterColor linear filter control");
            std::vector<float> samples;
            for (float y : {-.2f, .125f, .25f, .5f, .875f, 1.2f})
                for (float x : {-.2f, 0.f, .5f / 256, 1.f / 256, .1234f, .25f, .375f, .5f, .625f, .75f, .9f,
                                255.5f / 256, 1.f, 1.2f}) {
                    const Vector4 coordinate{x, y, 0, 0};
                    checked(native.device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 1),
                            "Original WaterColor SRGB state");
                    const auto dx = native.sample(pointers[6], false, coordinate),
                               gl = sampler.sample(textures.back(), false, coordinate);
                    checked(native.device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0),
                            "Negative missing WaterColor SRGB");
                    const auto wrong = native.sample(pointers[6], false, coordinate);
                    for (const auto &v : {coordinate, dx, gl, wrong})
                        samples.insert(samples.end(), v.begin(), v.end());
                }
            save(out / "watercolor-linear-srgb-samples.bin", samples);
        }
        for (unsigned slot : {4u, 5u, 7u, 8u, 9u, 10u, 13u}) {
            const unsigned size = slot == 7 ? 1024 : 64;
            Bytes raw;
            if (slot == 7) {
                raw.resize(size * size);
                for (unsigned y = 0; y < size; ++y)
                    for (unsigned x = 0; x < size; ++x)
                        raw[y * size + x] = std::uint8_t(1 + (x / 13 + y / 17) % 80);
            } else {
                std::vector<float> values(size * size * 4);
                for (unsigned y = 0; y < size; ++y)
                    for (unsigned x = 0; x < size; ++x) {
                        Vector4 v{};
                        if (slot == 4)
                            v = {.12f + x / 300.f, .2f + y / 250.f, .4f + (x + y) / 500.f, 1};
                        if (slot == 5)
                            v = {.35f + x / 220.f, .63f, .3f + y / 250.f, 1};
                        if (slot == 8)
                            v = {.15f + y / 100.f, .27f, .83f, .1f + x / 160.f};
                        if (slot == 9)
                            v = {.012f + x / 4000.f, .009f + y / 5000.f, .016f, (x % 13) / 150.f};
                        if (slot == 10)
                            v = {x < 8 ? 0.f : .2f + x / 100.f, .23f, .3f + y / 160.f, 1};
                        if (slot == 13)
                            v = {.1f + x / 90.f, .37f, .12f, 1};
                        std::copy(v.begin(), v.end(), values.begin() + (y * size + x) * 4);
                    }
                raw.resize(values.size() * 4);
                std::memcpy(raw.data(), values.data(), raw.size());
            }
            const auto data = control(size, size, slot == 7 ? 28 : 116, raw);
            textures.emplace_back(*context, data);
            inputs.pixel[slot] = textures.back().id();
            native_textures.push_back(native.upload(data, slot == 7 ? D3DFMT_A8 : D3DFMT_A32B32G32R32F));
            pointers[slot] = native_textures.back().value;
            verify_native_2d(pointers[slot], data);
            write(out / ("control-slot" + std::to_string(slot) + ".dds"), data.original_bytes());
            require(textures.back().read_level_raw(0, 0) == raw, "Water GL storage identity");
        }
        std::vector<float> lut(16 * 16 * 16 * 4);
        for (unsigned z = 0; z < 16; ++z)
            for (unsigned y = 0; y < 16; ++y)
                for (unsigned x = 0; x < 16; ++x) {
                    const Vector4 v{.03f + (x + z) / 38.f, .04f + (y + x) / 39.f, .05f + (z + y) / 40.f, 1};
                    std::copy(v.begin(), v.end(), lut.begin() + ((z * 16 + y) * 16 + x) * 4);
                }
        gpu.volume(lut, 16);
        auto native_lut = native.volume(lut, 16);
        inputs.pixel[11] = gpu.lut;
        pointers[11] = native_lut.value;
        save(out / "control-lut.bin", lut);
        std::array<GLuint, 2> fow_gl{};
        std::array<IDirect3DBaseTexture9 *, 2> fow_dx{};
        for (unsigned visible = 0; visible < 2; ++visible) {
            const Vector4 value{float(visible), .23f, .41f, 1};
            const auto data = control(1, 1, 116, {reinterpret_cast<const std::uint8_t *>(value.data()), 16});
            textures.emplace_back(*context, data);
            fow_gl[visible] = textures.back().id();
            native_textures.push_back(native.upload(data, D3DFMT_A32B32G32R32F));
            fow_dx[visible] = native_textures.back().value;
            verify_native_2d(fow_dx[visible], data);
            write(out / ("control-fow" + std::to_string(visible) + ".dds"), data.original_bytes());
        }
        const auto world = build_world_layout(
            12, 12, 0, {}, [](float x) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(x))); });
        const auto domain = WaterSurfaceDomain::from_world(world);
        const std::array<Vector4, 1> bounds{
            {{domain.origin_x, domain.origin_y, domain.origin_x + domain.span_x,
              domain.origin_y + domain.span_y}}};
        const auto mesh = build_water_surface_mesh(domain, bounds, 1, X87Precision::double_precision);
        save(out / "vertices.bin", mesh.vertices);
        save(out / "indices.bin", mesh.indices);
        save(out / "counts.bin", mesh.counts);
        save(out / "prefixes.bin", mesh.prefixes);
        WaterSurfaceResources resources{mesh.reallocate ? 137u : 37u, mesh.reallocate ? 283u : 83u,
                                        202, 303, 101, {}, {}};
        for (unsigned i = 0; i < resources.shared.size(); ++i)
            resources.shared[i] = 0x12340000 + 404 + i * 101;
        for (unsigned i = 0; i < resources.height_windows.size(); ++i)
            resources.height_windows[i] = 0xabcd0000 + 1201 + i * 17;
        std::ofstream report(out / "cases.tsv");
        report << "selector\tvariant\ttriangles\tmaxAbs\tnonfinite\tdiffering\n";
        unsigned cases = 0;
        unsigned guard_checks = 0;
        for (unsigned selector : {0u, 2u, 3u, 5u}) {
            GlWaterSurfaceRenderer renderer(context, mesh, selector);
            WaterProbe trilinear(*context, mesh, water_mip_diagnostic_source(selector));
            std::array<std::unique_ptr<WaterProbe>,4> arithmetic;
            for(unsigned i=0;i<arithmetic.size();++i) {
                const auto source=water_arithmetic_diagnostic_source(selector,i);
                arithmetic[i]=std::make_unique<WaterProbe>(*context,mesh,source);
                save_shader("water-ps"+std::to_string(selector)+"-arithmetic"+std::to_string(i)+".glsl",source);
            }
            const auto draws = water_surface_draws(mesh, resources, selector);
            Bytes packets(4); word(packets,0,unsigned(draws.size()));
            for(const auto &draw:draws) {
                const auto at=unsigned(packets.size());packets.resize(at+20);
                const std::array<unsigned,5> values{draw.view,draw.window,draw.selector,draw.first_index,draw.triangle_count};
                for(unsigned i=0;i<5;++i)word(packets,at+i*4,values[i]);
                packets.insert(packets.end(),draw.packet.begin(),draw.packet.end());
            }
            write(out/("s"+std::to_string(selector)+"-packets.bin"),packets);
            for (unsigned variant = 0; variant < 12; ++variant) {
                const float spanx = domain.field_width * domain.scales[0],
                            spany = domain.field_height * domain.scales[1];
                WaterSurfaceFrame frame{};
                frame.projection = {2 / spanx,
                                    0,
                                    0,
                                    0,
                                    0,
                                    -2 / spany,
                                    0,
                                    0,
                                    0,
                                    0,
                                    .01f,
                                    0,
                                    -1 - 2 * domain.origin_x / spanx,
                                    1 + 2 * domain.origin_y / spany,
                                    0,
                                    1};
                frame.fog_color = {.6f, .7f, .8f, 1};
                frame.fog_parameters = {.002f, .02f, 0, .37f};
                frame.shadow_fade = .6f;
                WaterSurfaceView view{};
                view.view = identity();
                view.overlay = identity();
                view.shadow = identity();
                view.reflection = identity();
                view.camera = {70, -190, 180};
                view.overlay[0] = 2 / domain.span_x;
                view.overlay[5] = 2 / domain.span_y;
                view.shadow[0] = .0014f;
                view.shadow[5] = .0017f;
                view.shadow[12] = .13f;
                view.shadow[13] = -.21f;
                view.reflection = view.shadow;
                WaterSurfaceSettings settings{.5f,
                                              {1.2f, 1.1f, .9f, 32},
                                              {.4f, .5f, .7f, 16},
                                              {.3f, -.4f, .8660254f},
                                              {-.2f, .7f, .68556546f},
                                              {.3f, -.4f, .8660254f, .4f},
                                              {-.2f, .7f, .68556546f, .3f},
                                              .2f,
                                              lean.gradient_scale(),
                                              .008f,
                                              .17f,
                                              .09f};
                if (variant == 0) {
                    frame.fog_parameters[1] = 0;
                    frame.shadow_fade = 0;
                    settings.reflection_amount = 0;
                }
                if (variant == 2) {
                    settings.water_phase = 2.7f;
                    settings.frequency = .031f;
                }
                if (variant == 3) {
                    settings.water_phase = -1.4f;
                    settings.frequency = .0003f;
                }
                if (variant == 4) {
                    view.camera = {-210, 90, 63};
                    frame.fog_parameters[1] = .07f;
                    settings.reflection_amount = .85f;
                }
                if (variant == 5) {
                    frame.projection[0] *= 3;
                    frame.projection[5] *= 3;
                    frame.projection[12] = .2f;
                    frame.projection[13] = -.1f;
                }
                if (variant == 8) {
                    frame.projection[3] = .00037f;
                    frame.projection[7] = -.00021f;
                    frame.projection[1] = .00031f;
                    frame.projection[4] = -.00017f;
                    view.shadow[3] = .00019f;
                    view.shadow[7] = .00027f;
                    view.reflection = view.shadow;
                }
                if (variant == 9) {
                    settings.gradient_scale *= .3f;
                    settings.frequency = .057f;
                    settings.water_phase = .73f;
                }
                if (variant == 10) {
                    settings.frequency = 0;
                    settings.ocean_height = .11f;
                    view.camera = {-350, 240, 90};
                }
                if (variant == 11) {
                    settings.specular_sun = {.71f, .94f, 1.33f, 2};
                    settings.specular_sky = {.73f, .89f, .92f, 4};
                    settings.reflection_amount = 1;
                    settings.fake_view_sun[3] = .87f;
                    settings.fake_view_sky[3] = .11f;
                    frame.fog_parameters[1] = .08f;
                }
                const auto constants =
                    water_surface_constants(domain, settings, frame, view, X87Precision::double_precision);
                if (selector == 3 && variant == 0) {
                    guard_checks = guards(gpu, mesh, constants, inputs);
                    std::ofstream pipeline_report(out / "pipeline-guards.txt");
                    pipeline_report << pipeline_lifetime_checks(gpu, mesh, constants, inputs) << '\n';
                    require(bool(pipeline_report), "Water shared shader guard report");
                }
                const auto stem = "s" + std::to_string(selector) + "-v" + std::to_string(variant);
                write(out / (stem + "-primitive.bin"), primitive(selector, settings, frame, view, bounds[0]));
                save(out / (stem + "-constants.bin"), constants);
                if (variant == 0) {
                    save(out / (stem + "-tf.bin"), renderer.capture(constants, false));
                    save(out / (stem + "-indexed.bin"), renderer.capture(constants, true));
                }
                auto selected_inputs = inputs;
                auto selected_pointers = pointers;
                if (variant == 6 || variant == 7) {
                    selected_inputs.pixel[10] = fow_gl[variant - 6];
                    selected_pointers[10] = fow_dx[variant - 6];
                }
                gpu.begin();
                for (const auto &draw : draws)
                    renderer.draw(constants, selected_inputs, draw.first_index, draw.triangle_count, 64, 64);
                const auto gl = gpu.end();
                gpu.registers(out, stem);
                native.begin(selector, constants, selected_pointers);
                std::array<float, 68> native_vertex{};
                std::array<float, 112> native_pixel{};
                checked(native.device->GetVertexShaderConstantF(0, native_vertex.data(), 17),
                        "Actual D3D9 vertex constants");
                checked(native.device->GetPixelShaderConstantF(0, native_pixel.data(), 28),
                        "Actual D3D9 pixel constants");
                save(out / (stem + "-uV-d3d9.bin"), native_vertex);
                save(out / (stem + "-uP-d3d9.bin"), native_pixel);
                for (const auto &draw : draws)
                    native.draw(mesh, draw.first_index, draw.triangle_count);
                const auto d3d = native.end();
                save(out / (stem + "-gl.bin"), gl);
                save(out / (stem + "-d3d9.bin"), d3d);
                gpu.begin();
                trilinear.draw(constants);
                const auto mip = gpu.end();
                save(out / (stem + "-trilinear.bin"), mip);
                for(unsigned i=0;i<arithmetic.size();++i) {
                    gpu.begin();arithmetic[i]->draw(constants);
                    save(out/(stem+"-arithmetic"+std::to_string(i)+".bin"),gpu.end());
                }
                if (argc == 5 && variant == 5 && selector != 5) {
                    const fs::path probes(argv[4]);
                    for (unsigned i = 0;; ++i) {
                        const auto key = "s" + std::to_string(selector) + "-prefix-" + (i < 10 ? "0" : "") +
                                         std::to_string(i);
                        if (!fs::exists(probes / (key + ".bin")))
                            break;
                        const auto bytes = read(probes / (key + ".bin")),
                                   source = read(probes / (key + ".glsl"));
                        require(bytes.size() % 4 == 0, "Water diagnostic alignment");
                        std::vector<DWORD> words(bytes.size() / 4);
                        std::memcpy(words.data(), bytes.data(), bytes.size());
                        Com<IDirect3DPixelShader9> shader;
                        checked(native.device->CreatePixelShader(words.data(), &shader.value),
                                "Water diagnostic PS");
                        WaterProbe program(*context, mesh,
                                           {reinterpret_cast<const char *>(source.data()), source.size()});
                        // Restore the original ordinary sampler binding before each probe.
                        gpu.begin();
                        for (const auto &draw : draws)
                            renderer.draw(constants, inputs, draw.first_index, draw.triangle_count, 64, 64);
                        gpu.begin();
                        program.draw(constants);
                        save(out / (key + "-gl.bin"), gpu.end());
                        native.begin(selector, constants, pointers);
                        checked(native.device->SetPixelShader(shader.value), "Water diagnostic binding");
                        for (const auto &draw : draws)
                            native.draw(mesh, draw.first_index, draw.triangle_count);
                        save(out / (key + "-d3d9.bin"), native.end());
                    }
                }
                if (precision && selector != 5 &&
                    (precision_variant < 0 || variant == unsigned(precision_variant))) {
                    const fs::path probes(argv[5]);
                    std::vector<fs::path> selected;
                    for (const auto &entry : fs::directory_iterator(probes))
                        if (entry.path().extension() == ".bin" &&
                            entry.path().filename().string().starts_with("s" + std::to_string(selector) + "-"))
                            selected.push_back(entry.path());
                    require(!selected.empty() && selected.size() <= 512, "Bounded complete water observations");
                    std::sort(selected.begin(), selected.end());
                    WaterPrecisionTarget observed(*context, gpu.gl, gpu.fbo, native.device.value);
                    for (const auto &binary : selected) {
                        auto glsl = binary;
                        glsl.replace_extension(".glsl");
                        const auto bytes = read(binary), source = read(glsl);
                        require(bytes.size() >= 8 && bytes.size() % 4 == 0, "Water observation bytecode extent");
                        std::vector<DWORD> words(bytes.size() / 4);
                        std::memcpy(words.data(), bytes.data(), bytes.size());
                        Com<IDirect3DPixelShader9> shader;
                        checked(native.device->CreatePixelShader(words.data(), &shader.value), "Water complete observation PS");
                        WaterProbe program(*context, mesh, {reinterpret_cast<const char *>(source.data()), source.size()});
                        gpu.begin();
                        for (const auto &draw : draws)
                            renderer.draw(constants, selected_inputs, draw.first_index, draw.triangle_count, 64, 64);
                        gpu.begin();
                        observed.clear();
                        program.draw(constants);
                        const auto key = stem + "-" + binary.stem().string();
                        save(out / (key + "-gl.bin"), gpu.end());
                        save(out / (key + "-observed-gl.bin"), observed.gl_output());
                        native.begin(selector, constants, selected_pointers);
                        checked(native.device->SetPixelShader(shader.value), "Water complete observation binding");
                        for (const auto &draw : draws)
                            native.draw(mesh, draw.first_index, draw.triangle_count);
                        save(out / (key + "-d3d9.bin"), native.end());
                        save(out / (key + "-observed-d3d9.bin"), observed.native_output());
                    }
                }
                float maximum = 0;
                unsigned nonfinite = 0, differing = 0;
                for (unsigned i = 0; i < gl.size(); ++i) {
                    if (!std::isfinite(gl[i]) || !std::isfinite(d3d[i]))
                        ++nonfinite;
                    else
                        maximum = std::max(maximum, std::abs(gl[i] - d3d[i]));
                    if (std::bit_cast<std::uint32_t>(gl[i]) != std::bit_cast<std::uint32_t>(d3d[i]))
                        ++differing;
                }
                report << selector << '\t' << variant << '\t' << mesh.indices.size() / 3 << '\t'
                       << std::setprecision(9) << maximum << '\t' << nonfinite << '\t' << differing << '\n';
                float mip_maximum = 0;
                for (unsigned i = 0; i < mip.size(); ++i)
                    mip_maximum = std::max(mip_maximum, std::abs(mip[i] - d3d[i]));
                std::cout << stem << " max=" << maximum << " trilinear=" << mip_maximum
                          << " nonfinite=" << nonfinite << " differing=" << differing << std::endl;
                ++cases;
            }
        }
        report.close();
        require(bool(report), "Water report write");
        std::ofstream device(out / "device.txt");
        device << context->description() << '\n';
        D3DADAPTER_IDENTIFIER9 adapter{};
        checked(native.api->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapter), "Water adapter");
        device << adapter.Description << '\n' << adapter.Driver << '\n';
        device.close();
        std::ofstream guard_report(out / "guards.txt");
        guard_report << guard_checks << '\n';
        guard_report.close();
        require(bool(guard_report), "Water guard report");
        std::cout << "Executed " << cases
                  << " complete original/GL water pairs; pixel agreement is "
                     "reported separately\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
