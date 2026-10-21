#include "civ5/graphics/gl_material_normals.hpp"
#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_material_textures.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "material_normal_native.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>
using namespace normal_validation;
using namespace civ5::graphics;
namespace fs = std::filesystem;
namespace {
int guards = 0;
void check(bool ok, const char *why) {
    ++guards;
    require(ok, why);
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "Missing normal rejection");
}
const std::array<std::array<float, 4>, 2> views{{{0, 0, 1, 1}, {-.125f, .125f, 1.125f, .875f}}};
const std::array<const char *, 6> names{"flat", "ramp-x", "ramp-y", "blocks", "impulse", "mixed"};
std::vector<float> field(unsigned kind) {
    std::vector<float> out(64 * 64 * 4);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            float red = 0;
            switch (kind) {
            case 0:
                red = .375f;
                break;
            case 1:
                red = x / 64.f;
                break;
            case 2:
                red = y / 64.f;
                break;
            case 3:
                red = (x / 8 + y / 8) % 2 == 0 ? 0.f : 1.f;
                break;
            case 4:
                red = x == 31 && y == 32 ? 1.f : 0.f;
                break;
            case 5:
                red = ((x * 13 + y * 7 + x * y) % 64) / 64.f;
                break;
            default:
                throw std::runtime_error("Invalid normal control field");
            }
            out[(y * 64 + x) * 4] = red;
            for (int c = 1; c < 4; ++c)
                out[(y * 64 + x) * 4 + c] = ((x * 3 + y * 5 + c * 7) % 64) / 64.f;
        }
    return out;
}
std::vector<float> scalar(std::span<const float> source, const std::array<float, 4> &uv) {
    const auto sample = [&](float u, float v) {
        const float x = u * 64 - .5f, y = v * 64 - .5f;
        const auto ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
        const float fx = x - ix, fy = y - iy;
        const auto at = [&](int x0, int y0) {
            return source[(std::clamp(y0, 0, 63) * 64 + std::clamp(x0, 0, 63)) * 4];
        };
        const float a = at(ix, iy) * (1 - fx) + at(ix + 1, iy) * fx;
        const float b = at(ix, iy + 1) * (1 - fx) + at(ix + 1, iy + 1) * fx;
        return a * (1 - fy) + b * fy;
    };
    std::vector<float> result;
    result.reserve(64 * 64 * 4);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            const float u = uv[0] + ((x + .5f) / 64) * (uv[2] - uv[0]);
            const float v = uv[1] + ((y + .5f) / 64) * (uv[3] - uv[1]);
            const float d = 1.f / 64;
            const std::array<float, 8> taps{sample(u - d, v - d), sample(u, v - d),    sample(u + d, v - d),
                                            sample(u - d, v),     sample(u + d, v),    sample(u - d, v + d),
                                            sample(u, v + d),     sample(u + d, v + d)};
            const auto encoded = encode_material_normal(taps);
            result.insert(result.end(), encoded.begin(), encoded.end());
        }
    return result;
}
struct Control {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    std::array<GLuint, 2> fbos{};
    std::array<GLuint, 4> textures{}; // material MRT0/1, normal target, replay input
    explicit Control(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        try {
            const auto &api = context->api();
            gl.CreateFramebuffers(2, fbos.data());
            api.CreateTextures(GL_TEXTURE_2D, 4, textures.data());
            for (auto t : textures)
                api.TextureStorage2D(t, 1, GL_RGBA32F, 64, 64);
            for (unsigned i = 0; i < 2; ++i)
                gl.NamedFramebufferTexture(fbos[0], GL_COLOR_ATTACHMENT0 + i, textures[i], 0);
            const GLenum buffers[]{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
            gl.NamedFramebufferDrawBuffers(fbos[0], 2, buffers);
            gl.NamedFramebufferTexture(fbos[1], GL_COLOR_ATTACHMENT0, textures[2], 0);
            gl.NamedFramebufferDrawBuffers(fbos[1], 1, buffers);
            for (auto f : fbos)
                require(gl.CheckNamedFramebufferStatus(f, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                        "Normal control FBO");
            gl.Viewport(0, 0, 64, 64);
            for (auto state : {GL_BLEND, GL_CULL_FACE, GL_DEPTH_TEST, GL_SCISSOR_TEST, GL_DITHER,
                               GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
                gl.Disable(state);
            context->check("Normal validation objects");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(2, fbos.data());
        fbos = {};
        context->api().DeleteTextures(4, textures.data());
        textures = {};
    }
    ~Control() { release(); }
    std::vector<float> readback(GLuint name) {
        std::vector<float> out(64 * 64 * 4);
        context->api().GetTextureImage(name, 0, GL_RGBA, GL_FLOAT, 64 * 64 * 16, out.data());
        context->check("Normal control readback");
        require(std::all_of(out.begin(), out.end(), [](float v) { return std::isfinite(v); }),
                "Nonfinite normal image or unwritten poison");
        return out;
    }
    void upload(std::span<const float> values) {
        require(values.size() == 64 * 64 * 4, "Normal upload extent");
        context->api().TextureSubImage2D(textures[3], 0, 0, 0, 64, 64, GL_RGBA, GL_FLOAT, values.data());
        const auto actual = readback(textures[3]);
        require(std::memcmp(actual.data(), values.data(), values.size_bytes()) == 0,
                "GL normal input upload differs");
    }
    std::vector<float> normals(GlMaterialNormalRenderer &renderer, GLuint source,
                               const std::array<float, 4> &uv) {
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbos[1]);
        const auto nan = std::numeric_limits<float>::quiet_NaN();
        const float poison[]{nan, nan, nan, nan};
        gl.ClearNamedFramebufferfv(fbos[1], GL_COLOR, 0, poison);
        renderer.draw(source, 64, uv);
        return readback(textures[2]);
    }
    void material(GlMaterialBlendRenderer &renderer, std::span<const float, 384> coefficients,
                  const std::array<GLuint, 16> &layers) {
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbos[0]);
        const float poison[]{-7, -7, -7, -7};
        for (int i = 0; i < 2; ++i)
            gl.ClearNamedFramebufferfv(fbos[0], GL_COLOR, i, poison);
        renderer.draw(coefficients, {0, 0}, layers, MaterialSampling::ordinary0);
    }
};
void controls(std::shared_ptr<GlContext> context, Control &c, GlMaterialNormalRenderer &renderer) {
    rejects([] { GlMaterialNormalRenderer invalid(nullptr); });
    c.upload(field(5));
    check(c.normals(renderer, c.textures[3], views[0]).size() == 64 * 64 * 4,
          "Positive normal draw control failed");
    c.gl.BindFramebuffer(GL_FRAMEBUFFER, c.fbos[1]);
    rejects([&] { renderer.draw(0, 64); });
    for (auto resolution : {0, -1, std::numeric_limits<int>::min()})
        rejects([&] { renderer.draw(c.textures[3], resolution); });
    for (unsigned at = 0; at < 4; ++at) {
        auto uv = views[0];
        uv[at] = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { renderer.draw(c.textures[3], 64, uv); });
    }
    GLuint wrong = 0;
    context->api().CreateTextures(GL_TEXTURE_3D, 1, &wrong);
    rejects([&] { renderer.draw(wrong, 64); });
    context->check("Rejected non-2D source left GL state clean");
    context->api().DeleteTextures(1, &wrong);
    rejects([&] { renderer.draw(wrong, 64); });
    context->api().CreateTextures(GL_TEXTURE_2D, 1, &wrong);
    rejects([&] { renderer.draw(wrong, 64); });
    context->api().DeleteTextures(1, &wrong);
    rejects([&] { renderer.draw(c.textures[2], 64); });
    c.gl.BindFramebuffer(GL_FRAMEBUFFER, c.fbos[0]);
    rejects([&] { renderer.draw(c.textures[1], 64); });
    bool rejected = false;
    std::thread other([&] {
        try {
            renderer.draw(c.textures[3], 64);
        } catch (const std::exception &) {
            rejected = true;
        }
    });
    other.join();
    check(rejected, "Normal renderer allowed foreign thread");
    auto owned = std::make_shared<GlContext>();
    std::weak_ptr<GlContext> retained = owned;
    auto independent = std::make_unique<GlMaterialNormalRenderer>(owned);
    owned.reset();
    check(!retained.expired(), "Normal context not retained");
    independent.reset();
    check(retained.expired(), "Normal context leak");
    context->api();
    context->check("Restore main normal context");
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 6, "Usage: civ5_gl_material_normals generation-input asset-root blend-shaders "
                           "normal-shaders fresh-output");
        const fs::path directory(argv[5]);
        require(!fs::exists(directory), "Fresh output directory required");
        const auto input = read(argv[1]);
        require(is_generation_input(input), "Normal pipeline requires source-generation inputs");
        const auto patches_cpp = patches(input);
        auto assets = std::make_shared<const MaterialAssets>(authored(argv[2]));
        auto context = std::make_shared<GlContext>();
        Control c(context);
        GlMaterialTextures textures(context, assets);
        GlMaterialNormalRenderer renderer(context);
        NativeBlend native_blend(argv[3], *assets);
        NativeNormals native(argv[4]);
        controls(context, c, renderer);
        fs::create_directories(directory);
        write(directory / "computed-patches.bin", computed_patches(patches_cpp));
        text(directory / "geometry-source.txt", "cpp-source-generation\n");
        text(directory / "normal-vertex.glsl", material_normal_vertex_shader());
        text(directory / "normal-fragment.glsl", material_normal_fragment_shader());
        std::ostringstream report;
        report << context->description() << '\n';
        unsigned cases = 0;
        auto compare = [&](const std::string &name, std::span<const float> source, GLuint direct) {
            write(directory / (name + "-input.bin"), source);
            c.upload(source);
            for (unsigned view = 0; view < 2; ++view) {
                const auto id = name + "-" + std::to_string(view);
                write(directory / (id + "-gl.bin"), c.normals(renderer, c.textures[3], views[view]));
                write(directory / (id + "-d3d9.bin"), native.draw(source, views[view]));
                write(directory / (id + "-scalar.bin"), scalar(source, views[view]));
                if (direct)
                    write(directory / (id + "-direct.bin"), c.normals(renderer, direct, views[view]));
                ++cases;
            }
        };
        for (unsigned kind = 0; kind < 6; ++kind)
            compare(names[kind], field(kind), 0);
        std::array<float, 384> coefficients{};
        assets->write_constants(2070.375f, coefficients);
        write(directory / "authored-coefficients.bin", coefficients);
        for (unsigned i = 0; i < patches_cpp.size(); ++i) {
            const auto &patch = patches_cpp[i];
            GlMaterialBlendRenderer blend(context, patch.mesh);
            c.material(blend, coefficients, textures.bind(patch.selected));
            const auto height = c.readback(c.textures[1]);
            const auto native_pixels = native_blend.draw(
                patch.mesh, coefficients, assets->selected_resources(patch.selected), -1, true);
            const std::span<const float> native_height(native_pixels.data() + 64 * 64 * 4, 64 * 64 * 4);
            const auto name = "material-" + std::to_string(i);
            write(directory / (name + "-diffuse.bin"), c.readback(c.textures[0]));
            write(directory / (name + "-native-blend.bin"), native_pixels);
            compare(name, height, c.textures[1]);
            for (unsigned v = 0; v < 2; ++v)
                write(directory / (name + "-" + std::to_string(v) + "-native-chain.bin"),
                      native.draw(native_height, views[v]));
        }
        c.upload(field(5));
        const auto correct = c.normals(renderer, c.textures[3], views[1]);
        context->api().TextureParameteri(c.textures[3], GL_TEXTURE_SWIZZLE_R, GL_ALPHA);
        write(directory / "alpha-negative.bin", c.normals(renderer, c.textures[3], views[1]));
        context->api().TextureParameteri(c.textures[3], GL_TEXTURE_SWIZZLE_R, GL_RED);
        // Diagnostic mutation of our OWN bound sampler, immediately restored.
        const auto get_integer =
            reinterpret_cast<PFNGLGETINTEGERI_VPROC>(context->proc_address("glGetIntegeri_v"));
        GLint sampler = 0;
        get_integer(GL_SAMPLER_BINDING, 0, &sampler);
        for (auto wrap : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
            c.gl.SamplerParameteri(sampler, wrap, GL_REPEAT);
        write(directory / "repeat-negative.bin", c.normals(renderer, c.textures[3], views[1]));
        for (auto wrap : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
            c.gl.SamplerParameteri(sampler, wrap, GL_CLAMP_TO_EDGE);
        const auto restored = c.normals(renderer, c.textures[3], views[1]);
        check(std::memcmp(correct.data(), restored.data(), correct.size() * 4) == 0,
              "Normal sampler not restored");
        report << "DONE cases=" << cases << "; guards=" << guards
               << "; originalVSAndPS=true; fullScene=false\n";
        text(directory / "execution.txt", report.str());
        std::cout << report.str();
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
