#include "civ5/graphics/gl_water_current_map.hpp"
#include "civ5/graphics/gl_water_current.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/material_sources.hpp"
#include "water_current_native.hpp"
#include <limits>
using namespace water_validation;
using water_current_validation::NativeCurrent;
namespace fs = std::filesystem;
namespace {
struct OriginalRaster {
    D3DVIEWPORT9 viewport{};
    std::vector<std::array<unsigned, 2>> states;
    std::vector<std::array<unsigned, 6>> clears;
    std::array<float, 4> plane{};
    explicit OriginalRaster(const fs::path &path) {
        const auto raw = read(path);
        require(raw.size() >= 52 && std::memcmp(raw.data(), "C5WCRS01", 8) == 0,
                "Original current raster fixture header");
        std::size_t cursor = 8;
        const auto copy = [&](void *target, std::size_t bytes) {
            require(cursor <= raw.size() && bytes <= raw.size() - cursor, "Complete raster record");
            if (bytes != 0)
                std::memcpy(target, raw.data() + cursor, bytes);
            cursor += bytes;
        };
        copy(&viewport, sizeof(viewport));
        unsigned count = 0;
        copy(&count, 4);
        require(count == 20, "Complete original raster state writes");
        states.resize(count);
        copy(states.data(), states.size() * sizeof(states[0]));
        copy(&count, 4);
        require(count <= 1, "Bounded original clear commands");
        clears.resize(count);
        copy(clears.data(), clears.size() * sizeof(clears[0]));
        copy(plane.data(), sizeof(plane));
        require(cursor == raw.size(), "Unexpected raster fixture suffix");
    }
    void begin(NativeCurrent &gpu) const {
        for (unsigned slot = 0; slot < 4; ++slot)
            checked(gpu.device->SetRenderTarget(slot, slot == 0 ? gpu.color[1].value : nullptr),
                    "Original current target slots");
        for (unsigned slot = 0; slot < 16; ++slot)
            checked(gpu.device->SetTexture(slot, nullptr), "Original current texture unbind");
        checked(gpu.device->SetDepthStencilSurface(nullptr), "Original current depth attachment");
        for (const auto &clear : clears) {
            require(clear[0] == 0 && clear[1] == 0, "Original whole current clear");
            checked(gpu.device->Clear(0, nullptr, clear[2], clear[3], std::bit_cast<float>(clear[4]), clear[5]),
                    "Original conditional current clear");
        }
        checked(gpu.device->SetViewport(&viewport), "Original current descriptor viewport");
        for (unsigned i = 0; i < states.size(); ++i) {
            checked(gpu.device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(states[i][0]), states[i][1]),
                    "Executed original current render state");
            if (i == 2)
                checked(gpu.device->SetClipPlane(0, plane.data()), "Original disabled clip plane");
        }
    }
};
Bytes original_draw(NativeCurrent &gpu, const OriginalRaster &raster,
                    const WaterCurrentConstants &constants, IDirect3DTexture9 *source) {
    // Remaining device-wide policy is explicit, as in the previous control.
    for (auto state : {D3DRS_SCISSORTESTENABLE, D3DRS_DITHERENABLE, D3DRS_SRGBWRITEENABLE})
        checked(gpu.device->SetRenderState(state, FALSE), "Explicit original device policy");
    raster.begin(gpu);
    for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
             {D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP}, {D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP},
             {D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP}, {D3DSAMP_MINFILTER, D3DTEXF_LINEAR},
             {D3DSAMP_MAGFILTER, D3DTEXF_LINEAR}, {D3DSAMP_MIPFILTER, D3DTEXF_LINEAR},
             {D3DSAMP_MAXMIPLEVEL, 0}, {D3DSAMP_MAXANISOTROPY, 1}, {D3DSAMP_MIPMAPLODBIAS, 0},
             {D3DSAMP_SRGBTEXTURE, 0}})
        checked(gpu.device->SetSamplerState(0, state, value), "Original current sampler0");
    checked(gpu.device->SetVertexDeclaration(gpu.declaration.value), "Original current declaration");
    checked(gpu.device->SetVertexShader(gpu.vs.value), "Original current vertex program");
    checked(gpu.device->SetPixelShader(gpu.ps.value), "Original current pixel program");
    checked(gpu.device->SetPixelShaderConstantF(0, constants.data(), 4), "Original current constants");
    checked(gpu.device->SetTexture(0, source), "Original current input");
    checked(gpu.device->BeginScene(), "Original owned current begin");
    const auto result = gpu.device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2,
        gpu.indices.data(), D3DFMT_INDEX32, gpu.vertices.data(), 32);
    checked(gpu.device->EndScene(), "Original owned current end");
    checked(result, "Original owned current draw");
    checked(gpu.device->GetRenderTargetData(gpu.color[1].value, gpu.pixels[1].value), "Original current transfer");
    Bytes pixels(256 * 256 * 4);
    D3DLOCKED_RECT lock{};
    checked(gpu.pixels[1]->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Original current pixel lock");
    const bool valid = lock.pBits && lock.Pitch >= 1024;
    if (valid)
        for (unsigned y = 0; y < 256; ++y)
            std::memcpy(pixels.data() + y * 1024, static_cast<char *>(lock.pBits) + y * lock.Pitch, 1024);
    checked(gpu.pixels[1]->UnlockRect(), "Original current pixel unlock");
    require(valid, "Original current readback pitch");
    for (std::size_t i = 0; i < pixels.size(); i += 4)
        std::swap(pixels[i], pixels[i + 2]);
    return pixels;
}
void poison(GlRenderApi &gl) {
    for (auto state : {GL_DEPTH_TEST, GL_STENCIL_TEST, GL_BLEND, GL_CULL_FACE, GL_SCISSOR_TEST,
                       GL_DITHER, GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD, GL_CLIP_DISTANCE0})
        gl.Enable(state);
    gl.DepthMask(GL_TRUE);
    gl.DepthFunc(GL_GREATER);
    gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    gl.Viewport(3, 5, 7, 9);
    gl.BlendEquationSeparate(GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT);
    gl.BlendFuncSeparate(GL_ZERO, GL_ONE, GL_ZERO, GL_ONE);
}
unsigned state_checks(const GlContext &context, GlRenderApi &gl) {
    unsigned count = 0;
    for (auto state : {GL_DEPTH_TEST, GL_STENCIL_TEST, GL_BLEND, GL_CULL_FACE, GL_SCISSOR_TEST,
                       GL_DITHER, GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD, GL_CLIP_DISTANCE0}) {
        require(!gl.IsEnabled(state), "Current map retained poisoned raster state");
        ++count;
    }
    for (auto [parameter, expected] : std::initializer_list<std::pair<GLenum, GLint>>{
             {GL_DEPTH_WRITEMASK, GL_FALSE}, {GL_DEPTH_FUNC, GL_LEQUAL}, {GL_DRAW_FRAMEBUFFER_BINDING, 0},
             {GL_BLEND_SRC_RGB, GL_ONE}, {GL_BLEND_SRC_ALPHA, GL_ONE}, {GL_BLEND_DST_RGB, GL_ZERO},
             {GL_BLEND_DST_ALPHA, GL_ZERO}, {GL_BLEND_EQUATION_RGB, GL_FUNC_ADD},
             {GL_BLEND_EQUATION_ALPHA, GL_FUNC_ADD}}) {
        GLint actual = -1;
        context.api().GetIntegerv(parameter, &actual);
        require(actual == expected, "Current map did not establish its raster profile");
        ++count;
    }
    std::array<GLint, 4> values{};
    context.api().GetIntegerv(GL_VIEWPORT, values.data());
    require(values == std::array<GLint, 4>{0, 0, 256, 256}, "Current map viewport");
    context.api().GetIntegerv(GL_COLOR_WRITEMASK, values.data());
    require(values == std::array<GLint, 4>{1, 1, 1, 1}, "Current map color mask");
    context.api().GetIntegerv(GL_POLYGON_MODE, values.data());
    require(values[0] == GL_FILL && values[1] == GL_FILL, "Current map fill mode");
    return count + 3;
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        require(argc == 5, "Usage: civ5_gl_water_current_map original-shaders quad-fixture raster-fixture fresh-output");
        const fs::path shaders(argv[1]), fixture(argv[2]), raster_fixture(argv[3]), out(argv[4]);
        require(!fs::exists(out), "Fresh owned current map output");
        auto context = std::make_shared<GlContext>();
        GlRenderApi gl(*context);
        NativeCurrent native(shaders, fixture);
        const std::array<OriginalRaster, 2> raster{{OriginalRaster(raster_fixture / "raster0.bin"),
                                                  OriginalRaster(raster_fixture / "raster1.bin")}};
        const auto levels = civ5::materials::material_resource_zero();
        Com<IDirect3DTexture9> input;
        checked(native.device->CreateTexture(256, 256, 7, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                             &input.value, nullptr), "Original current resource0");
        fs::create_directories(out);
        for (auto [name, source] : {std::pair{"current-vs.glsl", water_current_vertex_shader()},
                                   std::pair{"current-ps.glsl", water_current_fragment_shader()}})
            write(out / name, {reinterpret_cast<const std::uint8_t *>(source.data()), source.size()});
        for (unsigned mip = 0; mip < levels.size(); ++mip) {
            auto bgra = levels[mip].rgba;
            for (std::size_t i = 0; i < bgra.size(); i += 4)
                std::swap(bgra[i], bgra[i + 2]);
            native.upload(input.value, mip, bgra, levels[mip].width, levels[mip].height, 4);
            write(out / ("source-mip" + std::to_string(mip) + ".rgba"), levels[mip].rgba);
        }
        const auto is_texture = reinterpret_cast<PFNGLISTEXTUREPROC>(context->proc_address("glIsTexture"));
        const std::array<std::array<int, 2>, 3> grids{{{40, 28}, {128, 96}, {16, 14}}};
        unsigned checks = 0;
        for (unsigned grid = 0; grid < grids.size(); ++grid) {
            const auto raw = read(fixture / ("constants" + std::to_string(grid) + ".bin"));
            require(raw.size() == 64, "Original current constant extent");
            for (unsigned variant = 0; variant < 2; ++variant) {
                auto settings = civ5::materials::WaterCurrentSettings{};
                WaterCurrentConstants reference{};
                std::memcpy(reference.data(), raw.data(), raw.size());
                if (variant == 1) {
                    settings = {.37f, -.125f, 4.f};
                    reference[12] = .37f; reference[4] = -.125f; reference[8] = 4.f;
                }
                for (unsigned clear = 0; clear < 2; ++clear) {
                    poison(gl);
                    const auto owner_count = context.use_count();
                    GLuint target_id = 0, source_id = 0;
                    {
                        GlWaterCurrentMap map(context, grids[grid][0], grids[grid][1], settings, clear != 0);
                        checks += state_checks(*context, gl);
                        require(map.texture().levels().size() == 1 && map.texture().levels()[0].width == 256 &&
                                map.texture().levels()[0].height == 256 && map.texture().plan().internal_format == GL_RGBA8,
                                "Original current target metadata");
                        for (unsigned i = 0; i < 16; ++i) {
                            const bool written = i == 0 || i == 1 || i == 4 || i == 8 || i == 12;
                            require(std::bit_cast<unsigned>(map.constants()[i]) ==
                                    (written ? std::bit_cast<unsigned>(reference[i]) : 0u), "Owned current constants");
                        }
                        for (unsigned mip = 0; mip < levels.size(); ++mip)
                            require(map.source_texture().read_level_raw(0, mip) == levels[mip].rgba,
                                    "Owned current source mip identity");
                        const auto key = "g" + std::to_string(grid) + "-v" + std::to_string(variant) + "-c" + std::to_string(clear);
                        Bytes pixels(256 * 256 * 4);
                        context->api().GetTextureImage(map.texture().id(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                                       static_cast<GLsizei>(pixels.size()), pixels.data());
                        const auto original = original_draw(native, raster[clear], reference, input.value);
                        require(pixels == original, "Owned current map differs from original raster/shader draw");
                        write(out / (key + "-gl.rgba8"), pixels);
                        write(out / (key + "-d3d9.rgba8"), original);
                        save(out / (key + "-constants.bin"), map.constants());
                        target_id = map.texture().id(); source_id = map.source_texture().id();
                        require(is_texture(target_id) && is_texture(source_id), "Owned current texture lifetime");
                    }
                    require(context.use_count() == owner_count && !is_texture(target_id) && !is_texture(source_id),
                            "Current map did not release its context and textures");
                    context->check("Owned current lifecycle");
                    ++checks;
                }
            }
        }
        unsigned rejected = 0;
        const auto rejects = [&](auto action) {
            bool failed = false;
            try { action(); } catch (const std::invalid_argument &) { failed = true; }
            require(failed, "Missing current map input rejection");
            context->check("Rejected current map input");
            ++rejected;
        };
        rejects([&] { GlWaterCurrentMap bad(nullptr, 40, 28); });
        for (auto grid : {std::array<int, 2>{0, 28}, {-1, 28}, {40, 0}, {40, -1}})
            rejects([&] { GlWaterCurrentMap bad(context, grid[0], grid[1]); });
        for (unsigned index = 0; index < 3; ++index) {
            auto settings = civ5::materials::WaterCurrentSettings{};
            const auto bad = std::numeric_limits<float>::quiet_NaN();
            if (index == 0) settings.flow_strength = bad;
            if (index == 1) settings.noise_strength = bad;
            if (index == 2) settings.noise_scale = bad;
            rejects([&] { GlWaterCurrentMap invalid(context, 40, 28, settings); });
        }
        const auto result = "{\"schema\":\"civ5-owned-water-current-map-v1\",\"cases\":12,\"stateAndLifetimeChecks\":" +
            std::to_string(checks) + ",\"inputRejections\":" + std::to_string(rejected) + "}\n";
        write(out / "producer.json", {reinterpret_cast<const std::uint8_t *>(result.data()), result.size()});
        std::cout << result;
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
