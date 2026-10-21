// Device controls for native-format targets and automatic mip behavior.
// The pass-through shaders are validation controls, not original game shaders.
#include "civ5/graphics/gl_material_cache.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "material_blend_native.hpp"
#include <d3dcompiler.h>
#include <iomanip>
#include <iostream>
#include <thread>

using namespace civ5::graphics;
using namespace civ5::materials;
using namespace blend_validation;
namespace fs = std::filesystem;
namespace {
unsigned guards = 0;
void check(bool ok, const char *why) {
    ++guards;
    require(ok, why);
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try {
        f();
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, "Missing cache resource input rejection");
}
constexpr char pixel_source[] = R"(sampler2D inputTexture : register(s0);
float4 sampleLevel : register(c0);
float4 main(float2 uv : TEXCOORD0) : COLOR0 {
    return tex2Dlod(inputTexture, float4(uv, 0, sampleLevel.x));
})";
struct Native {
    OwnWindow window;
    Com<IDirect3D9> api{Direct3DCreate9(D3D_SDK_VERSION)};
    Com<IDirect3DDevice9> device;
    Com<IDirect3DPixelShader9> shader;
    D3DDISPLAYMODE mode{};
    D3DADAPTER_IDENTIFIER9 adapter{};
    Native() {
        require(api.value, "Own material cache D3D9 API");
        hr(api->GetAdapterDisplayMode(0, &mode), "Native adapter display mode");
        hr(api->GetAdapterIdentifier(0, 0, &adapter), "Native adapter identity");
        D3DPRESENT_PARAMETERS pp{};
        pp.BackBufferWidth = pp.BackBufferHeight = 128;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferCount = 1;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = window.value;
        pp.Windowed = TRUE;
        pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        hr(api->CreateDevice(0, D3DDEVTYPE_HAL, window.value, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp,
                             &device.value),
           "Own native cache device");
        Com<ID3DBlob> code, errors;
        const auto status = D3DCompile(
            pixel_source, sizeof(pixel_source) - 1, "cache-format-control", nullptr, nullptr, "main",
            "ps_3_0", D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, 0, &code.value, &errors.value);
        if (FAILED(status) && errors.value)
            throw std::runtime_error(static_cast<const char *>(errors->GetBufferPointer()));
        hr(status, "Compile cache device control");
        hr(device->CreatePixelShader(static_cast<const DWORD *>(code->GetBufferPointer()), &shader.value),
           "Create cache device control");
        hr(device->SetDepthStencilSurface(nullptr), "No cache control depth");
        for (auto [state, value] :
             std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{{D3DRS_ZENABLE, FALSE},
                                                                         {D3DRS_CULLMODE, D3DCULL_NONE},
                                                                         {D3DRS_ALPHABLENDENABLE, FALSE},
                                                                         {D3DRS_ALPHATESTENABLE, FALSE},
                                                                         {D3DRS_LIGHTING, FALSE},
                                                                         {D3DRS_SCISSORTESTENABLE, FALSE},
                                                                         {D3DRS_DITHERENABLE, FALSE},
                                                                         {D3DRS_SRGBWRITEENABLE, FALSE},
                                                                         {D3DRS_COLORWRITEENABLE, 15}})
            hr(device->SetRenderState(state, value), "Native cache control state");
        for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                 {D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP},
                 {D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP},
                 {D3DSAMP_MINFILTER, D3DTEXF_POINT},
                 {D3DSAMP_MAGFILTER, D3DTEXF_POINT},
                 {D3DSAMP_MIPFILTER, D3DTEXF_POINT},
                 {D3DSAMP_MAXMIPLEVEL, 0},
                 {D3DSAMP_SRGBTEXTURE, FALSE}})
            hr(device->SetSamplerState(0, state, value), "Native cache control sampler");
    }
    void draw(IDirect3DTexture9 *source, IDirect3DSurface9 *destination, unsigned width, unsigned height,
              unsigned mip) {
        hr(device->SetTexture(0, nullptr), "Unbind prior native cache sample");
        hr(device->SetRenderTarget(0, destination), "Native cache control target");
        hr(device->SetTexture(0, source), "Native cache control source");
        D3DVIEWPORT9 view{0, 0, width, height, 0, 1};
        hr(device->SetViewport(&view), "Native cache control viewport");
        hr(device->SetVertexShader(nullptr), "Native pretransformed control vertices");
        hr(device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1), "Native control declaration");
        hr(device->SetPixelShader(shader.value), "Native control shader");
        const float level[]{float(mip), 0, 0, 0};
        hr(device->SetPixelShaderConstantF(0, level, 1), "Native explicit control mip");
        struct V {
            float x, y, z, w, u, v;
        };
        const V vertices[]{{-.5f, -.5f, 0, 1, 0, 0},
                           {float(width) - .5f, -.5f, 0, 1, 1, 0},
                           {-.5f, float(height) - .5f, 0, 1, 0, 1},
                           {float(width) - .5f, float(height) - .5f, 0, 1, 1, 1}};
        hr(device->BeginScene(), "Native cache control scene");
        const auto status = device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(V));
        hr(device->EndScene(), "Native cache control scene end");
        hr(status, "Native cache control draw");
    }
    Bytes raw(IDirect3DSurface9 *surface, D3DFORMAT format, unsigned width, unsigned height, unsigned bpp) {
        Com<IDirect3DSurface9> staging;
        hr(device->CreateOffscreenPlainSurface(width, height, format, D3DPOOL_SYSTEMMEM, &staging.value,
                                               nullptr),
           "Native cache raw staging");
        hr(device->GetRenderTargetData(surface, staging.value), "Native cache raw readback");
        D3DLOCKED_RECT lock{};
        hr(staging->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Native cache raw lock");
        Bytes result(std::size_t(width) * height * bpp);
        for (unsigned y = 0; y < height; ++y)
            std::memcpy(result.data() + std::size_t(y) * width * bpp,
                        static_cast<const std::uint8_t *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                        width * bpp);
        hr(staging->UnlockRect(), "Native cache raw unlock");
        return result;
    }
};
struct GlControl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint program = 0, vao = 0, sampler = 0, fbo = 0;
    GLint lod = -1;
    explicit GlControl(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        program = gl.CreateProgram();
        const char *vertex = R"(#version 460 core
out vec2 uv;
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);uv=p;gl_Position=vec4(p*2.0-1.0,0,1);})";
        const char *fragment = R"(#version 460 core
