#include "civ5/graphics/gl_material_decal.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include "material_blend_native.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace civ5::graphics;
using civ5::materials::MaterialPatchMesh;
using blend_validation::Com;
using blend_validation::OwnWindow;
using blend_validation::hr;

namespace {
constexpr unsigned W = 16, H = 16;

void require(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}

struct NativeDecal {
    OwnWindow window;
    Com<IDirect3D9> api{Direct3DCreate9(D3D_SDK_VERSION)};
    Com<IDirect3DDevice9> device;
    Com<IDirect3DVertexShader9> vertex;
    Com<IDirect3DPixelShader9> pixel;
    Com<IDirect3DVertexDeclaration9> declaration;
    std::array<Com<IDirect3DSurface9>, 3> targets, readbacks;
    bool write_specular;

    NativeDecal(bool specular, const std::filesystem::path &shaders) : write_specular(specular) {
        require(api.value != nullptr, "Native decal D3D9 API");
        D3DPRESENT_PARAMETERS pp{};
        pp.BackBufferWidth = pp.BackBufferHeight = W;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferCount = 1;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = window.value;
        pp.Windowed = TRUE;
        pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        hr(api->CreateDevice(0, D3DDEVTYPE_HAL, window.value, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp,
                             &device.value),
           "Native decal device");
        for (unsigned i = 0; i < (write_specular ? 3u : 2u); ++i) {
            hr(device->CreateRenderTarget(W, H, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                          &targets[i].value, nullptr),
               "Native decal target");
            hr(device->CreateOffscreenPlainSurface(W, H, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM,
                                                   &readbacks[i].value, nullptr),
               "Native decal readback");
            hr(device->SetRenderTarget(i, targets[i].value), "Native decal MRT");
        }
        hr(device->SetDepthStencilSurface(nullptr), "Native decal depth");
        D3DVIEWPORT9 viewport{0, 0, W, H, 0, 1};
        hr(device->SetViewport(&viewport), "Native decal viewport");
        for (auto [state, value] : std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{
                 {D3DRS_ZENABLE, FALSE},
                 {D3DRS_ZWRITEENABLE, FALSE},
                 {D3DRS_STENCILENABLE, FALSE},
                 {D3DRS_CULLMODE, D3DCULL_NONE},
                 {D3DRS_SCISSORTESTENABLE, FALSE},
                 {D3DRS_DITHERENABLE, FALSE},
                 {D3DRS_SRGBWRITEENABLE, FALSE},
                 {D3DRS_ALPHATESTENABLE, FALSE},
                 {D3DRS_ALPHABLENDENABLE, FALSE},
                 {D3DRS_COLORWRITEENABLE, 15},
                 {D3DRS_COLORWRITEENABLE1, 15},
                 {D3DRS_COLORWRITEENABLE2, 15}})
            hr(device->SetRenderState(state, value), "Native decal raster state");
        const auto vs = blend_validation::read(shaders / "original-vs.bin");
        const auto ps = blend_validation::read(shaders / (write_specular ? "original-ps-specular.bin" : "original-ps.bin"));
        require(vs.size() == 604 && ps.size() == (write_specular ? 492u : 408u), "Original decal shader extents");
        hr(device->CreateVertexShader(reinterpret_cast<const DWORD *>(vs.data()), &vertex.value), "Original decal VS");
        hr(device->CreatePixelShader(reinterpret_cast<const DWORD *>(ps.data()), &pixel.value), "Original decal PS");
        const D3DVERTEXELEMENT9 elements[]{
            {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
            {0, 20, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0}, D3DDECL_END()};
        hr(device->CreateVertexDeclaration(elements, &declaration.value), "Native decal declaration");
    }

    std::vector<float> draw(const MaterialPatchMesh &mesh, const GlMaterialDecalFrame &frame,
                            std::array<IDirect3DTexture9 *, 3> textures) {
        hr(device->SetVertexDeclaration(declaration.value), "Native decal declaration bind");
        hr(device->SetVertexShader(vertex.value), "Native decal VS bind");
        hr(device->SetPixelShader(pixel.value), "Native decal PS bind");
        hr(device->SetVertexShaderConstantF(0, frame.transform.data(), 4), "Native decal transform");
        hr(device->SetVertexShaderConstantF(4, frame.texture_scales.data(), 1), "Native decal scales");
        hr(device->SetVertexShaderConstantF(5, frame.bounds.data(), 1), "Native decal bounds");
        const std::array<float,4> z{frame.z_distance,0,0,0};
        hr(device->SetVertexShaderConstantF(6, z.data(), 1), "Native decal z distance");
        for (unsigned i = 0; i < (write_specular ? 3u : 2u); ++i) {
            hr(device->SetTexture(i, textures[i]), "Native decal texture");
            for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                     {D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP},
                     {D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP},
                     {D3DSAMP_MINFILTER, D3DTEXF_LINEAR},
                     {D3DSAMP_MAGFILTER, D3DTEXF_LINEAR},
                     {D3DSAMP_MIPFILTER, D3DTEXF_NONE},
                     {D3DSAMP_MAXMIPLEVEL, 0},
                     {D3DSAMP_SRGBTEXTURE, FALSE}})
                hr(device->SetSamplerState(i, state, value), "Native decal sampler");
        }
        hr(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1, 0), "Native decal clear");
        hr(device->BeginScene(), "Native decal begin");
        const auto drawn = device->DrawIndexedPrimitiveUP(
            D3DPT_TRIANGLELIST, 0, mesh.columns() * mesh.rows(), static_cast<UINT>(mesh.indices().size() / 3),
            mesh.indices().data(), D3DFMT_INDEX16, mesh.vertices().data(), 48);
        const auto ended = device->EndScene();
        hr(drawn, "Native decal draw");
        hr(ended, "Native decal end");
        std::vector<float> result((write_specular ? 3u : 2u) * W * H * 4);
        for (unsigned target = 0; target < (write_specular ? 3u : 2u); ++target) {
            hr(device->GetRenderTargetData(targets[target].value, readbacks[target].value),
               "Native decal readback");
            D3DLOCKED_RECT lock{};
            hr(readbacks[target]->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Native decal lock");
            require(lock.Pitch >= int(W * 16), "Native decal readback pitch");
            for (unsigned y = 0; y < H; ++y)
                std::memcpy(result.data() + (target * W * H + y * W) * 4,
                            static_cast<const std::uint8_t *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                            W * 16);
            hr(readbacks[target]->UnlockRect(), "Native decal unlock");
        }
        return result;
    }
};

