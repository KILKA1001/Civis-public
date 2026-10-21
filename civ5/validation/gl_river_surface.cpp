#include "civ5/graphics/gl_river_surface.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "river_surface_native.hpp"
#include <cmath>
#include <limits>
#include <sstream>
using namespace river_validation;
using namespace civ5::terrain;
namespace fs = std::filesystem;
#include "river_surface_probe.hpp"
#include "water_precision_target.hpp"
namespace {
Bytes authored(const fs::path &root, const char *archive, const char *name) {
    const auto relative = std::string("Resource/") + archive;
    const auto opened = FpkSource::open(root / relative, relative);
    require(std::holds_alternative<std::shared_ptr<FpkSource>>(opened), "Original river FPK");
    const auto found = std::get<std::shared_ptr<FpkSource>>(opened)->find(AssetKey(name));
    require(std::holds_alternative<AssetHandle>(found), "Original river resource");
    return std::get<AssetHandle>(found).read_bytes();
}
void text(const fs::path &path, std::string_view value) {
    write(path, {reinterpret_cast<const std::uint8_t *>(value.data()), value.size()});
}
DdsData control(unsigned width, unsigned height, const std::vector<float> &values) {
    Bytes bytes(128 + values.size() * 4);
    const auto word = [&](unsigned at, unsigned value) { std::memcpy(bytes.data() + at, &value, 4); };
    word(0, 0x20534444);
    word(4, 124);
    word(8, 0x1007);
    word(12, height);
    word(16, width);
    word(28, 1);
    word(76, 32);
    word(80, 4);
    word(84, 116);
    std::memcpy(bytes.data() + 128, values.data(), values.size() * 4);
    return DdsData::parse(std::move(bytes), NumericDdsProfile::legacy_d3d9);
}
void verify(IDirect3DBaseTexture9 *base, const DdsData &data, const GlTexture &gl) {
    for (const auto &level : data.levels()) {
        const auto raw = data.payload(0, level.mip);
        require(gl.read_level_raw(0, level.mip) == Bytes(raw.begin(), raw.end()),
                "River authored GL storage identity");
        D3DLOCKED_RECT lock{};
        auto *texture = static_cast<IDirect3DTexture9 *>(base);
        checked(texture->LockRect(level.mip, &lock, nullptr, D3DLOCK_READONLY), "River source readback");
        bool ok = lock.pBits && lock.Pitch >= int(level.row_pitch);
        if (ok)
            for (std::size_t y = 0; y < level.length / level.row_pitch; ++y)
                ok = ok && std::memcmp(static_cast<char *>(lock.pBits) + y * lock.Pitch,
                                       raw.data() + y * level.row_pitch, level.row_pitch) == 0;
        checked(texture->UnlockRect(level.mip), "River source readback unlock");
        require(ok, "River original texture storage identity");
    }
}
struct Target {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GlTexture color, depth;
    GLuint framebuffer = 0;
    std::array<GLuint, 2> volumes{};
    Target(std::shared_ptr<GlContext> c)
        : context(c), gl(*c),
          color(GlTexture::render_target(*c, 64, 64, 1, RenderTextureFormat::rgba32_float)),
          depth(GlTexture::render_target(*c, 64, 64, 1, RenderTextureFormat::depth24_stencil8)) {
        gl.CreateFramebuffers(1, &framebuffer);
        gl.NamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, color.id(), 0);
        gl.NamedFramebufferTexture(framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, depth.id(), 0);
        const GLenum target = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(framebuffer, 1, &target);
        require(gl.CheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "River framebuffer");
    }
    ~Target() {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &framebuffer);
        context->api().DeleteTextures(2, volumes.data());
    }
    void begin(float z = 1) {
        gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        gl.Viewport(0, 0, 64, 64);
        gl.DepthRange(0, 1);
        for (auto state : {GL_SCISSOR_TEST, GL_DITHER, GL_RASTERIZER_DISCARD})
            gl.Disable(state);
        gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        gl.DepthMask(GL_TRUE);
        const float sentinel[]{187 / 255.f, 51 / 255.f, 119 / 255.f, 0};
        gl.ClearNamedFramebufferfv(framebuffer, GL_COLOR, 0, sentinel);
        gl.ClearNamedFramebufferfv(framebuffer, GL_DEPTH, 0, &z);
    }
    std::vector<float> pixels() {
        std::vector<float> result(64 * 64 * 4);
        context->api().GetTextureImage(color.id(), 0, GL_RGBA, GL_FLOAT, result.size() * 4, result.data());
        context->check("River GL pixels");
        return result;
    }
    void volume(unsigned index, const std::vector<float> &data, unsigned size) {
        auto &api = context->api();
        api.CreateTextures(GL_TEXTURE_3D, 1, &volumes[index]);
        const auto storage =
            reinterpret_cast<PFNGLTEXTURESTORAGE3DPROC>(context->proc_address("glTextureStorage3D"));
        storage(volumes[index], 1, GL_RGBA32F, size, size, size);
        api.TextureSubImage3D(volumes[index], 0, 0, 0, 0, size, size, size, GL_RGBA, GL_FLOAT, data.data());
        std::vector<float> readback(data.size());
        api.GetTextureImage(volumes[index], 0, GL_RGBA, GL_FLOAT, data.size() * 4, readback.data());
        require(readback == data, "River volume storage identity");
    }
    void registers(const fs::path &out, const std::string &stem, bool drawn) {
        GLint program = 0;
        context->api().GetIntegerv(GL_CURRENT_PROGRAM, &program);
        require(!drawn || program != 0, "River bound program");
        const auto get = reinterpret_cast<PFNGLGETUNIFORMFVPROC>(context->proc_address("glGetUniformfv"));
        for (auto [name, count] : {std::pair{"uV", 17u}, std::pair{"uP", 28u}}) {
            std::vector<float> values(count * 4);
            std::vector<unsigned> active(count);
            for (unsigned i = 0; i < count; ++i) {
                const auto key = std::string(name) + "[" + std::to_string(i) + "]";
                const auto location = drawn ? gl.GetUniformLocation(program, key.c_str()) : -1;
                active[i] = location >= 0;
                if (location >= 0)
                    get(program, location, values.data() + i * 4);
            }
            save(out / (stem + "-" + name + "-gl.bin"), values);
            save(out / (stem + "-" + name + "-active.bin"), active);
        }
    }
};
RiverSurfaceMesh load_mesh(const fs::path &folder) {
    auto v = read(folder / "vertices.bin"), i = read(folder / "indices.bin");
    require(v.size() % 32 == 0 && i.size() % 4 == 0, "River source mesh framing");
    RiverSurfaceMesh mesh;
    mesh.vertices.resize(v.size() / 32);
    mesh.indices.resize(i.size() / 4);
    if (!v.empty())
        std::memcpy(mesh.vertices.data(), v.data(), v.size());
    if (!i.empty())
        std::memcpy(mesh.indices.data(), i.data(), i.size());
    return mesh;
}
WaterConstantBlock constants(const RiverSurfaceMesh &mesh, unsigned variant, float gradient) {
    Vector3 lo{0, 0, 0}, hi{1, 1, 1};
    if (!mesh.vertices.empty()) {
        lo = {mesh.vertices[0][0], mesh.vertices[0][1], mesh.vertices[0][2]};
        hi = lo;
        for (const auto &v : mesh.vertices)
            for (unsigned j = 0; j < 3; ++j) {
                lo[j] = std::min(lo[j], v[j]);
                hi[j] = std::max(hi[j], v[j]);
            }
    }
    const float sx = std::max(hi[0] - lo[0], 1.f) * 1.2f, sy = std::max(hi[1] - lo[1], 1.f) * 1.2f;
    const float x = (lo[0] + hi[0]) * .5f, y = (lo[1] + hi[1]) * .5f, z = (lo[2] + hi[2]) * .5f;
    const WaterSurfaceDomain domain{64, 64, x - sx * .5f, y - sy * .5f, sx, sy, {1, 1, .2f}};
    WaterSurfaceFrame frame{};
    frame.projection = {2 / sx, 0,     0, 0,           0,           2 / sy,          0, 0, 0,
                        0,      .001f, 0, -2 * x / sx, -2 * y / sy, .5f - z * .001f, 1};
    frame.fog_color = {.6f, .7f, .8f, 1};
    frame.fog_parameters = {.002f, .000002f, 0, .37f};
    frame.shadow_fade = .6f;
    WaterSurfaceView view{};
    view.view = identity_matrix();
    view.overlay = frame.projection;
    view.shadow = identity_matrix();
    view.reflection = frame.projection;
    view.camera = {x + 70, y - 190, z + 180};
    WaterSurfaceSettings settings{.5f,
                                  {1.2f, 1.1f, .9f, 32},
                                  {.4f, .5f, .7f, 16},
                                  {.3f, -.4f, .8660254f},
                                  {-.2f, .7f, .68556546f},
                                  {.3f, -.4f, .8660254f, .4f},
                                  {-.2f, .7f, .68556546f, .3f},
                                  .2f,
                                  gradient,
                                  .008f,
                                  .17f,
                                  .09f};
    if (variant == 0) {
        frame.fog_parameters[1] = 0;
        settings.reflection_amount = 0;
    }
    if (variant == 2)
        settings.water_phase = 2.7f;
    if (variant == 3)
        settings.water_phase = -1.4f;
    if (variant == 4) {
        view.camera = {x - 210, y + 90, z + 63};
        frame.fog_parameters[1] = .000007f;
        settings.reflection_amount = .85f;
    }
    if (variant == 5) {
        frame.projection[0] *= 3;
        frame.projection[5] *= 3;
        frame.projection[12] *= 3;
        frame.projection[13] *= 3;
    }
    if (variant == 6) {
        frame.projection[0] *= .17f;
        frame.projection[5] *= .17f;
        frame.projection[12] *= .17f;
        frame.projection[13] *= .17f;
    }
    if (variant == 8) {
        frame.projection[3] = .00037f;
        frame.projection[7] = -.00021f;
        frame.projection[1] = .00031f;
        frame.projection[4] = -.00017f;
    }
    if (variant == 9) {
        settings.gradient_scale *= .3f;
        settings.water_phase = .73f;
    }
    if (variant == 10)
        view.camera = {x - 350, y + 240, z + 90};
    if (variant == 11) {
        settings.specular_sun = {.71f, .94f, 1.33f, 2};
        settings.specular_sky = {.73f, .89f, .92f, 4};
        settings.reflection_amount = 1;
        settings.fake_view_sun[3] = .87f;
        settings.fake_view_sky[3] = .11f;
        frame.fog_parameters[1] = .000008f;
    }
    // Keep every original elevated vertex inside the depth interval, then use
    // shifted copies in the two-layer depth-write witnesses.
    const float sz = std::max(hi[2] - lo[2], 1.f);
    frame.projection[10] = .2f / sz;
    frame.projection[14] = .5f - z * frame.projection[10];
    return water_surface_constants(domain, settings, frame, view, X87Precision::double_precision);
}
unsigned guards(Target &gpu, const RiverSurfaceMesh &mesh, const GlRiverSurfaceInputs &inputs,
                float gradient) {
    if (mesh.indices.empty())
        return 0;
    unsigned count = 0;
    const auto rejects = [&](auto action) {
        bool rejected = false;
        try {
            action();
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        require(rejected, "River malformed input was accepted");
        gpu.context->check("River rejected input preserved valid GL state");
        ++count;
    };
    auto c = constants(mesh, 1, gradient);
    rejects([&] { GlRiverSurfaceRenderer invalid(nullptr, mesh, 1); });
    rejects([&] { GlRiverSurfaceRenderer invalid(gpu.context, mesh, 2); });
    auto invalid_mesh = mesh;
    invalid_mesh.indices.pop_back();
    rejects([&] { GlRiverSurfaceRenderer invalid(gpu.context, invalid_mesh, 1); });
    invalid_mesh = mesh;
    invalid_mesh.indices[0] = static_cast<unsigned>(mesh.vertices.size());
    rejects([&] { GlRiverSurfaceRenderer invalid(gpu.context, invalid_mesh, 1); });
    invalid_mesh = mesh;
    invalid_mesh.vertices[0][5] = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { GlRiverSurfaceRenderer invalid(gpu.context, invalid_mesh, 1); });
    GlRiverSurfaceRenderer high(gpu.context, mesh, 4);
    GlRiverSurfaceRenderer low(gpu.context, mesh, 1);
    gpu.begin();
    high.draw(c, inputs, 64, 64);
    const auto baseline = gpu.pixels();
    auto invalid_c = c;
    invalid_c[0] = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { high.draw(invalid_c, inputs, 64, 64); });
    rejects([&] { high.draw(c, inputs, 0, 64); });
    for (unsigned slot : {0u, 4u, 5u, 6u, 8u, 9u, 10u, 11u, 12u}) {
        auto invalid = inputs;
        invalid.pixel[slot] = 0;
        rejects([&] { high.draw(c, invalid, 64, 64); });
    }
    for (unsigned slot = 0; slot < 4; ++slot) {
        auto invalid = inputs;
        invalid.pixel[slot] = inputs.pixel[slot % 2 ? 0 : 4];
        rejects([&] { high.draw(c, invalid, 64, 64); });
    }
    auto invalid = inputs;
    invalid.pixel[11] = inputs.pixel[10];
    rejects([&] { high.draw(c, invalid, 64, 64); });
    invalid = inputs;
    invalid.pixel[4] = inputs.pixel[11];
    rejects([&] { high.draw(c, invalid, 64, 64); });
    invalid = inputs;
    invalid.pixel[6] = inputs.pixel[9];
    rejects([&] { high.draw(c, invalid, 64, 64); });
    invalid = inputs;
    invalid.pixel[5] = gpu.color.id();
    rejects([&] { high.draw(c, invalid, 64, 64); });
    invalid = inputs;
    invalid.pixel[5] = gpu.depth.id();
    rejects([&] { high.draw(c, invalid, 64, 64); });
    gpu.begin();
    auto unused = inputs;
    unused.pixel[7] = 0xffffffffu;
    auto unused_c = c;
    unused_c[44] = std::numeric_limits<float>::quiet_NaN(); // ocean phase, absent from river VS
    high.draw(unused_c, unused, 64, 64);
    require(gpu.pixels() == baseline, "Unused river constant/texture changed output");
    ++count;
    gpu.begin();
    low.draw(c, inputs, 64, 64);
    const auto low_baseline = gpu.pixels();
    for (unsigned slot = 0; slot < 5; ++slot)
        unused.pixel[slot] = 0xffffffffu;
    gpu.begin();
    low.draw(c, unused, 64, 64);
    require(gpu.pixels() == low_baseline, "Low river variant accessed high-quality textures");
    ++count;
    auto owned_mesh = mesh;
    std::unique_ptr<GlRiverSurfaceRenderer> retained;
    {
        GlRiverSurfaceRenderer owner(gpu.context, owned_mesh, 4);
        retained = std::make_unique<GlRiverSurfaceRenderer>(owner, owned_mesh);
    }
    owned_mesh.vertices.clear();
    owned_mesh.indices.clear();
    gpu.begin();
    retained->draw(c, inputs, 64, 64);
    require(gpu.pixels() == baseline, "River storage or shared pipeline depended on released caller state");
    ++count;
    owned_mesh = mesh;
    for (auto &vertex : owned_mesh.vertices)
        vertex[7] = std::numeric_limits<float>::quiet_NaN(); // unconsumed original COLOR0
    GlRiverSurfaceRenderer color_lane(high, owned_mesh);
    gpu.begin();
    color_lane.draw(c, inputs, 64, 64);
    require(gpu.pixels() == baseline, "Unused original COLOR0 affected river output");
    ++count;
    GlRiverSurfaceRenderer empty(high, RiverSurfaceMesh{});
    empty.draw(c, {}, 64, 64);
    require(gpu.pixels() == baseline && empty.capture(c, true).empty(), "Empty river modified target");
    ++count;
    return count;
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        require(
            argc >= 5 && argc <= 7,
            "Usage: civ5_gl_river_surface assets original-shaders mesh-directory fresh-output [diagnostics]");
        const fs::path assets(argv[1]), shaders(argv[2]), mesh_path(argv[3]), out(argv[4]);
        require(!fs::exists(out), "Fresh river GPU output");
        fs::create_directories(out);
        auto context = std::make_shared<GlContext>();
        Target gpu(context);
        NativeRiver native(shaders, out);
        D3DADAPTER_IDENTIFIER9 adapter{};
        checked(native.api->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapter), "River native adapter");
        text(out / "device.txt",
             context->description() + "\n" + adapter.Description + "\n" + adapter.Driver + "\nHAL/HWVP\n");
        text(out / "river-vs.glsl", river_surface_vertex_shader());
        for (unsigned selector : {1u, 4u})
            text(out / ("river-" + std::to_string(selector) + "-ps.glsl"),
                 river_surface_fragment_shader(selector));
        GlRiverSurfaceInputs inputs;
        std::array<IDirect3DBaseTexture9 *, 13> pointers{};
        std::vector<GlTexture> textures;
        std::vector<Com<IDirect3DBaseTexture9>> native_textures;
        const auto smooth =
            LeanData::parse(authored(assets, "Common/TerrainModels.fpk", "riversmooth16.lean"));
        const auto rough =
            LeanData::parse(authored(assets, "Common/Upgrade1Models.fpk", "riverrough_new16.lean"));
        write(out / "riversmooth16.lean", smooth.original_bytes());
        write(out / "riverrough_new16.lean", rough.original_bytes());
        for (unsigned slot = 0; slot < 4; ++slot) {
            const auto &data = slot < 2 ? smooth : rough;
            const auto map = slot % 2 ? LeanMap::luminance16 : LeanMap::rgba16;
            textures.push_back(GlTexture::lean(*context, data, map));
            GlSampler sampler(*context);
            sampler.SamplerParameteri(sampler.sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            sampler.SamplerParameteri(sampler.sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            for (auto state : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
                sampler.SamplerParameteri(sampler.sampler, state, GL_REPEAT);
            std::vector<float> before, after;
            for (float lod : {0.f, .3f, 1.f, 2.7f, 7.f})
                for (unsigned i = 0; i < 12; ++i) {
                    const auto v =
                        sampler.sample(textures.back(), false, {-.13f + i * .173f, .17f + i * .219f, 0, lod});
                    before.insert(before.end(), v.begin(), v.end());
                }
            for (const auto &level : data.levels(map)) {
                const auto raw = data.payload(map, level.mip);
                require(textures.back().read_level_raw(0, level.mip) == Bytes(raw.begin(), raw.end()),
                        "River LEAN GL storage identity");
            }
            for (float lod : {0.f, .3f, 1.f, 2.7f, 7.f})
                for (unsigned i = 0; i < 12; ++i) {
                    const auto v =
                        sampler.sample(textures.back(), false, {-.13f + i * .173f, .17f + i * .219f, 0, lod});
                    after.insert(after.end(), v.begin(), v.end());
                }
            save(out / ("lean" + std::to_string(slot) + "-before.bin"), before);
            save(out / ("lean" + std::to_string(slot) + "-after.bin"), after);
            require(before == after, "LEAN readback changed explicit linear samples");
            inputs.pixel[slot] = textures.back().id();
            auto t = native.lean(data, map);
            native_textures.emplace_back(std::exchange(t.value, nullptr));
            pointers[slot] = native_textures.back().value;
        }
        for (auto [slot, name] : {std::pair{5u, "river_alpha_new.dds"},
                                  {6u, "river_diffuse_new.dds"},
                                  {8u, "waterdepthcolor_new.dds"}}) {
            const auto data = DdsData::parse(authored(assets, "DX9/Upgrade1Textures.fpk", name));
            write(out / name, data.original_bytes());
            std::cout << name << " " << data.width() << "x" << data.height()
                      << " bpp=" << data.bits_per_pixel() << " fourcc=" << data.fourcc() << '\n';
            textures.emplace_back(*context, data,
                                  slot == 6 ? DdsSamplingColor::srgb : DdsSamplingColor::linear);
            inputs.pixel[slot] = textures.back().id();
            const auto format = data.format() == DdsFormat::dxt5 ? D3DFMT_DXT5 : D3DFMT_A8R8G8B8;
            require(data.format() == DdsFormat::dxt5 ||
                        (data.format() == DdsFormat::masked_rgb && data.bits_per_pixel() == 32 &&
                         data.channel_masks() ==
                             std::array<std::uint32_t, 4>{0xff0000, 0xff00, 0xff, 0xff000000}),
                    "River authored D3D9 format mapping");
            native_textures.push_back(native.upload(data, format));
            pointers[slot] = native_textures.back().value;
            verify(pointers[slot], data, textures.back());
        }
        for (unsigned slot : {4u, 9u, 10u}) {
            std::vector<float> values(64 * 64 * 4);
            for (unsigned y = 0; y < 64; ++y)
                for (unsigned x = 0; x < 64; ++x) {
                    Vector4 v{};
                    if (slot == 4)
                        v = {.12f + x / 300.f, .2f + y / 250.f, .4f + (x + y) / 500.f, 1};
                    if (slot == 9)
                        v = {.012f + x / 4000.f, .009f + y / 5000.f, .016f, (x % 13) / 150.f};
                    if (slot == 10)
                        v = {x < 8 ? 0.f : .2f + x / 100.f, .23f, .3f + y / 160.f, 1};
                    std::copy(v.begin(), v.end(), values.begin() + (y * 64 + x) * 4);
                }
            const auto data = control(64, 64, values);
            textures.emplace_back(*context, data);
            inputs.pixel[slot] = textures.back().id();
            native_textures.push_back(native.upload(data, D3DFMT_A32B32G32R32F));
            pointers[slot] = native_textures.back().value;
            verify(pointers[slot], data, textures.back());
            write(out / ("control-slot" + std::to_string(slot) + ".dds"), data.original_bytes());
        }
        std::array<Com<IDirect3DVolumeTexture9>, 2> volumes;
        for (unsigned volume = 0; volume < 2; ++volume) {
            std::vector<float> values(16 * 16 * 16 * 4);
            for (unsigned z = 0; z < 16; ++z)
                for (unsigned y = 0; y < 16; ++y)
                    for (unsigned x = 0; x < 16; ++x) {
                        const Vector4 v = volume
                                              ? Vector4{.03f + x / 17.f, .04f + y / 18.f, .05f + z / 19.f, 1}
                                              : Vector4{.02f + (x + z) / 38.f, .03f + (y + x) / 39.f,
                                                        .04f + (z + y) / 40.f, 1};
                        std::copy(v.begin(), v.end(), values.begin() + ((z * 16 + y) * 16 + x) * 4);
                    }
            gpu.volume(volume, values, 16);
            auto t = native.volume(values, 16);
            volumes[volume].value = std::exchange(t.value, nullptr);
            inputs.pixel[11 + volume] = gpu.volumes[volume];
            pointers[11 + volume] = volumes[volume].value;
            save(out / ("control-volume" + std::to_string(volume) + ".bin"), values);
        }
        std::vector<GlTexture> visibility;
        std::array<Com<IDirect3DBaseTexture9>, 2> native_visibility;
        for (unsigned i = 0; i < 2; ++i) {
            std::vector<float> values(64 * 64 * 4);
            for (unsigned texel = 0; texel < 64 * 64; ++texel) {
                values[texel * 4] = float(i);
                values[texel * 4 + 2] = float(i);
                values[texel * 4 + 3] = 1;
            }
            const auto data = control(64, 64, values);
            visibility.emplace_back(*context, data);
            auto t = native.upload(data, D3DFMT_A32B32G32R32F);
            native_visibility[i].value = std::exchange(t.value, nullptr);
            verify(native_visibility[i].value, data, visibility[i]);
            write(out / ("control-visibility" + std::to_string(i) + ".dds"), data.original_bytes());
        }
        const auto mesh = load_mesh(mesh_path);
        native.mesh(mesh);
        save(out / "vertices.bin", mesh.vertices);
        save(out / "indices.bin", mesh.indices);
        text(out / "guards.txt", std::to_string(guards(gpu, mesh, inputs, smooth.gradient_scale())) + "\n");
        std::ofstream report(out / "cases.tsv");
        report << "selector\tvariant\tmaxAbs\tnonfinite\tbadChannels\tcoveredPixels\n";
        for (unsigned selector : {1u, 4u}) {
            GlRiverSurfaceRenderer renderer(context, mesh, selector);
            auto reversed = mesh;
            for (std::size_t index = 0; index < reversed.indices.size(); index += 3)
                std::swap(reversed.indices[index], reversed.indices[index + 1]);
            GlRiverSurfaceRenderer reverse_renderer(renderer, reversed);
            std::array<RiverSurfaceMesh, 2> layers{mesh, mesh};
            const auto baseline_constants = constants(mesh, 1, smooth.gradient_scale());
            for (unsigned layer = 0; layer < 2; ++layer)
                for (auto &vertex : layers[layer].vertices)
                    vertex[2] += (layer == 0 ? .05f : -.05f) / baseline_constants[10];
            GlRiverSurfaceRenderer farther(renderer, layers[0]), nearer(renderer, layers[1]);
            for (unsigned variant = 0; variant < 17; ++variant) {
                const auto c = constants(mesh, variant, smooth.gradient_scale());
                const auto stem = "s" + std::to_string(selector) + "-v" + std::to_string(variant);
                save(out / (stem + "-constants.bin"), c);
                const float z = variant == 7 ? 0.f : 1.f;
                auto selected_inputs = inputs;
                auto selected_pointers = pointers;
                if (variant == 12 || variant == 13) {
                    selected_inputs.pixel[10] = visibility[variant - 12].id();
                    selected_pointers[10] = native_visibility[variant - 12].value;
                }
                native.mesh(variant == 14 ? reversed : mesh);
                gpu.begin(z);
                (variant == 14 ? reverse_renderer : renderer).draw(c, selected_inputs, 64, 64);
                if (variant >= 15)
                    (variant == 15 ? farther : nearer).draw(c, selected_inputs, 64, 64);
                const auto gl = gpu.pixels();
                gpu.registers(out, stem, !mesh.indices.empty());
                native.begin(selector, c, selected_pointers, z);
                native.draw();
                if (variant >= 15) {
                    native.mesh(layers[variant - 15]);
                    native.draw();
                }
                const auto dx = native.end();
                std::array<float, 68> vs{};
                std::array<float, 112> ps{};
                checked(native.device->GetVertexShaderConstantF(0, vs.data(), 17),
                        "River actual VS constants");
                checked(native.device->GetPixelShaderConstantF(0, ps.data(), 28),
                        "River actual PS constants");
                save(out / (stem + "-uV-d3d9.bin"), vs);
                save(out / (stem + "-uP-d3d9.bin"), ps);
                save(out / (stem + "-gl.bin"), gl);
                save(out / (stem + "-d3d9.bin"), dx);
                save(out / (stem + "-varyings.bin"),
                     (variant == 14 ? reverse_renderer : renderer).capture(c, true));
                double maximum = 0;
                unsigned nonfinite = 0, bad = 0, covered = 0;
                for (unsigned i = 0; i < gl.size(); ++i) {
                    if (!std::isfinite(gl[i]) || !std::isfinite(dx[i]))
                        ++nonfinite;
                    else {
                        const double delta = std::abs(double(gl[i]) - dx[i]);
                        maximum = std::max(maximum, delta);
                        bad += delta > 1e-5;
                    }
                    if (i % 4 == 3 && dx[i] != 0)
                        ++covered;
                }
                report << selector << '\t' << variant << '\t' << std::setprecision(17) << maximum << '\t'
                       << nonfinite << '\t' << bad << '\t' << covered << '\n';
                std::cout << stem << " max=" << maximum << " bad=" << bad << " nonfinite=" << nonfinite
                          << " covered=" << covered << '\n';
                if (argc >= 6 && variant == (argc == 7 ? std::stoul(argv[6]) : 1u)) {
                    std::vector<fs::path> probes;
                    for (const auto &entry : fs::directory_iterator(fs::path(argv[5])))
                        if (entry.path().extension() == ".bin" &&
                            entry.path().stem().string().starts_with("s" + std::to_string(selector) + "-"))
                            probes.push_back(entry.path());
                    std::sort(probes.begin(), probes.end());
                    require(!probes.empty() && probes.size() <= 512, "Bounded river diagnostics");
                    for (const auto &binary : probes) {
                        auto source = binary;
                        source.replace_extension(".glsl");
                        const auto code = read(binary), glsl = read(source);
                        require(code.size() % 4 == 0, "River diagnostic bytecode alignment");
                        std::vector<DWORD> words(code.size() / 4);
                        std::memcpy(words.data(), code.data(), code.size());
                        Com<IDirect3DPixelShader9> shader;
                        checked(native.device->CreatePixelShader(words.data(), &shader.value),
                                "River diagnostic PS");
                        RiverProbe program(*context, mesh,
                                           {reinterpret_cast<const char *>(glsl.data()), glsl.size()});
                        std::unique_ptr<WaterPrecisionTarget> observed;
                        if (binary.stem().string().ends_with("-mrt"))
                            observed = std::make_unique<WaterPrecisionTarget>(
                                *context, gpu.gl, gpu.framebuffer, native.device.value);
                        gpu.begin();
                        if (observed) {
                            const float sentinel[]{187 / 255.f, 51 / 255.f, 119 / 255.f, 0};
                            gpu.gl.ClearNamedFramebufferfv(gpu.framebuffer, GL_COLOR, 1, sentinel);
                        }
                        renderer.draw(c, inputs, 64, 64);
                        gpu.begin();
                        gpu.gl.Disable(GL_BLEND);
                        program.draw(c);
                        save(out / (binary.stem().string() + "-gl.bin"), gpu.pixels());
                        if (observed)
                            save(out / (binary.stem().string() + "-observed-gl.bin"), observed->gl_output());
                        native.begin(selector, c, pointers);
                        checked(native.device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE),
                                "River diagnostic blend");
                        checked(native.device->SetPixelShader(shader.value), "River diagnostic bind");
                        native.draw();
                        save(out / (binary.stem().string() + "-d3d9.bin"), native.end());
                        if (observed)
                            save(out / (binary.stem().string() + "-observed-d3d9.bin"),
                                 observed->native_output());
                    }
                }
            }
        }
        report.close();
        require(bool(report), "River GPU report");
        context->check("River GPU corpus complete");
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
