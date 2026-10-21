// Validation only: observe a small set of actual world-water pixels without
// rebuilding the landscape. Captured source inputs are never production input.
#include "civ5/graphics/gl_water_surface.hpp"
#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "water_surface_native.hpp"
#include <map>
using namespace water_validation;
namespace fs = std::filesystem;
#include "water_precision_target.hpp"
#include "water_surface_probe.hpp"
namespace {
constexpr unsigned width = 1280, height = 960;
Bytes authored(const fs::path &root, const char *archive, const char *name) {
    const auto relative = std::string("Resource/") + archive;
    const auto source = FpkSource::open(root / relative, relative);
    require(std::holds_alternative<std::shared_ptr<FpkSource>>(source), "Original water FPK");
    const auto found = std::get<std::shared_ptr<FpkSource>>(source)->find(AssetKey(name));
    require(std::holds_alternative<AssetHandle>(found), "Original water resource");
    return std::get<AssetHandle>(found).read_bytes();
}
void word(Bytes &bytes, unsigned offset, unsigned value) {
    std::memcpy(bytes.data() + offset, &value, 4);
}
DdsData control(unsigned extent, bool alpha, std::span<const std::uint8_t> raw) {
    require(raw.size() == std::size_t(extent) * extent * (alpha ? 1 : 16), "Control image extent");
    Bytes data(128);
    for (auto [offset, value] : {std::pair{0u, 0x20534444u}, {4, 124}, {8, 0x1007},
                                 {12, extent}, {16, extent}, {28, 1}, {76, 32},
                                 {80, alpha ? 2u : 4u}, {84, alpha ? 0u : 116u}})
        word(data, offset, value);
    if (alpha) { word(data, 88, 8); word(data, 104, 255); }
    data.insert(data.end(), raw.begin(), raw.end());
    return DdsData::parse(std::move(data), NumericDdsProfile::legacy_d3d9);
}
template <class T> std::vector<T> load(const fs::path &path) {
    const auto raw = read(path);
    require(raw.size() % sizeof(T) == 0, "Aligned world diagnostic input");
    std::vector<T> result(raw.size() / sizeof(T));
    std::memcpy(result.data(), raw.data(), raw.size());
    return result;
}
struct Targets {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    NativeWater &native;
    GLuint target = 0, fbo = 0, volume = 0;
    Com<IDirect3DSurface9> color, pixels;
    Targets(std::shared_ptr<GlContext> c, NativeWater &n) : context(std::move(c)), gl(*context), native(n) {
        context->api().CreateTextures(GL_TEXTURE_2D, 1, &target);
        context->api().TextureStorage2D(target, 1, GL_RGBA32F, width, height);
        gl.CreateFramebuffers(1, &fbo);
        gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, target, 0);
        const GLenum buffer = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(fbo, 1, &buffer);
        require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Diagnostic FBO");
        checked(native.device->CreateRenderTarget(width, height, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE,
                                                   0, FALSE, &color.value, nullptr), "Diagnostic native target");
        checked(native.device->CreateOffscreenPlainSurface(width, height, D3DFMT_A32B32G32R32F,
                    D3DPOOL_SYSTEMMEM, &pixels.value, nullptr), "Diagnostic native readback");
    }
    ~Targets() {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &fbo);
        context->api().DeleteTextures(1, &target);
        context->api().DeleteTextures(1, &volume);
    }
    void begin_gl() {
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl.Viewport(0, 0, width, height);
        for (auto state : {GL_DEPTH_TEST, GL_BLEND, GL_CULL_FACE, GL_SCISSOR_TEST, GL_DITHER,
                           GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD}) gl.Disable(state);
        const float sentinel[]{187 / 255.f, 51 / 255.f, 119 / 255.f, 1};
        gl.ClearNamedFramebufferfv(fbo, GL_COLOR, 0, sentinel);
    }
    void begin_dx() {
        checked(native.device->SetRenderTarget(0, color.value), "Diagnostic native target binding");
        checked(native.device->SetDepthStencilSurface(nullptr), "Diagnostic no depth");
        D3DVIEWPORT9 viewport{0, 0, width, height, 0, 1};
        checked(native.device->SetViewport(&viewport), "Diagnostic original viewport");
        for (auto [state, value] : std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{
                 {D3DRS_ZENABLE, 0}, {D3DRS_CULLMODE, D3DCULL_NONE}, {D3DRS_ALPHABLENDENABLE, 0},
                 {D3DRS_ALPHATESTENABLE, 0}, {D3DRS_SCISSORTESTENABLE, 0}, {D3DRS_DITHERENABLE, 0},
                 {D3DRS_SRGBWRITEENABLE, 0}, {D3DRS_COLORWRITEENABLE, 15}})
            checked(native.device->SetRenderState(state, value), "Diagnostic original raster control");
        checked(native.device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xffbb3377, 1, 0), "Diagnostic sentinel");
        checked(native.device->BeginScene(), "Diagnostic begin scene");
    }
    std::vector<float> sample_gl(std::span<const unsigned> points) {
        std::vector<float> all(width * height * 4), result;
        context->api().GetTextureImage(target, 0, GL_RGBA, GL_FLOAT, all.size() * 4, all.data());
        context->check("Diagnostic GL readback");
        for (unsigned point : points) result.insert(result.end(), all.begin() + point * 4, all.begin() + point * 4 + 4);
        return result;
    }
    std::vector<float> sample_dx(std::span<const unsigned> points) {
        checked(native.device->EndScene(), "Diagnostic end scene");
        checked(native.device->GetRenderTargetData(color.value, pixels.value), "Diagnostic native transfer");
        D3DLOCKED_RECT lock{};
        checked(pixels->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Diagnostic native read lock");
        const bool valid = lock.pBits && lock.Pitch >= int(width * 16);
        std::vector<float> result(points.size() * 4);
        if (valid)
            for (unsigned i = 0; i < points.size(); ++i)
                std::memcpy(result.data() + i * 4, static_cast<char *>(lock.pBits) +
                            std::size_t(points[i] / width) * lock.Pitch + (points[i] % width) * 16, 16);
        checked(pixels->UnlockRect(), "Diagnostic native read unlock");
        require(valid, "Diagnostic native pitch");
        return result;
    }
};
}
int wmain(int argc, wchar_t **argv) {
    try {
        require(argc == 5, "Usage: civ5_gl_water_world_probe asset-root original-water-shaders fixture fresh-output");
        const fs::path root(argv[1]), shaders(argv[2]), fixture(argv[3]), out(argv[4]);
        require(!fs::exists(out), "Fresh world-water observation output");
        auto context = std::make_shared<GlContext>();
        NativeWater native(shaders);
        Targets gpu(context, native);
        WaterSurfaceMesh mesh;
        mesh.vertices = load<std::array<float, 4>>(fixture / "vertices.bin");
        mesh.indices = load<std::uint16_t>(fixture / "indices.bin");
        const auto blocks = load<WaterConstantBlock>(fixture / "constants.bin");
        require(blocks.size() == 1, "One original complete water constant block");
        const auto &constants = blocks[0];
        const auto draws = load<std::array<unsigned, 4>>(fixture / "draws.bin");
        const auto points = load<unsigned>(fixture / "points.bin");
        require(!points.empty() && points.size() <= 10000, "Bounded diagnostic observations");
        for (auto point : points) require(point < width * height, "Diagnostic point bounds");
        GlWaterSurfaceRenderer renderer(context, mesh, 3);
        GlWaterSurfaceInputs inputs;
        std::array<IDirect3DBaseTexture9 *, 14> images{};
        std::vector<GlTexture> textures;
        std::vector<Com<IDirect3DBaseTexture9>> native_textures;
        const auto lean = LeanData::parse(authored(root, "Common/TerrainModels.fpk", "waterbumps16.lean"));
        for (unsigned slot = 0; slot < 2; ++slot) {
            const auto map = slot == 0 ? LeanMap::rgba16 : LeanMap::luminance16;
            textures.push_back(GlTexture::lean(*context, lean, map));
            inputs.pixel[slot] = textures.back().id();
            auto value = native.lean(lean, map);
            native_textures.emplace_back(std::exchange(value.value, nullptr));
            images[slot] = native_textures.back().value;
        }
        const auto color = DdsData::parse(authored(root, "DX9/Upgrade1Textures.fpk", "waterdepthcolor_new.dds"));
        textures.emplace_back(*context, color, DdsSamplingColor::srgb);
        inputs.pixel[6] = textures.back().id();
        native_textures.push_back(native.upload(color, D3DFMT_A8R8G8B8));
        images[6] = native_textures.back().value;
        for (unsigned slot : {4u, 5u, 8u, 9u, 10u, 13u}) {
            if (slot == 5) {
                const std::array<Rgba8Level, 1> levels{{{256, 256, read(fixture / "current.rgba8")}}};
                textures.push_back(GlTexture::rgba8(*context, levels));
                Com<IDirect3DTexture9> texture;
                checked(native.device->CreateTexture(256, 256, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                                     &texture.value, nullptr), "Captured current source");
                auto bgra = levels[0].rgba;
                for (std::size_t at = 0; at < bgra.size(); at += 4) std::swap(bgra[at], bgra[at + 2]);
                D3DLOCKED_RECT lock{};
                checked(texture->LockRect(0, &lock, nullptr, 0), "Captured current lock");
                require(lock.Pitch >= 1024, "Captured current pitch");
                for (unsigned y = 0; y < 256; ++y)
                    std::memcpy(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, bgra.data() + y * 1024, 1024);
                checked(texture->UnlockRect(0), "Captured current unlock");
                native_textures.emplace_back(std::exchange(texture.value, nullptr));
            } else if (slot == 8 && fs::is_regular_file(fixture / "fogramp.dds")) {
                const auto fog = DdsData::parse(read(fixture / "fogramp.dds"));
                textures.emplace_back(*context, fog);
                native_textures.push_back(native.upload(fog, D3DFMT_A8R8G8B8));
            } else {
                const std::array<float, 4> value =
                    slot == 10 ? std::array<float, 4>{1, 0, 1, 1} : std::array<float, 4>{};
                const auto data =
                    control(1, false, {reinterpret_cast<const std::uint8_t *>(value.data()), 16});
                textures.emplace_back(*context, data);
                native_textures.push_back(native.upload(data, D3DFMT_A32B32G32R32F));
            }
            inputs.pixel[slot] = textures.back().id();
            images[slot] = native_textures.back().value;
        }
        std::vector<float> volume(16 * 16 * 16 * 4);
        for (unsigned z = 0; z < 16; ++z) for (unsigned y = 0; y < 16; ++y) for (unsigned x = 0; x < 16; ++x) {
            const unsigned i = ((z * 16 + y) * 16 + x) * 4;
            volume[i] = x / 15.f; volume[i + 1] = y / 15.f; volume[i + 2] = z / 15.f; volume[i + 3] = 1;
        }
        context->api().CreateTextures(GL_TEXTURE_3D, 1, &gpu.volume);
        const auto storage = reinterpret_cast<PFNGLTEXTURESTORAGE3DPROC>(context->proc_address("glTextureStorage3D"));
        storage(gpu.volume, 1, GL_RGBA32F, 16, 16, 16);
        context->api().TextureSubImage3D(gpu.volume, 0, 0, 0, 0, 16, 16, 16, GL_RGBA, GL_FLOAT, volume.data());
        auto native_volume = native.volume(volume, 16);
        inputs.pixel[11] = gpu.volume; images[11] = native_volume.value;
        std::map<unsigned, GlTexture> heights;
        std::map<unsigned, Com<IDirect3DBaseTexture9>> native_heights;
        for (const auto &draw : draws) {
            require(draw[0] == 0 && draw[2] <= mesh.indices.size() && std::uint64_t(draw[3]) * 3 <= mesh.indices.size() - draw[2], "Original draw bounds");
            if (heights.contains(draw[1])) continue;
            const auto raw = read(fixture / ("height-window-" + std::to_string(draw[1]) + ".bin"));
            heights.emplace(draw[1], GlTexture::alpha8(*context, 1024, 1024, raw));
            native_heights.emplace(draw[1], native.upload(control(1024, true, raw), D3DFMT_A8));
        }
        fs::create_directories(out);
        std::unique_ptr<WaterPrecisionTarget> observed;
        const auto selected = [&](const std::vector<float> &all) {
            std::vector<float> result;
            for (unsigned point : points)
                result.insert(result.end(), all.begin() + point * 4, all.begin() + point * 4 + 4);
            return result;
        };
        const auto render = [&](const std::string &name, WaterProbe *probe, IDirect3DPixelShader9 *ps) {
            gpu.begin_gl();
            if (observed)
                observed->clear();
            for (const auto &draw : draws) {
                inputs.pixel[7] = heights.at(draw[1]).id();
                if (probe) {
                    context->api().BindTextureUnit(7, inputs.pixel[7]);
                    probe->draw(constants, width, height, draw[2], draw[3]);
                } else renderer.draw(constants, inputs, draw[2], draw[3], width, height);
            }
            save(out / (name + "-gl.bin"), gpu.sample_gl(points));
            if (observed)
                save(out / (name + "-observed-gl.bin"), selected(observed->gl_output()));
            gpu.begin_dx();
            for (const auto &draw : draws) {
                images[7] = native_heights.at(draw[1]).value;
                native.bind(3, constants, images);
                if (ps) checked(native.device->SetPixelShader(ps), "World diagnostic prefix binding");
                native.draw(mesh, draw[2], draw[3]);
            }
            save(out / (name + "-d3d9.bin"), gpu.sample_dx(points));
            if (observed)
                save(out / (name + "-observed-d3d9.bin"), selected(observed->native_output()));
        };
        render("full", nullptr, nullptr);
        if (fs::is_regular_file(fixture / "second-mrt.txt"))
            observed = std::make_unique<WaterPrecisionTarget>(*context, gpu.gl, gpu.fbo, native.device.value,
                                                              width, height);
        unsigned count = 0;
        for (const auto &file : fs::directory_iterator(fixture)) {
            if (file.path().extension() != ".glsl") continue;
            const auto bytes = read(fixture / (file.path().stem().string() + ".bin"));
            require(bytes.size() >= 8 && bytes.size() % 4 == 0, "Aligned world diagnostic bytecode");
            std::vector<DWORD> words(bytes.size() / 4);
            std::memcpy(words.data(), bytes.data(), bytes.size());
            Com<IDirect3DPixelShader9> ps;
            checked(native.device->CreatePixelShader(words.data(), &ps.value), "World diagnostic prefix");
            const auto text = read(file.path());
            WaterProbe probe(*context, mesh, {reinterpret_cast<const char *>(text.data()), text.size()});
            render(file.path().stem().string(), &probe, ps.value);
            if (++count % 20 == 0) std::cout << "Observed " << count << " original water prefixes\n" << std::flush;
        }
        std::cout << "PASS diagnostic observations: " << count << " prefixes, " << points.size() << " actual water pixels\n";
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