struct GlInputs {
    std::shared_ptr<GlContext> context;
    std::array<GLuint, 3> textures{};
    ~GlInputs() {
        if (context)
            context->api().DeleteTextures(static_cast<GLsizei>(textures.size()), textures.data());
    }
};

GlInputs make_inputs(std::shared_ptr<GlContext> context) {
    GlInputs result{std::move(context)};
    std::array<float, W * H * 4> values{};
    for (unsigned texture = 0; texture < 3; ++texture) {
        for (unsigned y = 0; y < H; ++y)
            for (unsigned x = 0; x < W; ++x) {
                const auto at = (y * W + x) * 4;
                values[at + 0] = .05f + .013f * float((x + texture * 3) % 17);
                values[at + 1] = texture == 1 ? float((x + y) % (H + 1)) / float(H) :
                                                .11f + .017f * float((y + texture) % 13);
                values[at + 2] = .23f + .009f * float((x * 3 + y + texture) % 19);
                values[at + 3] = 1.f;
            }
        const auto &api = result.context->api();
        api.CreateTextures(GL_TEXTURE_2D, 1, &result.textures[texture]);
        api.TextureStorage2D(result.textures[texture], 1, GL_RGBA32F, W, H);
        api.TextureSubImage2D(result.textures[texture], 0, 0, 0, W, H, GL_RGBA, GL_FLOAT, values.data());
        api.TextureParameteri(result.textures[texture], GL_TEXTURE_BASE_LEVEL, 0);
        api.TextureParameteri(result.textures[texture], GL_TEXTURE_MAX_LEVEL, 0);
    }
    result.context->check("Decal input texture upload");
    return result;
}

MaterialPatchMesh mesh() {
    std::vector<std::uint8_t> vertices(3 * 3 * 48);
    for (unsigned y = 0; y < 3; ++y)
        for (unsigned x = 0; x < 3; ++x) {
            const auto at = (y * 3 + x) * 48;
            const std::array<float, 3> position{float(x), float(y), 0.f};
            const std::array<float, 3> texcoord{float(x) / 2.f, float(y) / 2.f, 0.f};
            std::memcpy(vertices.data() + at, position.data(), sizeof(position));
            std::memcpy(vertices.data() + at + 20, texcoord.data(), sizeof(texcoord));
        }
    std::vector<std::uint16_t> indices;
    for (unsigned y = 0; y < 2; ++y)
        for (unsigned x = 0; x < 2; ++x) {
            const auto i = static_cast<std::uint16_t>(y * 3 + x);
            indices.insert(indices.end(), {i, static_cast<std::uint16_t>(i + 1),
                                           static_cast<std::uint16_t>(i + 3),
                                           static_cast<std::uint16_t>(i + 1),
                                           static_cast<std::uint16_t>(i + 4),
                                           static_cast<std::uint16_t>(i + 3)});
        }
    return MaterialPatchMesh(3, 3, vertices, indices);
}

