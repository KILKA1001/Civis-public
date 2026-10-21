#include "civ5/graphics/gl_water_current.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/material_sources.hpp"
#include "water_current_native.hpp"
#include <cmath>
#include <limits>

using namespace water_validation;
using water_current_validation::NativeCurrent;
namespace fs = std::filesystem;
namespace {
constexpr unsigned extent = 256;
struct Control {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint fbo = 0;
    std::array<GLuint, 2> target{};
    std::array<GLuint, 2> sources{};
    explicit Control(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        gl.CreateFramebuffers(1, &fbo);
        context->api().CreateTextures(GL_TEXTURE_2D, 2, target.data());
        context->api().CreateTextures(GL_TEXTURE_2D, 2, sources.data());
        for (unsigned i = 0; i < 2; ++i)
            context->api().TextureStorage2D(target[i], 1, i == 0 ? GL_RGBA32F : GL_RGBA8, extent, extent);
    }
    ~Control() {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &fbo);
        context->api().DeleteTextures(2, target.data());
        context->api().DeleteTextures(2, sources.data());
    }
    void begin(unsigned format) {
        gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, target[format], 0);
        const GLenum buffer = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(fbo, 1, &buffer);
        require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Current FBO");
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl.Viewport(0, 0, extent, extent);
        for (auto state : {GL_DEPTH_TEST, GL_BLEND, GL_CULL_FACE, GL_SCISSOR_TEST, GL_DITHER,
                           GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
            gl.Disable(state);
        const float sentinel[]{187 / 255.f, 51 / 255.f, 119 / 255.f, 1};
        gl.ClearNamedFramebufferfv(fbo, GL_COLOR, 0, sentinel);
    }
    Bytes end(unsigned format) {
        Bytes result(extent * extent * (format == 0 ? 16 : 4));
        context->api().GetTextureImage(target[format], 0, GL_RGBA, format == 0 ? GL_FLOAT : GL_UNSIGNED_BYTE,
                                       static_cast<GLsizei>(result.size()), result.data());
        context->check("Current GL readback");
        return result;
    }
};
// Prefixes only observe diagnostic registers; they never replace the full pair.
struct Probe {
    GlRenderApi &gl;
    GLuint program = 0;
    Probe(GlRenderApi &api, std::string_view fragment) : gl(api) {
        program = gl.CreateProgram();
        for (auto [stage, text] : {std::pair{GL_VERTEX_SHADER, water_current_vertex_shader()},
                                  {GL_FRAGMENT_SHADER, fragment}}) {
            const auto shader = gl.CreateShader(stage);
            const auto *source = text.data();
            const auto length = GLint(text.size());
            gl.ShaderSource(shader, 1, &source, &length);
            gl.CompileShader(shader);
            GLint ok = 0;
            gl.GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[8192]{};
                gl.GetShaderInfoLog(shader, sizeof(log), nullptr, log);
                gl.DeleteShader(shader);
                throw std::runtime_error(log);
            }
            gl.AttachShader(program, shader);
            gl.DeleteShader(shader);
        }
        gl.LinkProgram(program);
        GLint ok = 0;
        gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
        require(ok, "Current diagnostic link");
    }
    ~Probe() { gl.UseProgram(0); gl.DeleteProgram(program); }
    void draw(const WaterCurrentConstants &c) {
        const auto location = gl.GetUniformLocation(program, "uP[0]");
        if (location >= 0) gl.ProgramUniform4fv(program, location, 4, c.data());
        const float center[]{1.f / extent, 1.f / extent};
        gl.ProgramUniform2fv(program, gl.GetUniformLocation(program, "uPixelCenter"), 1, center);
        gl.UseProgram(program);
        // Renderer has bound its already verified shared quad and source sampler.
        gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }
};
unsigned guards(Control &gpu, GlWaterCurrentRenderer &renderer, GLuint source) {
    unsigned count = 0;
    const auto rejects = [&](auto action) {
        bool rejected = false;
        try { action(); } catch (const std::invalid_argument &) { rejected = true; }
        require(rejected, "Missing current renderer input rejection");
        gpu.context->check("Rejected current input leaves GL clean");
        ++count;
    };
    const auto constants = water_current_constants(40, 28, {});
    gpu.begin(0);
    renderer.draw(constants, source, extent, extent);
    const auto baseline = gpu.end(0);
    rejects([&] { GlWaterCurrentRenderer bad(nullptr); });
    rejects([&] { renderer.draw(constants, source, 0, extent); });
    rejects([&] { renderer.draw(constants, source, extent, -1); });
    rejects([&] { renderer.draw(constants, 0, extent, extent); });
    rejects([&] { renderer.draw(constants, gpu.target[0], extent, extent); });
    for (unsigned index : {0u, 1u, 4u, 8u, 12u}) {
        auto bad = constants;
        bad[index] = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { renderer.draw(bad, source, extent, extent); });
    }
    for (auto type : {GL_TEXTURE_2D, GL_TEXTURE_3D}) {
        GLuint empty = 0;
        gpu.context->api().CreateTextures(type, 1, &empty);
        rejects([&] { renderer.draw(constants, empty, extent, extent); });
        gpu.context->api().DeleteTextures(1, &empty);
    }
    auto unused = constants;
    for (unsigned index : {2u, 3u, 5u, 6u, 7u, 9u, 10u, 11u, 13u, 14u, 15u})
        unused[index] = std::bit_cast<float>(0x7fc00000u + index);
    renderer.draw(unused, source, extent, extent);
    require(gpu.end(0) == baseline, "Unused original current CB components changed output");
    return count + 1;
}
}
int wmain(int argc, wchar_t **argv) {
    try {
        require(argc == 4 || argc == 5, "Usage: civ5_gl_water_current original-shaders original-quad-fixture fresh-output [diagnostic-prefixes]");
        const fs::path shaders(argv[1]), fixture(argv[2]), out(argv[3]);
        require(!fs::exists(out), "Fresh current GPU output");
        auto context = std::make_shared<GlContext>();
        Control gpu(context);
        NativeCurrent native(shaders, fixture);
        GlWaterCurrentRenderer renderer(context);
        fs::create_directories(out);
        for (auto [name, source] : {std::pair{"current-vs.glsl", water_current_vertex_shader()},
                                    {"current-ps.glsl", water_current_fragment_shader()}})
            write(out / name, {reinterpret_cast<const std::uint8_t *>(source.data()), source.size()});
        const auto quad = water_current_quad();
        save(out / "vertices.bin", quad.vertices);
        save(out / "indices.bin", quad.indices);
        const auto levels = material_resource_zero();
        auto zero = GlTexture::rgba8(*context, levels);
        std::array<Com<IDirect3DTexture9>, 3> sources;
        checked(native.device->CreateTexture(256, 256, 7, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                             &sources[0].value, nullptr), "Original resource0");
        for (unsigned mip = 0; mip < 7; ++mip) {
            const auto &level = levels[mip];
            require(zero.read_level_raw(0, mip) == level.rgba, "Resource0 GL bytes");
            auto bgra = level.rgba;
            for (std::size_t i = 0; i < bgra.size(); i += 4)
                std::swap(bgra[i], bgra[i + 2]);
            native.upload(sources[0].value, mip, bgra, level.width, level.height, 4);
            write(out / ("resource0-mip" + std::to_string(mip) + ".rgba"), level.rgba);
        }
        for (unsigned source = 1; source < 3; ++source) {
            std::vector<float> values(32 * 32 * 4);
            for (unsigned y = 0; y < 32; ++y)
                for (unsigned x = 0; x < 32; ++x) {
                    const unsigned at = (y * 32 + x) * 4;
                    values[at] = source == 1 ? 1.f : float((x * 7 + y * 11) % 32) / 32;
                    values[at + 1] = .25f;
                    values[at + 2] = .75f;
                    values[at + 3] = 1.f;
                }
            const auto texture = gpu.sources[source - 1];
            context->api().TextureStorage2D(texture, 1, GL_RGBA32F, 32, 32);
            context->api().TextureSubImage2D(texture, 0, 0, 0, 32, 32, GL_RGBA, GL_FLOAT, values.data());
            std::vector<float> readback(values.size());
            context->api().GetTextureImage(texture, 0, GL_RGBA, GL_FLOAT, sizeof(float) * readback.size(), readback.data());
            require(readback == values, "Current float source GL identity");
            checked(native.device->CreateTexture(32, 32, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED,
                                                 &sources[source].value, nullptr), "Current float source");
            native.upload(sources[source].value, 0,
                          {reinterpret_cast<const std::uint8_t *>(values.data()), values.size() * 4}, 32, 32, 16);
            save(out / ("source" + std::to_string(source) + ".f32"), values);
        }
        unsigned cases = 0;
        for (unsigned grid = 0; grid < 3; ++grid) {
            const auto cb = read(fixture / ("constants" + std::to_string(grid) + ".bin"));
            require(cb.size() == 64, "Original current constant block");
            WaterCurrentConstants original{};
            std::memcpy(original.data(), cb.data(), cb.size());
            for (unsigned variant = 0; variant < 4; ++variant) {
                auto settings = WaterCurrentSettings{};
                if (variant == 1) settings.noise_strength = 0;
                if (variant == 2) settings.flow_strength = 0;
                if (variant == 3) settings = {.37f, -.125f, 4.f};
                auto c = water_current_constants(int(original[0]), int(original[1]), settings, original);
                auto reference = original;
                // Explicit alternate controls; variant0 retains executed original constants.
                if (variant != 0) {
                    reference[12] = settings.flow_strength;
                    reference[4] = settings.noise_strength;
                    reference[8] = settings.noise_scale;
                }
                require(std::memcmp(c.data(), reference.data(), 64) == 0, "Independent original current CB");
                for (unsigned source = 0; source < 3; ++source) {
                    const auto key = "g" + std::to_string(grid) + "-v" + std::to_string(variant) + "-s" + std::to_string(source);
                    save(out / (key + "-constants.bin"), c);
                    for (unsigned format = 0; format < 2; ++format) {
                        const auto suffix = format == 0 ? "-f32" : "-rgba8";
                        const auto dx = native.render(reference, sources[source].value, format);
                        gpu.begin(format);
                        renderer.draw(c, source == 0 ? zero.id() : gpu.sources[source - 1], extent, extent);
                        const auto gl = gpu.end(format);
                        write(out / (key + suffix + "-d3d9.bin"), dx);
                        write(out / (key + suffix + "-gl.bin"), gl);
                    }
                    ++cases;
                }
            }
        }
        const auto guard_count = guards(gpu, renderer, zero.id());
        if (argc == 5) {
            const fs::path probes(argv[4]);
            fs::create_directories(out / "diagnostic");
            const auto cb = read(fixture / "constants0.bin");
            WaterCurrentConstants c{};
            std::memcpy(c.data(), cb.data(), 64);
            for (const auto &file : fs::directory_iterator(probes)) {
                if (file.path().extension() != ".bin") continue;
                const auto bytes = read(file.path());
                require(bytes.size() >= 8 && bytes.size() % 4 == 0, "Aligned diagnostic shader");
                std::vector<DWORD> words(bytes.size() / 4);
                std::memcpy(words.data(), bytes.data(), bytes.size());
                Com<IDirect3DPixelShader9> ps;
                checked(native.device->CreatePixelShader(words.data(), &ps.value), "Current diagnostic prefix");
                const auto glsl = read(probes / (file.path().stem().string() + ".glsl"));
                Probe probe(gpu.gl, {reinterpret_cast<const char *>(glsl.data()), glsl.size()});
                const auto dx = native.render(c, sources[0].value, 0, ps.value);
                gpu.begin(0);
                renderer.draw(c, zero.id(), extent, extent);
                probe.draw(c);
                const auto gl = gpu.end(0);
                write(out / "diagnostic" / (file.path().stem().string() + "-d3d9.bin"), dx);
                write(out / "diagnostic" / (file.path().stem().string() + "-gl.bin"), gl);
            }
        }
        const std::string report = "{\n\"schema\":\"civ5-water-current-gpu-v1\",\n\"cases\":" +
            std::to_string(cases) + ",\n\"guards\":" + std::to_string(guard_count) +
            ",\n\"width\":256,\n\"height\":256,\n\"explicitControlledRaster\":true,\n"
            "\"originalGameProcessAccess\":false,\n\"gpu\":\"" + context->description() + "\"\n}\n";
        write(out / "producer.json", {reinterpret_cast<const std::uint8_t *>(report.data()), report.size()});
        std::cout << report;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
