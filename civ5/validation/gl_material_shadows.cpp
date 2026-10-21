#include "civ5/graphics/gl_material_shadows.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include "civ5/terrain/height_addressing.hpp"
#include "material_shadow_native.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>
using namespace shadow_validation;
using namespace civ5::graphics;
namespace fs = std::filesystem;
namespace {
int checks = 0;
void check(bool ok, const char *why) {
    ++checks;
    require(ok, why);
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "Missing shadow input rejection");
}
std::span<const std::uint8_t> bytes(std::span<const float> v) {
    return {reinterpret_cast<const std::uint8_t *>(v.data()), v.size_bytes()};
}
const std::array<const char *, 6> names{"flat", "ridge-x", "ridge-y", "water", "blocks", "mixed"};
const std::array<std::array<float, 3>, 4> suns{{{1, 0, .5f}, {0, 1, .5f}, {.75f, 1, .5f}, {-1, -.5f, .5f}}};
std::vector<float> field(unsigned kind) {
    std::vector<float> values(64 * 64 * 4);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            for (int c = 0; c < 3; ++c)
                values[(y * 64 + x) * 4 + c] = ((x * 7 + y * 3 + c * 11) % 64) / 64.f;
            float alpha = 0;
            switch (kind) {
            case 0:
                alpha = .375f;
                break;
            case 1:
                alpha = x >= 28 && x <= 35 ? .875f : .25f;
                break;
            case 2:
                alpha = y >= 28 && y <= 35 ? .875f : .25f;
                break;
            case 3:
                alpha = x >= 28 && x <= 35 ? .5f : .125f;
                break;
            case 4:
                alpha = (x / 8 + y / 8) % 2 == 0 ? .25f : .875f;
                break;
            case 5:
                alpha = ((x * 13 + y * 7 + x * y) % 64) / 64.f;
                break;
            default:
                throw std::runtime_error("Shadow field index");
            }
            values[(y * 64 + x) * 4 + 3] = alpha;
        }
    return values;
}
civ5::terrain::HeightField height_source(std::span<const std::uint8_t> data) {
    std::vector<std::int32_t> common;
    int width = 0, height = 0;
    unsigned count = 0;
    for (const auto &record : generation_validation::records(data))
        if (record.op == 5) {
            generation_validation::Reader r{record.input};
            const generation_validation::Source source(r);
            if (count++ == 0) {
                width = source.width;
                height = source.height;
                common = source.heights;
            } else
                require(width == source.width && height == source.height && common == source.heights,
                        "Full patches do not share the same U16 source");
        }
    require(count == 3 && width > 0 && height > 0, "Missing complete source corpus");
    std::vector<std::uint16_t> source;
    source.reserve(common.size());
    for (auto v : common) {
        require(v >= 0 && v <= 65535, "Non-U16 source height");
        source.push_back(static_cast<std::uint16_t>(v));
    }
    return {civ5::terrain::HeightDomain(width, height), source};
}
struct FloatSource {
    std::shared_ptr<GlContext> context;
    GLuint id = 0;
    FloatSource(std::shared_ptr<GlContext> c, unsigned size, std::span<const float> values)
        : context(std::move(c)) {
        require(values.size() == std::size_t(size) * size * 4, "Float shadow source extent");
        const auto &api = context->api();
        try {
            api.CreateTextures(GL_TEXTURE_2D, 1, &id);
            api.TextureStorage2D(id, 1, GL_RGBA32F, size, size);
            api.TextureSubImage2D(id, 0, 0, 0, size, size, GL_RGBA, GL_FLOAT, values.data());
            std::vector<float> actual(values.size());
            api.GetTextureImage(id, 0, GL_RGBA, GL_FLOAT, static_cast<GLsizei>(values.size_bytes()),
                                actual.data());
            context->check("Shadow float source upload/readback");
            require(std::memcmp(actual.data(), values.data(), values.size_bytes()) == 0,
                    "GL shadow source bytes differ");
        } catch (...) {
            api.DeleteTextures(1, &id);
            id = 0;
            throw;
        }
    }
    ~FloatSource() { context->api().DeleteTextures(1, &id); }
    FloatSource(const FloatSource &) = delete;
};
struct Control {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint fbo = 0, target = 0;
    explicit Control(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        try {
            const auto &api = context->api();
            gl.CreateFramebuffers(1, &fbo);
            api.CreateTextures(GL_TEXTURE_2D, 1, &target);
            api.TextureStorage2D(target, 1, GL_RGBA32F, 64, 64);
            gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, target, 0);
            const GLenum buffer = GL_COLOR_ATTACHMENT0;
            gl.NamedFramebufferDrawBuffers(fbo, 1, &buffer);
            require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                    "Shadow framebuffer");
            gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
            gl.Viewport(0, 0, 64, 64);
            for (auto state : {GL_BLEND, GL_DEPTH_TEST, GL_CULL_FACE, GL_SCISSOR_TEST, GL_DITHER,
                               GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
                gl.Disable(state);
            context->check("Own macro-shadow control target");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &fbo);
        fbo = 0;
        context->api().DeleteTextures(1, &target);
        target = 0;
    }
    ~Control() { release(); }
    void clear(float value) {
        const float color[]{value, value, value, value};
        gl.ClearNamedFramebufferfv(fbo, GL_COLOR, 0, color);
    }
    std::vector<float> readback() {
        std::vector<float> output(64 * 64 * 4);
        context->api().GetTextureImage(target, 0, GL_RGBA, GL_FLOAT, 64 * 64 * 16, output.data());
        context->check("Shadow target readback");
        return output;
    }
    std::vector<float> draw(GlMaterialShadowRenderer &renderer, GLuint source,
                            std::span<const MaterialShadowParameters> samples, bool accumulated,
                            const std::array<float, 2> &offset = {0, 0}) {
        clear(accumulated ? 0 : std::numeric_limits<float>::quiet_NaN());
        if (accumulated)
            renderer.draw_accumulated(source, samples, 64, 64);
        else {
            require(samples.size() == 1, "Single shadow sample extent");
            renderer.draw(source, samples[0], 64, 64, offset);
        }
        auto output = readback();
        const float bound = accumulated ? 1 : samples[0].jitter_add[2];
        for (std::size_t i = 0; i < output.size(); i += 4)
            require(std::isfinite(output[i]) && output[i] >= 0 && output[i] <= bound &&
                        output[i] == output[i + 1] && output[i] == output[i + 2] &&
                        output[i] == output[i + 3],
                    "Uncovered/nonfinite/out-of-range GL shadow");
        return output;
    }
};
MaterialShadowParameters parameters(bool computed, unsigned direction, std::uint32_t count,
                                    const std::array<float, 2> &jitter = {.125f, -.25f}) {
    const auto size = computed ? 1024.f : 64.f;
    return material_shadow_parameters(
        {1, .866f, .2f}, suns[direction], {1.f / size, (computed ? -1.f : 1.f) / size},
        computed ? std::array<std::int32_t, 2>{64, 64} : std::array<std::int32_t, 2>{0, 0}, jitter, count,
        93);
}
void guards(std::shared_ptr<GlContext> context, Control &c, const MaterialPatchMesh &mesh) {
    const auto p = parameters(false, 0, 1);
    const FloatSource source(context, 64, field(5));
    rejects([&] { GlMaterialShadowRenderer invalid(nullptr, mesh); });
    auto moved = mesh;
    const auto retained_mesh = std::move(moved);
    rejects([&] { GlMaterialShadowRenderer invalid(context, moved); });
    auto vertices = Bytes(mesh.vertices().begin(), mesh.vertices().end());
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    std::memcpy(vertices.data() + 12, &nan, 4);
    const MaterialPatchMesh bad_uv(mesh.columns(), mesh.rows(), vertices, mesh.indices());
    rejects([&] { GlMaterialShadowRenderer invalid(context, bad_uv); });
    GlMaterialShadowRenderer renderer(context, mesh);
    check(c.draw(renderer, source.id, std::span(&p, 1), false).size() == 16384, "Positive shadow draw");
    rejects([&] { renderer.draw(0, p, 64, 64); });
    rejects([&] { renderer.draw(c.target, p, 64, 64); });
    for (auto n : {0, -1, std::numeric_limits<int>::min()}) {
        rejects([&] { renderer.draw(source.id, p, n, 64); });
        rejects([&] { renderer.draw(source.id, p, 64, n); });
    }
    rejects([&] { renderer.draw(source.id, p, 64, 64, {0, nan}); });
    auto invalid = p;
    invalid.sun_direction = {0, 0, 1};
    rejects([&] { renderer.draw(source.id, invalid, 64, 64); });
    GLuint wrong = 0;
    context->api().CreateTextures(GL_TEXTURE_3D, 1, &wrong);
    rejects([&] { renderer.draw(wrong, p, 64, 64); });
    context->api().DeleteTextures(1, &wrong);
    rejects([&] { renderer.draw(wrong, p, 64, 64); });
    context->api().CreateTextures(GL_TEXTURE_2D, 1, &wrong);
    rejects([&] { renderer.draw(wrong, p, 64, 64); });
    context->api().DeleteTextures(1, &wrong);
    const std::array<MaterialShadowParameters, 2> bad_samples{p, invalid};
    c.clear(.375f);
    c.gl.Enable(GL_BLEND);
    const auto before = c.readback();
    rejects([&] { renderer.draw_accumulated(source.id, bad_samples, 64, 64); });
    check(c.gl.IsEnabled(GL_BLEND) && before == c.readback(),
          "Invalid shadow list changed target or blending");
    c.gl.Disable(GL_BLEND);
    renderer.draw_accumulated(source.id, {}, 64, 64);
    check(!c.gl.IsEnabled(GL_BLEND) && before == c.readback(), "Empty shadow list drew pixels");
    c.gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    // Clear before setting all-channel accumulation; explicitly restore write mask
    // for the clear, since framebuffer clears also respect masks.
    c.gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    c.clear(0);
    c.gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    renderer.draw_accumulated(source.id, std::span(&p, 1), 64, 64);
    check(!c.gl.IsEnabled(GL_BLEND), "Shadow accumulation left blending enabled");
    const auto accumulated_pixels = c.readback();
    const auto single_pixels = c.draw(renderer, source.id, std::span(&p, 1), false);
    check(accumulated_pixels == single_pixels, "Shadow additive profile or all-channel mask differs");
    bool rejected = false;
    std::thread other([&] {
        try {
            renderer.draw(source.id, p, 64, 64);
        } catch (const std::exception &) {
            rejected = true;
        }
    });
    other.join();
    check(rejected, "Shadow renderer allowed another thread");
    auto owned = std::make_shared<GlContext>();
    std::weak_ptr<GlContext> retained = owned;
    auto independent = std::make_unique<GlMaterialShadowRenderer>(owned, mesh);
    owned.reset();
    check(!retained.expired(), "Shadow context not retained");
    independent.reset();
    check(retained.expired(), "Shadow context leak");
    context->api();
    context->check("Restore main shadow context");
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 4,
                "Usage: civ5_gl_material_shadows generation-input original-shader-directory fresh-output");
        const fs::path directory(argv[3]);
        require(!fs::exists(directory), "Fresh shadow output directory required");
        const auto input = read(argv[1]);
        require(is_generation_input(input), "Shadow pipeline requires source-generation inputs");
        const auto source = height_source(input);
        const auto computed = patches(input);
        const auto a8 = source.extract_a8_texture(0, 0);
        auto context = std::make_shared<GlContext>();
        Control c(context);
        NativeShadows native(argv[2]);
        guards(context, c, computed[2].mesh);
        fs::create_directories(directory);
        write(directory / "computed-patches.bin", computed_patches(computed));
        text(directory / "geometry-source.txt", "cpp-source-generation\n");
        write(directory / "computed-a8.bin", a8);
        text(directory / "shadow-vertex.glsl", material_shadow_vertex_shader());
        text(directory / "shadow-fragment.glsl", material_shadow_fragment_shader());
        unsigned cases = 0, unorm_cases = 0;
        auto execute = [&](const std::string &name, GLuint texture, IDirect3DTexture9 *native_texture,
                           bool generated, bool unorm) {
            for (unsigned patch = generated ? 0 : 2; patch < 3; ++patch) {
                const auto &mesh = computed[patch].mesh;
                GlMaterialShadowRenderer renderer(context, mesh);
                for (unsigned direction = 0; direction < 4; ++direction)
                    for (bool accumulated : {false, true}) {
                        const unsigned count =
                            accumulated ? (1u << direction) : (direction % 2 == 0 ? 1u : 4u);
                        std::vector<MaterialShadowParameters> samples;
                        for (unsigned i = 0; i < (accumulated ? count : 1); ++i)
                            samples.push_back(parameters(
                                generated, direction, count,
                                accumulated
                                    ? std::array<float, 2>{(int(i) - 5) * .03125f, (7 - int(i)) * .0625f}
                                    : std::array<float, 2>{.125f, -.25f}));
                        const auto id = name + "-p" + std::to_string(patch) + "-d" +
                                        std::to_string(direction) +
                                        (accumulated ? "-acc" + std::to_string(count) : "");
                        const auto gl = c.draw(renderer, texture, samples, accumulated);
                        const auto dx = native.draw(mesh, native_texture, samples, accumulated);
                        write(directory / (id + "-gl.bin"), gl);
                        write(directory / (id + "-d3d9.bin"), dx);
                        if (name == "mixed" && direction == 0 && !accumulated) {
                            context->api().TextureParameteri(texture, GL_TEXTURE_SWIZZLE_A, GL_RED);
                            write(directory / "alpha-negative.bin",
                                  c.draw(renderer, texture, samples, false));
                            context->api().TextureParameteri(texture, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
                            write(directory / "half-pixel-negative.bin",
                                  c.draw(renderer, texture, samples, false, {-1.f / 64, 1.f / 64}));
                            check(c.draw(renderer, texture, samples, false) == gl,
                                  "Shadow negative state not restored");
                        }
                        if (unorm)
                            ++unorm_cases;
                        else
                            ++cases;
                    }
            }
        };
        for (unsigned kind = 0; kind < 6; ++kind) {
            const auto values = field(kind);
            FloatSource gl(context, 64, values);
            auto dx = native.texture(64, bytes(values), false);
            write(directory / (std::string(names[kind]) + "-input.bin"), values);
            execute(names[kind], gl.id, dx.value, false, false);
        }
        std::vector<float> values(a8.size() * 4);
        for (std::size_t i = 0; i < a8.size(); ++i)
            values[i * 4 + 3] = a8[i] / 255.f;
        write(directory / "computed-a8-input.bin", values);
        {
            FloatSource gl(context, 1024, values);
            auto dx = native.texture(1024, bytes(values), false);
            execute("computed-a8", gl.id, dx.value, true, false);
        }
        {
            auto gl = GlTexture::alpha8(*context, 1024, 1024, a8);
            auto dx = native.texture(1024, a8, true);
            check(gl.read_level_raw(0, 0) == a8, "Production A8 upload differs");
            execute("computed-a8-unorm", gl.id(), dx.value, true, true);
        }
        std::ostringstream report;
        report << context->description() << "\nDONE floatCases=" << cases << "; unormA8Cases=" << unorm_cases
               << "; guards=" << checks << "; originalVSAndPS=true; fullScene=false\n";
        text(directory / "execution.txt", report.str());
        std::cout << report.str();
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