GlMaterialDecalFrame frame() {
    GlMaterialDecalFrame result{};
    result.transform = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -2, -2, 0, 1};
    result.texture_scales = {1, 1, 0, 0};
    result.bounds = {0, 0, 1, 1};
    return result;
}

std::vector<float> gl_draw(GlMaterialDecalRenderer &renderer, std::shared_ptr<GlContext> context,
                           const GlMaterialDecalFrame &constants, const GlInputs &inputs,
                           bool specular);

std::vector<float> gl_draw(std::shared_ptr<GlContext> context, const MaterialPatchMesh &geometry,
                           const GlMaterialDecalFrame &constants, const GlInputs &inputs,
                           bool specular) {
    GlMaterialDecalRenderer renderer(context, geometry, specular);
    return gl_draw(renderer, context, constants, inputs, specular);
}

std::vector<float> gl_draw(GlMaterialDecalRenderer &renderer, std::shared_ptr<GlContext> context,
                           const GlMaterialDecalFrame &constants, const GlInputs &inputs,
                           bool specular) {
    std::array<GlTexture, 3> targets{
        GlTexture::render_target(*context, W, H, 1, RenderTextureFormat::rgba32_float),
        GlTexture::render_target(*context, W, H, 1, RenderTextureFormat::rgba32_float),
        GlTexture::render_target(*context, W, H, 1, RenderTextureFormat::rgba32_float)};
    GlRenderApi gl(*context);
    GLuint fbo = 0;
    gl.CreateFramebuffers(1, &fbo);
    try {
        for (unsigned i = 0; i < (specular ? 3u : 2u); ++i)
            gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0 + i, targets[i].id(), 0);
        const std::array<GLenum, 3> buffers{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                             GL_COLOR_ATTACHMENT2};
        gl.NamedFramebufferDrawBuffers(fbo, specular ? 3 : 2, buffers.data());
        require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "GL decal framebuffer");
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl.Viewport(0, 0, W, H);
        for (auto state : {GL_BLEND, GL_CULL_FACE, GL_DEPTH_TEST, GL_SCISSOR_TEST, GL_DITHER,
                           GL_FRAMEBUFFER_SRGB})
            gl.Disable(state);
        gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        const std::array<float,4> zero{};
        for (unsigned i=0;i<(specular?3u:2u);++i) gl.ClearNamedFramebufferfv(fbo,GL_COLOR,i,zero.data());
        renderer.draw(constants, inputs.textures[0], inputs.textures[1], specular ? inputs.textures[2] : 0,
                      {int(W), int(H)});
        context->check("GL decal draw");
        std::vector<float> result((specular ? 3u : 2u) * W * H * 4);
        for (unsigned i = 0; i < (specular ? 3u : 2u); ++i) {
            const auto bytes = targets[i].read_level_raw(0, 0);
            require(bytes.size() == W * H * 16, "GL decal readback extent");
            std::memcpy(result.data() + i * W * H * 4, bytes.data(), bytes.size());
        }
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &fbo);
        return result;
    } catch (...) {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &fbo);
        throw;
    }
}

float max_error(std::span<const float> a, std::span<const float> b) {
    require(a.size() == b.size(), "Decal comparison extent");
    float result = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        require(std::isfinite(a[i]) && std::isfinite(b[i]), "Nonfinite decal output");
        result = std::max(result, std::abs(a[i] - b[i]));
    }
    return result;
}
} // namespace