layout(binding=0) uniform sampler2D sourceTexture;
uniform float sampleLevel;in vec2 uv;layout(location=0) out vec4 color;
void main(){color=textureLod(sourceTexture,uv,sampleLevel);})";
        for (auto [kind, source] :
             {std::pair{GL_VERTEX_SHADER, vertex}, std::pair{GL_FRAGMENT_SHADER, fragment}}) {
            const auto s = gl.CreateShader(kind);
            gl.ShaderSource(s, 1, &source, nullptr);
            gl.CompileShader(s);
            GLint ok = 0;
            gl.GetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[8192]{};
                gl.GetShaderInfoLog(s, sizeof(log), nullptr, log);
                gl.DeleteShader(s);
                throw std::runtime_error(log);
            }
            gl.AttachShader(program, s);
            gl.DeleteShader(s);
        }
        gl.LinkProgram(program);
        GLint ok = 0;
        gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
        require(ok, "Link GL cache control");
        lod = gl.GetUniformLocation(program, "sampleLevel");
        gl.CreateVertexArrays(1, &vao);
        gl.CreateSamplers(1, &sampler);
        gl.CreateFramebuffers(1, &fbo);
        for (auto [name, value] :
             {std::pair{GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST},
              std::pair{GL_TEXTURE_MAG_FILTER, GL_NEAREST}, std::pair{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
              std::pair{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE}})
            gl.SamplerParameteri(sampler, name, value);
    }
    ~GlControl() {
        context->api();
        gl.DeleteFramebuffers(1, &fbo);
        gl.DeleteSamplers(1, &sampler);
        gl.DeleteVertexArrays(1, &vao);
        gl.DeleteProgram(program);
    }
    void draw(GLuint source, GLuint target, unsigned width, unsigned height, unsigned mip) {
        const auto &api = context->api();
        api.BindTextureUnit(0, 0);
        gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, target, 0);
        const GLenum buffer = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(fbo, 1, &buffer);
        require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "GL native-format cache framebuffer incomplete");
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl.Viewport(0, 0, width, height);
        for (auto state : {GL_BLEND, GL_CULL_FACE, GL_DEPTH_TEST, GL_SCISSOR_TEST, GL_DITHER,
                           GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
            gl.Disable(state);
        gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        api.BindTextureUnit(0, source);
        gl.BindSampler(0, sampler);
        gl.ProgramUniform1f(program, lod, float(mip));
        gl.UseProgram(program);
        gl.BindVertexArray(vao);
        gl.DrawArrays(GL_TRIANGLES, 0, 3);
        context->check("GL cache format control draw");
    }
};
std::vector<float> pattern(unsigned width, unsigned height, unsigned kind) {
    std::vector<float> values(std::size_t(width) * height * 4);
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = 0; x < width; ++x)
            for (unsigned c = 0; c < 4; ++c) {
                const auto n = (x * 71 + y * 113 + c * 179 + x * y * 13) % 4096;
                float value = float(n) / 4095.f;
                if (kind == 1)
                    value = float(int(n) - 2048) / 1024.f;
                if (kind == 2)
                    value = (float(n % 257) + .5f) / float(c == 1 ? 63 : 31);
                values[(std::size_t(y) * width + x) * 4 + c] = value;
            }
    return values;
}
void ownership_controls(std::shared_ptr<GlContext> context) {
    using R = MaterialCacheRole;
    const auto plans = material_cache_texture_plan(17, 9, false);
    rejects([&] { GlMaterialCacheTextures invalid(nullptr, plans, {true, true, true}); });
    rejects([&] {
        GlMaterialCacheTextures invalid(context, material_cache_texture_plan(17, 9, true),
                                        {true, true, true});
    });
    auto repeated = plans;
    repeated.push_back(plans.front());
    rejects([&] { GlMaterialCacheTextures invalid(context, repeated, {true, true, true}); });
    auto bad = plans;
    bad.back().requested_levels = 99;
    rejects([&] { GlMaterialCacheTextures invalid(context, bad, {true, true, true}); });
    auto original =
        std::make_unique<GlMaterialCacheTextures>(context, plans, MaterialCacheMipPolicy{true, false, true});
    auto moved = std::move(*original);
    rejects([&] { original->texture(R::height_temporary); });
    check(moved.texture(R::height_temporary).levels().size() == 5, "Float cache effective mips");
    check(moved.texture(R::normal_temporary).levels().size() == 1, "UNORM cache effective base-only request");
    rejects([&] { moved.texture(R::patch_64); });
    bool other_thread = false;
    std::thread worker([&] {
        try {
            moved.texture(R::normal_temporary).read_level_raw(0, 0);
        } catch (const std::exception &) {
            other_thread = true;
        }
    });
    worker.join();
    check(other_thread, "Cache GPU readback must retain thread affinity");
    auto retained = std::make_shared<GlContext>();
    std::weak_ptr<GlContext> weak = retained;
    auto owned = std::make_unique<GlMaterialCacheTextures>(retained, plans,
                                                           MaterialCacheMipPolicy{false, false, false});
    retained.reset();
    check(!weak.expired(), "Cache must retain creating context");
    check(owned->texture(R::height_temporary).belongs_to(*weak.lock()), "Retained cache context identity");
    rejects([&] { owned->rebuild_context(nullptr); });
    rejects([&] { owned->rebuild_context(weak.lock()); });
    auto replacement = std::make_shared<GlContext>();
    owned->rebuild_context(replacement);
    check(owned->texture(R::height_temporary).belongs_to(*replacement),
          "Cache replacement context identity");
    check(weak.expired(), "Cache replacement retained the old context");
    replacement->check("Cache replacement context");
    owned.reset();
    replacement->check("Cache replacement context released");
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 2, "Usage: civ5_gl_material_cache fresh-output");
        const fs::path output(argv[1]);
        require(!fs::exists(output), "Fresh cache GPU output required");
        auto context = std::make_shared<GlContext>();
        GlControl gl(context);
        Native native;
        ownership_controls(context);
        require(fs::create_directories(output), "Create cache GPU output");
        std::ofstream report(output / "gpu.json");
        report << "{\"schema\":\"civ5-material-cache-gpu-control-v1\",\"gl\":"
               << std::quoted(context->description())
               << ",\"d3d\":" << std::quoted(native.adapter.Description) << ",\"guards\":" << guards
               << ",\"originalGameProcessAccess\":false,\"originalShadersExecuted\":false,\"cases\":[";
        unsigned ordinal = 0;
        for (auto [format, role, bpp] :
             {std::tuple{D3DFMT_A16B16G16R16F, MaterialCacheRole::height_temporary, 8u},
              std::tuple{D3DFMT_A8R8G8B8, MaterialCacheRole::normal_temporary, 4u},
              std::tuple{D3DFMT_R5G6B5, MaterialCacheRole::patch_64, 2u}}) {
            const auto capability = native.api->CheckDeviceFormat(
                0, D3DDEVTYPE_HAL, native.mode.Format, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP,
                D3DRTYPE_TEXTURE, format);
            require(capability == D3D_OK || capability == D3DOK_NOAUTOGEN,
                    "Native cache format not supported");
            for (auto [width, height] : {std::pair{64u, 64u}, std::pair{127u, 65u}})
                for (unsigned kind = 0; kind < 3; ++kind) {
                    auto values = pattern(width, height, kind);
                    Com<IDirect3DTexture9> source, target;
                    hr(native.device->CreateTexture(width, height, 1, 0, D3DFMT_A32B32G32R32F,
                                                    D3DPOOL_MANAGED, &source.value, nullptr),
                       "Native cache float source");
                    D3DLOCKED_RECT lock{};
                    hr(source->LockRect(0, &lock, nullptr, 0), "Native control source lock");
                    for (unsigned y = 0; y < height; ++y)
                        std::memcpy(static_cast<std::uint8_t *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                                    values.data() + std::size_t(y) * width * 4, width * 16);
                    hr(source->UnlockRect(0), "Native control source unlock");
                    hr(native.device->CreateTexture(width, height, 0,
                                                    D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, format,
                                                    D3DPOOL_DEFAULT, &target.value, nullptr),
                       "Native original-format cache target");
                    Com<IDirect3DSurface9> surface;
                    hr(target->GetSurfaceLevel(0, &surface.value), "Native cache top level");
                    MaterialCacheTexturePlan plan{
                        role, 0, width, height, 0, static_cast<MaterialCacheFormat>(format), 0x401, 0, 0};
                    const bool automatic = capability == D3D_OK;
                    GlMaterialCacheTextures cache(context, std::span(&plan, 1),
                                                  {automatic, automatic, automatic});
                    GLuint gl_source = 0, gl_sampled = 0;
                    const auto &api = context->api();
                    api.CreateTextures(GL_TEXTURE_2D, 1, &gl_source);
                    api.TextureStorage2D(gl_source, 1, GL_RGBA32F, width, height);
                    api.TextureSubImage2D(gl_source, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT,
                                          values.data());
                    api.TextureParameteri(gl_source, GL_TEXTURE_MAX_LEVEL, 0);
                    native.draw(source.value, surface.value, width, height, 0);
                    gl.draw(gl_source, cache.texture(role).id(), width, height, 0);
                    const auto prefix = "case-" + std::to_string(ordinal);
                    write(output / (prefix + "-native-base.bin"),
                          native.raw(surface.value, format, width, height, bpp));
                    write(output / (prefix + "-gl-base.bin"), cache.texture(role).read_level_raw(0, 0));
                    cache.generate_mips(role);
                    if (ordinal)
                        report << ',';
                    report << "{\"index\":" << ordinal << ",\"format\":" << format << ",\"width\":" << width
                           << ",\"height\":" << height << ",\"pattern\":" << kind
                           << ",\"checkDeviceFormat\":" << std::uint32_t(capability)
                           << ",\"nativeVisibleLevels\":" << target->GetLevelCount()
                           << ",\"nativeAutoFilter\":" << target->GetAutoGenFilterType()
                           << ",\"effectiveLevels\":" << cache.texture(role).levels().size() << ",\"mips\":[";
                    unsigned mip = 0;
                    for (const auto &level : cache.texture(role).levels()) {
                        Com<IDirect3DSurface9> sampled;
                        hr(native.device->CreateRenderTarget(level.width, level.height, D3DFMT_A32B32G32R32F,
                                                             D3DMULTISAMPLE_NONE, 0, FALSE, &sampled.value,
                                                             nullptr),
                           "Native cache sampled target");
                        api.CreateTextures(GL_TEXTURE_2D, 1, &gl_sampled);
                        api.TextureStorage2D(gl_sampled, 1, GL_RGBA32F, level.width, level.height);
                        native.draw(target.value, sampled.value, level.width, level.height, mip);
                        gl.draw(cache.texture(role).id(), gl_sampled, level.width, level.height, mip);
                        const auto name = prefix + "-mip-" + std::to_string(mip);
                        write(output / (name + "-native.bin"),
                              native.raw(sampled.value, D3DFMT_A32B32G32R32F, level.width, level.height, 16));
                        Bytes raw(std::size_t(level.width) * level.height * 16);
                        api.GetTextureImage(gl_sampled, 0, GL_RGBA, GL_FLOAT,
                                            static_cast<GLsizei>(raw.size()), raw.data());
                        context->check("Read cache mip control");
                        write(output / (name + "-gl.bin"), raw);
                        if (mip)
                            report << ',';
                        report << "{\"level\":" << mip << ",\"width\":" << level.width
                               << ",\"height\":" << level.height << '}';
                        api.DeleteTextures(1, &gl_sampled);
                        gl_sampled = 0;
                        ++mip;
                    }
                    report << "]}";
                    api.DeleteTextures(1, &gl_source);
                    std::cout << "PASS cache GPU control " << ordinal << " format=" << format
                              << " size=" << width << 'x' << height << " levels=" << mip << '\n';
                    ++ordinal;
                }
        }
        report << "],\"fullGraphicsParity\":false}\n";
        report.close();
        require(bool(report), "Cache GPU report write");
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
