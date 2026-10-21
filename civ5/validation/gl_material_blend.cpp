#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_material_textures.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "material_blend_native.hpp"
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>
using namespace blend_validation;
using namespace civ5::graphics;
namespace fs = std::filesystem;
namespace {
int controls = 0;
void check(bool value, const char *message) {
    ++controls;
    require(value, message);
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try {
        f();
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, "Missing material input rejection");
}
struct GlControl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint framebuffer = 0;
    std::array<GLuint, 2> targets{};
    std::array<std::array<GLuint, 16>, 2> textures{};
    explicit GlControl(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        try {
            const auto &api = context->api();
            gl.CreateFramebuffers(1, &framebuffer);
            api.CreateTextures(GL_TEXTURE_2D, 2, targets.data());
            for (unsigned i = 0; i < 2; ++i) {
                api.TextureStorage2D(targets[i], 1, GL_RGBA32F, 64, 64);
                gl.NamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0 + i, targets[i], 0);
            }
            const GLenum buffers[]{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
            gl.NamedFramebufferDrawBuffers(framebuffer, 2, buffers);
            require(gl.CheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                    "Incomplete material MRT");
            gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            gl.Viewport(0, 0, 64, 64);
            for (auto state : {GL_BLEND, GL_CULL_FACE, GL_DEPTH_TEST, GL_SCISSOR_TEST, GL_DITHER,
                               GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
                gl.Disable(state);
            for (unsigned pattern = 0; pattern < 2; ++pattern) {
                api.CreateTextures(GL_TEXTURE_2D, 16, textures[pattern].data());
                for (unsigned layer = 0; layer < 16; ++layer) {
                    const auto values = float_texture(layer, pattern != 0);
                    api.TextureStorage2D(textures[pattern][layer], 1, GL_RGBA32F, 4, 4);
                    api.TextureSubImage2D(textures[pattern][layer], 0, 0, 0, 4, 4, GL_RGBA, GL_FLOAT,
                                          values.data());
                }
            }
            context->check("Own material validation FBO/textures");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        const auto &api = context->api();
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &framebuffer);
        framebuffer = 0;
        api.DeleteTextures(2, targets.data());
        targets = {};
        for (auto &set : textures) {
            api.DeleteTextures(16, set.data());
            set = {};
        }
    }
    ~GlControl() { release(); }
    std::vector<float> draw(GlMaterialBlendRenderer &renderer, std::span<const float, 384> coefficients,
                            const std::array<GLuint, 16> &names, bool linear) {
        const float clear[]{-7, -7, -7, -7};
        for (unsigned i = 0; i < 2; ++i)
            gl.ClearNamedFramebufferfv(framebuffer, GL_COLOR, i, clear);
        renderer.draw(coefficients, {0, 0}, names,
                      linear ? MaterialSampling::ordinary0 : MaterialSampling::point_control);
        std::vector<float> output(64 * 64 * 8);
        for (unsigned i = 0; i < 2; ++i)
            context->api().GetTextureImage(targets[i], 0, GL_RGBA, GL_FLOAT, 64 * 64 * 16,
                                           output.data() + i * 64 * 64 * 4);
        context->check("Read both material shader targets");
        return output;
    }
};
void gpu_guards(std::shared_ptr<GlContext> context, const Patch &patch, GlControl &control) {
    const auto &api = context->api();
    const auto &gl = control.gl;
    const auto coefficients = float_coefficients();
    rejects([&] { GlMaterialBlendRenderer invalid(nullptr, patch.mesh); });
    auto moved_source = patch.mesh;
    const auto moved_destination = std::move(moved_source);
    rejects([&] { GlMaterialBlendRenderer invalid(context, moved_source); });
    GlMaterialBlendRenderer renderer(context, patch.mesh);
    auto bad = coefficients;
    bad[383] = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { renderer.capture(bad, {0, 0}); });
    rejects([&] { renderer.capture(coefficients, {0, std::numeric_limits<float>::infinity()}); });
    auto names = control.textures[0];
    names[15] = 0;
    rejects([&] { renderer.draw(coefficients, {0, 0}, names, MaterialSampling::point_control); });
    rejects(
        [&] { renderer.draw(coefficients, {0, 0}, control.textures[0], static_cast<MaterialSampling>(99)); });
    GLuint buffer = 0, feedback = 0;
    gl.CreateBuffers(1, &buffer);
    gl.CreateTransformFeedbacks(1, &feedback);
    gl.NamedBufferData(buffer, patch.mesh.columns() * patch.mesh.rows() * 28 * 4, nullptr, GL_STREAM_READ);
    gl.TransformFeedbackBufferBase(feedback, 0, buffer);
    gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback);
    for (bool discard : {false, true}) {
        if (discard)
            gl.Enable(GL_RASTERIZER_DISCARD);
        else
            gl.Disable(GL_RASTERIZER_DISCARD);
        const auto captured = renderer.capture(coefficients, {0, 0});
        check(captured.size() == std::size_t(patch.mesh.columns() * patch.mesh.rows() * 28),
              "Wrong material TF size");
        GLint binding = 0;
        api.GetIntegerv(GL_TRANSFORM_FEEDBACK_BINDING, &binding);
        check(static_cast<GLuint>(binding) == feedback &&
                  bool(gl.IsEnabled(GL_RASTERIZER_DISCARD)) == discard,
              "Material feedback state not restored");
    }
    gl.Enable(GL_RASTERIZER_DISCARD);
    gl.BeginTransformFeedback(GL_POINTS);
    context->check("Own active feedback control");
    rejects([&] { renderer.capture(coefficients, {0, 0}); });
    gl.EndTransformFeedback();
    gl.Disable(GL_RASTERIZER_DISCARD);
    gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
    gl.DeleteTransformFeedbacks(1, &feedback);
    gl.DeleteBuffers(1, &buffer);
    bool cross_thread_rejected = false;
    std::thread other([&] {
        try {
            renderer.capture(coefficients, {0, 0});
        } catch (const std::exception &) {
            cross_thread_rejected = true;
        }
    });
    other.join();
    check(cross_thread_rejected, "Cross-thread material capture not rejected");
    // Separate context retained solely by renderer; mesh/source lifetime not needed after upload.
    auto owned = std::make_shared<GlContext>();
    std::weak_ptr<GlContext> retained = owned;
    auto independent = std::make_unique<GlMaterialBlendRenderer>(owned, patch.mesh);
    owned.reset();
    check(!retained.expired(), "Material renderer did not retain context");
    check(independent->capture(coefficients, {0, 0}).size() ==
              std::size_t(patch.mesh.columns() * patch.mesh.rows() * 28),
          "Retained renderer failed");
    independent.reset();
    check(retained.expired(), "Material context leaked");
    context->api();
    context->check("Restore main material test context");
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(
            argc == 5,
            "Usage: civ5_gl_material_blend computed-patches.bin asset-root shader-directory fresh-output");
        const fs::path directory(argv[4]);
        require(!fs::exists(directory), "Fresh output directory required");
        const auto input_bytes = read(argv[1]);
        const auto sources = patches(input_bytes);
        auto assets = std::make_shared<const MaterialAssets>(authored(argv[2]));
        auto context = std::make_shared<GlContext>();
        std::cout << context->description() << '\n';
        GlControl control(context);
        GlMaterialTextures textures(context, assets);
        NativeBlend native(argv[3], *assets);
        gpu_guards(context, sources[0], control);
        fs::create_directories(directory);
        text(directory / "geometry-source.txt",
             is_generation_input(input_bytes) ? "cpp-source-generation\n" : "computed-mesh-replay\n");
        write(directory / "computed-patches.bin", computed_patches(sources));
        const auto coefficients = float_coefficients();
        write(directory / "coefficients.bin", coefficients);
        text(directory / "vertex.glsl", material_blend_vertex_shader());
        text(directory / "fragment.glsl", material_blend_fragment_shader());
        std::array<float, 384> authored_coefficients{};
        assets->write_constants(2070.375f, authored_coefficients);
        write(directory / "authored-coefficients.bin", authored_coefficients);
        std::ostringstream report;
        report << "Own original VS/PS versus C++ material blend\n" << context->description() << "\n";
        unsigned cases = 0;
        for (unsigned i = 0; i < sources.size(); ++i) {
            const auto &patch = sources[i];
            GlMaterialBlendRenderer renderer(context, patch.mesh);
            for (unsigned offset = 0; offset < 2; ++offset) {
                const std::array<float, 2> texel_offset =
                    offset ? std::array{-1.f / 64, 1.f / 64} : std::array{0.f, 0.f};
                write(directory / ("case-" + std::to_string(i) + "-offset-" + std::to_string(offset) +
                                   "-feedback.bin"),
                      renderer.capture(coefficients, texel_offset));
            }
            write(directory / ("case-" + std::to_string(i) + "-indexed-feedback.bin"),
                  renderer.capture(coefficients, {0, 0}, true));
            auto draw = [&](const std::string &name, std::span<const float, 384> constants,
                            const std::array<GLuint, 16> &names, const std::array<std::uint16_t, 16> &ids,
                            int texture_set, bool linear) {
                const auto gl = control.draw(renderer, constants, names, linear),
                           dx = native.draw(patch.mesh, constants, ids, texture_set, linear);
                write(directory / (name + "-rgba.bin"), gl);
                write(directory / (name + "-d3d9.bin"), dx);
                report << name << "; channels=" << gl.size() << "; source=" << texture_set
                       << "; linear=" << linear << '\n';
                ++cases;
            };
            for (unsigned pattern = 0; pattern < 2; ++pattern)
                draw("case-" + std::to_string(i) + "-pattern-" + (pattern ? "true" : "false"), coefficients,
                     control.textures[pattern], {}, pattern, false);
            const auto ids = assets->selected_resources(patch.selected);
            const auto names = textures.bind(patch.selected);
            const auto zero_names = textures.bind({});
            for (bool linear : {false, true}) {
                const auto suffix = linear ? "-linear" : "-point";
                draw("case-" + std::to_string(i) + "-authored" + suffix, authored_coefficients, names, ids,
                     -1, linear);
                draw("case-" + std::to_string(i) + "-zero" + suffix, authored_coefficients, zero_names, {},
                     -1, linear);
            }
            if (i == 1) {
                const auto baseline = control.draw(renderer, authored_coefficients, names, false);
                for (unsigned lane = 0; lane < 8; ++lane)
                    context->api().TextureParameteri(names[lane], GL_TEXTURE_SWIZZLE_A, GL_ONE);
                const auto opaque = control.draw(renderer, authored_coefficients, names, false);
                for (unsigned lane = 0; lane < 8; ++lane)
                    context->api().TextureParameteri(names[lane], GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
                write(directory / "force-opaque-negative.bin", opaque);
                float error = 0;
                for (std::size_t at = 0; at < baseline.size(); ++at)
                    error = std::max(error, std::abs(baseline[at] - opaque[at]));
                check(error > .01f, "Missing alpha negative control");
                report << "forceOpaqueMaxError=" << error << '\n';
            }
        }
        report << "DONE cases=" << cases << "; guards=" << controls
               << "; compare outputs separately; not a scene parity claim\n";
        text(directory / "execution.txt", report.str());
        std::cout << report.str();
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