int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: civ5_gl_material_decal original-shaders fresh-output");
        const std::filesystem::path output(argv[2]);
        require(!std::filesystem::exists(output), "Fresh decal output required");
        std::filesystem::create_directories(output);
        auto context = std::make_shared<GlContext>(W, H);
        const auto geometry = mesh();
        auto constants = frame();
        auto inputs = make_inputs(context);
        std::ofstream report(output / "decal.json");
        report << "{\"schema\":\"civ5-material-decal-pass-v1\",\"gl\":"
               << std::quoted(context->description()) << ",\"cases\":[";
        for (unsigned variant = 0; variant < 4; ++variant) {
            const auto specular = variant & 1u;
            constants = frame();
            if (variant >= 2) {
                constants.bounds = {-3,5,-1,9};
                constants.transform = {.5f,0,0,0,0,.25f,0,0,0,0,1,0,0,-2.5f,0,1};
            }
            NativeDecal native(specular != 0, argv[1]);
            // The native side receives the same authored values through small
            // float textures created below; GL names never cross the API.
            std::array<Com<IDirect3DTexture9>, 3> textures;
            std::array<float, W * H * 4> values{};
            for (unsigned texture = 0; texture < 3; ++texture) {
                for (unsigned y = 0; y < H; ++y)
                    for (unsigned x = 0; x < W; ++x) {
                        const auto at = (y * W + x) * 4;
                        values[at + 0] = .05f + .013f * float((x + texture * 3) % 17);
                        values[at + 1] = texture == 1 ? float((x + y) % (H + 1)) / float(H) :
                                                        .11f + .017f * float((y + texture) % 13);
                        values[at + 2] = .23f + .009f * float((x * 3 + y + texture) % 19);
                        values[at + 3] = 1.f;
                    }
                hr(native.device->CreateTexture(W, H, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED,
                                                 &textures[texture].value, nullptr),
                   "Native decal input texture");
                D3DLOCKED_RECT lock{};
                hr(textures[texture]->LockRect(0, &lock, nullptr, 0), "Native decal input lock");
                require(lock.Pitch >= int(W * 16), "Native decal input pitch");
                for (unsigned y = 0; y < H; ++y)
                    std::memcpy(static_cast<std::uint8_t *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                                values.data() + std::size_t(y) * W * 4, W * 16);
                hr(textures[texture]->UnlockRect(0), "Native decal input unlock");
            }
            auto native_pixels = native.draw(geometry, constants,
                                             {textures[0].value, textures[1].value, textures[2].value});
            auto gl_pixels = gl_draw(context, geometry, constants, inputs, specular != 0);
            const auto error = max_error(native_pixels, gl_pixels);
            require(error <= 3e-5f, "GL decal pass differs from D3D9 control");
            auto replacement = std::make_shared<GlContext>(W, H);
            auto replacement_inputs = make_inputs(replacement);
            GlMaterialDecalRenderer recovered(context, geometry, specular != 0);
            const auto before_recovery = gl_draw(recovered, context, constants, inputs, specular != 0);
            bool rejected = false;
            try {
                recovered.rebuild_context(context);
            } catch (const std::invalid_argument &) {
                rejected = true;
            }
            require(rejected, "Decal recovery accepted the creating context");
            recovered.rebuild_context(replacement);
            const auto after_recovery = gl_draw(recovered, replacement, constants, replacement_inputs,
                                                specular != 0);
            require(after_recovery == before_recovery, "Decal context recovery changed pass pixels");
            if (variant)
                report << ',';
            report << "{\"translatedBounds\":" << (variant >= 2) << ",\"specular\":" << specular << ",\"maxAbsError\":" << error
                   << ",\"outputs\":" << (specular ? 3 : 2)
                   << ",\"contextRecoveryBitExact\":true}";
            std::cout << "PASS decal pass variant=" << variant << " specular=" << specular << " maxAbsError=" << error << '\n';
        }
        report << "],\"originalShaderExecuted\":true,\"sameRunConstants\":false,"
                  "\"worldModeEnabled\":false,\"compressionTouched\":false,\"fullGraphicsParity\":false}\n";
        report.close();
        require(bool(report), "Decal report write");
        std::ofstream scope(output / "scope.txt");
        scope << "Isolated 05D98A10 decal vertex and 05D9B760/05D9B8F8 pixel behavior.\n"
                 "The D3D9 side executes the unmodified exported original SM3 bytecode with controlled constants and MRT layout.\n"
                 "The GL side translates the same arithmetic and applies the explicit D3D9-to-GL clip convention.\n"
                 "Both two-output and optional three-output passes cover identity and translated/nonunit bounds; see decal.json for errors.\n"
                 "No original game process, same-run decal caller, pool lease, world-mode admission, compression or deletion.\n";
        scope.close();
        require(bool(scope), "Decal scope write");
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
