#include "civ5/graphics/gl_water_reflection_map.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "water_surface_native.hpp"
using namespace water_validation;
namespace fs = std::filesystem;
namespace {
struct OriginalRaster {
    D3DVIEWPORT9 viewport{};
    std::array<std::array<unsigned, 2>, 20> states{};
    std::array<unsigned, 6> clear{};
    std::array<float, 4> plane{};
    explicit OriginalRaster(const fs::path &path) {
        const auto raw = read(path);
        require(raw.size() == 236 && std::memcmp(raw.data(), "C5WRRS01", 8) == 0,
                "Original reflection raster fixture");
        unsigned count = 0;
        std::memcpy(&viewport, raw.data()+8, 24);
        std::memcpy(&count, raw.data()+32, 4);
        require(count == 20, "Complete original reflection state list");
        std::memcpy(states.data(), raw.data()+36, 160);
        std::memcpy(clear.data(), raw.data()+196, 24);
        std::memcpy(plane.data(), raw.data()+220, 16);
        require(clear[0] == 0 && clear[1] == 0, "Whole original reflection clear");
    }
};
using Vertex = std::array<float, 4>;
struct Draw {
    std::array<Vertex, 6> vertices;
    std::array<float, 4> color;
};
Draw rectangle(float x0, float x1, float y0, float y1, float depth,
               std::array<float, 4> color, bool reversed = false) {
    Vertex a{x0, y0, depth, 1}, b{x1, y0, depth, 1}, c{x1, y1, depth, 1}, d{x0, y1, depth, 1};
    Draw result{{a, c, b, a, d, c}, color};
    if (reversed) {
        std::swap(result.vertices[1], result.vertices[2]);
        std::swap(result.vertices[4], result.vertices[5]);
    }
    return result;
}
std::vector<Draw> geometry(unsigned variant) {
    const float z = variant ? .25f : .5f;
    return {rectangle(-.75f, -.125f, -.75f, .75f, z, {1, 0, 0, .5f}),
            rectangle(-.75f, -.125f, -.75f, .75f, .875f, {0, 1, 0, 1}),
            rectangle(-.75f, -.125f, -.75f, .75f, z, {0, 0, 1, .5f}),
            rectangle(.125f, .75f, -.75f, .75f, .25f, {1, 1, 0, 1}, true),
            rectangle(.25f, .5f, -.25f, .25f, .75f, {0, 1, 1, 1}),
            rectangle(-.5f, .5f, -.125f, .125f, 0.f, {1, 0, 1, .25f})};
}
struct NativeReflection : NativeSampler {
    Com<IDirect3DTexture9> map;
    Com<IDirect3DSurface9> color, pixels, depth;
    Com<IDirect3DVertexShader9> vs;
    Com<IDirect3DPixelShader9> ps;
    Com<IDirect3DVertexDeclaration9> declaration;
    explicit NativeReflection(unsigned size) {
        checked(device->CreateTexture(1024, 1024, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                                      D3DPOOL_DEFAULT, &map.value, nullptr), "Original reflection target");
        checked(map->GetSurfaceLevel(0, &color.value), "Original reflection surface");
        checked(device->CreateOffscreenPlainSurface(1024, 1024, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
                                                   &pixels.value, nullptr), "Reflection readback storage");
        checked(device->CreateDepthStencilSurface(size, size, D3DFMT_D24S8, D3DMULTISAMPLE_NONE,
                                                  0, TRUE, &depth.value, nullptr), "Explicit shared depth storage");
        for (bool vertex : {true, false}) {
            const std::string code = vertex ? "float4 main(float4 p:POSITION):POSITION{return p;}" :
                "float4 color:register(c0);float4 main():COLOR{return color;}";
            Com<ID3DBlob> binary, error;
            const auto hr = D3DCompile(code.data(), code.size(), "reflection raster diagnostic", nullptr,
                nullptr, "main", vertex ? "vs_3_0" : "ps_3_0", 0, 0, &binary.value, &error.value);
            if (FAILED(hr) && error.value)
                throw std::runtime_error(static_cast<const char *>(error->GetBufferPointer()));
            checked(hr, "Reflection diagnostic shader compile");
            const auto *words = static_cast<const DWORD *>(binary->GetBufferPointer());
            if (vertex) checked(device->CreateVertexShader(words, &vs.value), "Reflection diagnostic VS");
            else checked(device->CreatePixelShader(words, &ps.value), "Reflection diagnostic PS");
        }
        const D3DVERTEXELEMENT9 elements[]{{0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT,
                                           D3DDECLUSAGE_POSITION, 0}, D3DDECL_END()};
        checked(device->CreateVertexDeclaration(elements, &declaration.value), "Reflection diagnostic layout");
    }
    Bytes render(const OriginalRaster &raster, std::span<const Draw> draws) {
        for (unsigned i = 0; i < 4; ++i)
            checked(device->SetRenderTarget(i, i == 0 ? color.value : nullptr), "Original reflection target binding");
        for (unsigned i = 0; i < 16; ++i)
            checked(device->SetTexture(i, nullptr), "Original reflection texture unbind");
        for (auto state : {D3DRS_SCISSORTESTENABLE, D3DRS_DITHERENABLE, D3DRS_SRGBWRITEENABLE})
            checked(device->SetRenderState(state, FALSE), "Explicit device control");
        checked(device->SetDepthStencilSurface(depth.value), "Original shared depth binding");
        checked(device->Clear(0, nullptr, raster.clear[2], raster.clear[3],
            std::bit_cast<float>(raster.clear[4]), raster.clear[5]), "Original reflection clear");
        checked(device->SetViewport(&raster.viewport), "Original reflection viewport");
        for (unsigned i = 0; i < raster.states.size(); ++i) {
            checked(device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(raster.states[i][0]), raster.states[i][1]),
                    "Original reflection raster state");
            if (i == 2)
                checked(device->SetClipPlane(0, raster.plane.data()), "Original disabled clip plane");
        }
        checked(device->SetVertexShader(vs.value), "Diagnostic reflection VS binding");
        checked(device->SetPixelShader(ps.value), "Diagnostic reflection PS binding");
        checked(device->SetVertexDeclaration(declaration.value), "Diagnostic reflection layout binding");
        checked(device->BeginScene(), "Reflection diagnostic begin");
        for (const auto &draw : draws) {
            checked(device->SetPixelShaderConstantF(0, draw.color.data(), 1), "Reflection diagnostic color");
            checked(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, draw.vertices.data(), sizeof(Vertex)),
                    "Reflection diagnostic geometry");
        }
        checked(device->EndScene(), "Reflection diagnostic end");
        checked(device->GetRenderTargetData(color.value, pixels.value), "Reflection target readback");
        D3DLOCKED_RECT lock{};
        checked(pixels->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Reflection pixel lock");
        const bool valid = lock.pBits && lock.Pitch >= 4096;
        Bytes result(1024*1024*4);
        if (valid)
            for (unsigned y = 0; y < 1024; ++y)
                std::memcpy(result.data()+y*4096, static_cast<char *>(lock.pBits)+y*lock.Pitch, 4096);
        checked(pixels->UnlockRect(), "Reflection pixel unlock");
        require(valid, "Reflection pixel pitch");
        return result; // Both stored controls use native BGRA8 row order.
    }
};
struct Probe {
    GlRenderApi gl;
    GLuint program = 0, vao = 0, buffer = 0;
    GLint color = -1;
    explicit Probe(const GlContext &context) : gl(context) {
        program = gl.CreateProgram();
        const std::string_view vertex = "#version 460 core\nlayout(location=0) in vec4 p;void main(){"
            "gl_Position=vec4(p.x,-p.y,2*p.z-p.w,p.w);gl_Position.xy+=vec2(1.0/1024.0)*p.w;}";
        const std::string_view fragment = "#version 460 core\nuniform vec4 color;"
            "layout(location=0) out vec4 outColor;void main(){outColor=color;}";
        for (auto [stage, text] : {std::pair{GL_VERTEX_SHADER, vertex}, {GL_FRAGMENT_SHADER, fragment}}) {
            const auto shader = gl.CreateShader(stage);
            const auto *source = text.data(); const GLint length = text.size();
            gl.ShaderSource(shader, 1, &source, &length);
            gl.CompileShader(shader);
            GLint ok = 0;
            gl.GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
            require(ok, "GL reflection diagnostic shader");
            gl.AttachShader(program, shader); gl.DeleteShader(shader);
        }
        gl.LinkProgram(program);
        GLint ok = 0; gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
        require(ok, "GL reflection diagnostic link");
        color = gl.GetUniformLocation(program, "color");
        gl.CreateVertexArrays(1, &vao); gl.CreateBuffers(1, &buffer);
        gl.VertexArrayVertexBuffer(vao, 0, buffer, 0, sizeof(Vertex));
        gl.EnableVertexArrayAttrib(vao, 0);
        gl.VertexArrayAttribFormat(vao, 0, 4, GL_FLOAT, GL_FALSE, 0);
        gl.VertexArrayAttribBinding(vao, 0, 0);
    }
    ~Probe() {
        gl.UseProgram(0); gl.BindVertexArray(0); gl.DeleteBuffers(1, &buffer);
        gl.DeleteVertexArrays(1, &vao); gl.DeleteProgram(program);
    }
    void draw(std::span<const Draw> draws) {
        gl.UseProgram(program); gl.BindVertexArray(vao);
        for (const auto &draw : draws) {
            gl.NamedBufferData(buffer, sizeof(draw.vertices), draw.vertices.data(), GL_STREAM_DRAW);
            gl.ProgramUniform4fv(program, color, 1, draw.color.data());
            gl.DrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
};
void poison(GlRenderApi &gl) {
    for (auto state : {GL_STENCIL_TEST, GL_SCISSOR_TEST, GL_DITHER, GL_FRAMEBUFFER_SRGB,
                       GL_RASTERIZER_DISCARD, GL_CLIP_DISTANCE0, GL_POLYGON_OFFSET_FILL})
        gl.Enable(state);
    gl.Disable(GL_BLEND); gl.Disable(GL_DEPTH_TEST); gl.Disable(GL_CULL_FACE);
    gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); gl.DepthMask(GL_FALSE);
    gl.DepthFunc(GL_GREATER); gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    gl.DepthRange(.2, .8);
    gl.FrontFace(GL_CW); gl.CullFace(GL_BACK);
    gl.BlendEquationSeparate(GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT);
    gl.BlendFuncSeparate(GL_ZERO, GL_ONE, GL_ZERO, GL_ONE);
    gl.Viewport(3, 5, 7, 9);
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        require(argc == 3, "Usage: gl_water_reflection_map raster-fixture fresh-output");
        const fs::path fixture = argv[1], out = argv[2];
        require(!fs::exists(out), "Fresh reflection GPU output");
        const OriginalRaster raster(fixture / "raster.bin");
        fs::create_directories(out);
        auto context = std::make_shared<GlContext>();
        GlRenderApi gl(*context); Probe probe(*context);
        unsigned checks = 0, rejected = 0;
        const auto check = [&](bool ok, const char *why) { require(ok, why); ++checks; };
        const auto rejects = [&](auto action) {
            bool caught = false;
            try { action(); } catch (const std::invalid_argument &) { caught = true; }
            require(caught, "Missing reflection guard"); ++rejected;
        };
        for (unsigned quality : {0u, 1u, 2u, 3u, 0xffffffffu})
            for (unsigned option : {0u, 1u, 2u, 0xffffffffu}) {
                if ((quality == 1 || quality == 2) && option == 1) continue;
                GLuint name = 0;
                {
                    GlWaterReflectionMap map(context, quality, option);
                    const auto &texture = map.texture(); name = texture.id();
                    check(!map.resource().render_target && texture.levels().size() == 1 &&
                        texture.levels()[0].width == 32 && texture.levels()[0].height == 32 &&
                        texture.plan().internal_format == GL_RGBA8, "Original fallback storage");
                    check(texture.read_mip_raw(0) == Bytes(4096, 0), "Original fallback pixel bytes");
                    rejects([&] { map.render(); });
                }
                check(!context->api().IsTexture(name), "Fallback texture lifetime");
            }
        for (unsigned size : {1024u, 2048u}) {
            NativeReflection native(size);
            auto depth = std::make_shared<GlTexture>(GlTexture::render_target(*context, size, size, 1,
                                                       RenderTextureFormat::depth24_stencil8));
            const unsigned sentinel = 0x400000a7;
            std::vector<unsigned> prior(std::size_t(size)*size, sentinel);
            context->api().TextureSubImage2D(depth->id(), 0, 0, 0, size, size,
                                            GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, prior.data());
            for (unsigned quality : {1u, 2u}) {
                const auto before = depth.use_count();
                GLuint name = 0;
                {
                    GlWaterReflectionMap map(context, quality, 1, depth);
                    check(depth.use_count() == before+1, "Reflection retains shared depth ownership");
                    rejects([&] { map.texture(); });
                    for (unsigned variant = 0; variant < 3; ++variant) {
                        const auto draws = variant == 0 ? std::vector<Draw>{} : geometry(variant-1);
                        poison(gl);
                        map.render([&] {
                            GLint v = 0;
                            for (auto state : {GL_BLEND, GL_DEPTH_TEST, GL_CULL_FACE})
                                check(gl.IsEnabled(state), "Reflection required state enable");
                            for (auto state : {GL_STENCIL_TEST, GL_SCISSOR_TEST, GL_DITHER, GL_FRAMEBUFFER_SRGB,
                                               GL_RASTERIZER_DISCARD, GL_CLIP_DISTANCE0, GL_POLYGON_OFFSET_FILL})
                                check(!gl.IsEnabled(state), "Reflection clears poisoned state");
                            for (auto [state, expected] : std::initializer_list<std::pair<GLenum, GLint>>{
                                {GL_BLEND_SRC_RGB, GL_SRC_ALPHA}, {GL_BLEND_DST_RGB, GL_ONE_MINUS_SRC_ALPHA},
                                {GL_BLEND_SRC_ALPHA, GL_ONE}, {GL_BLEND_DST_ALPHA, GL_ONE},
                                {GL_DEPTH_FUNC, GL_LEQUAL}, {GL_DEPTH_WRITEMASK, 1}}) {
                                context->api().GetIntegerv(state, &v); check(v == expected, "Reflection recovered state");
                            }
                            std::array<double, 2> range{};
                            const auto get_double = reinterpret_cast<PFNGLGETDOUBLEVPROC>(context->proc_address("glGetDoublev"));
                            get_double(GL_DEPTH_RANGE, range.data());
                            check(range == std::array<double, 2>{0, 1}, "Original reflection viewport depth range");
                            rejects([&] { map.texture(); });
                            rejects([&] { map.render(); });
                            probe.draw(draws);
                        });
                        name = map.texture().id();
                        const auto actual = map.texture().read_mip_raw(0), expected = native.render(raster, draws);
                        const auto key = "s"+std::to_string(size)+"-q"+std::to_string(quality)+"-v"+std::to_string(variant);
                        write(out/(key+"-gl.bgra8"), actual); write(out/(key+"-d3d9.bgra8"), expected);
                        check(actual == expected, "Reflection clear/blend/depth/cull pixels differ from original raster");
                        GLint fbo = -1; context->api().GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fbo);
                        check(fbo == 0, "Reflection ends offscreen pass");
                        if (variant == 0) {
                            const auto raw = depth->read_mip_raw(0);
                            std::vector<unsigned> values(prior.size());
                            std::memcpy(values.data(), raw.data(), raw.size());
                            for (unsigned y = 0; y < size; ++y)
                                for (unsigned x = 0; x < size; ++x)
                                    require(values[std::size_t(y)*size+x] ==
                                        (x < 1024 && y < 1024 ? 0xffffffa7u : sentinel),
                                        "Reflection clear changed stencil or depth outside target");
                            ++checks;
                        }
                    }
                    bool thrown = false;
                    try { map.render([] { throw std::runtime_error("Explicit failed child draw"); }); }
                    catch (const std::runtime_error &) { thrown = true; }
                    check(thrown, "Failed child draw propagates");
                    rejects([&] { map.texture(); });
                    map.render();
                    check(map.texture().read_mip_raw(0) == Bytes(1024*1024*4, 0), "Reflection retry clears incomplete pixels");
                }
                check(depth.use_count() == before && !context->api().IsTexture(name), "Reflection target/depth release");
            }
            rejects([&] { GlWaterReflectionMap invalid(nullptr, 2, 1, depth); });
            rejects([&] { GlWaterReflectionMap invalid(context, 2, 1); });
        }
        for (auto [size, format] : {std::pair{512u, RenderTextureFormat::depth24_stencil8},
                                    {1024u, RenderTextureFormat::rgba8}}) {
            auto bad = std::make_shared<GlTexture>(GlTexture::render_target(*context, size, size, 1, format));
            rejects([&] { GlWaterReflectionMap invalid(context, 2, 1, bad); });
        }
        rejects([&] { GlTexture::render_target(*context, 1024, 1024, 2, RenderTextureFormat::depth24_stencil8); });
        context->check("Reflection GPU resource and state checks");
        const auto report = "{\"schema\":\"civ5-owned-water-reflection-map-v1\",\"gpuCases\":12,"
            "\"fallbackCases\":18,\"checks\":"+std::to_string(checks)+",\"rejectedInputsAndStates\":"+
            std::to_string(rejected)+",\"originalObjectDraws\":false,\"sharedDepthAllocationExplicit\":true,"
            "\"fullGraphicsParity\":false}\n";
        write(out/"producer.json", {reinterpret_cast<const std::uint8_t *>(report.data()), report.size()});
        std::cout << report;
        return 0;
    } catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
