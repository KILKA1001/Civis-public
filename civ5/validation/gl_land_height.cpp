// Own hidden GPU only. Explicit synthetic/computed-reference source fields,
// never expected geometry as renderer input or a claim about native game state.
#include "civ5/graphics/gl_land_height.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::graphics;
using namespace civ5::terrain;
namespace fs = std::filesystem;
namespace {
void require(bool ok, const char *reason) {
    if (!ok)
        throw std::runtime_error(reason);
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try {
        f();
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    require(rejected, "Missing LAND input guard");
}
void word(std::ostream &out, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        out.put(static_cast<char>(value >> (i * 8)));
}
template <class T> void raw(std::ostream &out, std::span<const T> bytes) {
    static_assert(std::endian::native == std::endian::little);
    out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size_bytes());
    require(bool(out), "GPU output write failed");
}
template <class T> void save(const fs::path &file, std::span<const T> bytes) {
    std::ofstream out(file, std::ios::binary);
    raw(out, bytes);
    out.close();
    require(bool(out), "GPU output close failed");
}
std::array<float, 16> projection(float x, float y, float w, float h) {
    return {2.f / w, 0, 0, 0, 0, 2.f / h, 0, 0, 0, 0, 0, 0, -1.f - 2.f * x / w, -1.f - 2.f * y / h, 0, 1};
}
struct Target {
    GlContext &context;
    GlRenderApi &gl;
    GLuint texture = 0, frame = 0;
    Target(GlContext &c, GlRenderApi &g, int width, int height, GLenum format) : context(c), gl(g) {
        const auto &api = context.api();
        try {
            api.CreateTextures(GL_TEXTURE_2D, 1, &texture);
            api.TextureStorage2D(texture, 1, format, width, height);
            gl.CreateFramebuffers(1, &frame);
            gl.NamedFramebufferTexture(frame, GL_COLOR_ATTACHMENT0, texture, 0);
            require(gl.CheckNamedFramebufferStatus(frame, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                    "Incomplete LAND target");
            gl.BindFramebuffer(GL_FRAMEBUFFER, frame);
            gl.Viewport(0, 0, width, height);
            for (auto state : {GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND, GL_SCISSOR_TEST, GL_FRAMEBUFFER_SRGB})
                gl.Disable(state);
            gl.ClearColor(0, 0, 0, 0);
            gl.Clear(GL_COLOR_BUFFER_BIT);
            context.check("Create own LAND target");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &frame);
        context.api().DeleteTextures(1, &texture);
    }
    ~Target() { release(); }
};
void guards(const std::shared_ptr<GlContext> &context) {
    const auto &api = context->api();
    GlRenderApi gl(*context);
    const auto field =
        std::make_shared<HeightField>(HeightDomain(16, 16), std::vector<std::uint16_t>(256, 0xff00));
    rejects([&] { GlLandHeightRenderer r(context, field, 0); });
    rejects([&] { GlLandHeightRenderer r(context, field, 4); });
    rejects([&] { GlLandHeightRenderer r(context, nullptr, 1); });
    rejects([&] { GlLandHeightRenderer r(nullptr, field, 1); });
    rejects([&] { GlTexture::alpha8(*context, 0, 2, {}); });
    const std::array<std::uint8_t, 2> short_data{0, 255};
    rejects([&] { GlTexture::alpha8(*context, 3, 1, short_data); });
    GLuint pbo = 0;
    gl.CreateBuffers(1, &pbo);
    gl.NamedBufferData(pbo, 16, nullptr, GL_STATIC_DRAW);
    const std::array<GLenum, 12> fields{GL_UNPACK_ALIGNMENT,
                                        GL_UNPACK_ROW_LENGTH,
                                        GL_UNPACK_IMAGE_HEIGHT,
                                        GL_UNPACK_SKIP_PIXELS,
                                        GL_UNPACK_SKIP_ROWS,
                                        GL_UNPACK_SKIP_IMAGES,
                                        GL_UNPACK_SWAP_BYTES,
                                        GL_UNPACK_LSB_FIRST,
                                        GL_UNPACK_COMPRESSED_BLOCK_WIDTH,
                                        GL_UNPACK_COMPRESSED_BLOCK_HEIGHT,
                                        GL_UNPACK_COMPRESSED_BLOCK_DEPTH,
                                        GL_UNPACK_COMPRESSED_BLOCK_SIZE};
    const std::array<GLint, 12> hostile{8, 2048, 2048, 7, 3, 2, 1, 1, 8, 4, 1, 32};
    std::array<GLint, 12> previous{};
    GLint old_pbo = 0;
    api.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &old_pbo);
    for (std::size_t i = 0; i < fields.size(); ++i) {
        api.GetIntegerv(fields[i], &previous[i]);
        api.PixelStorei(fields[i], hostile[i]);
    }
    api.BindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    {
        GlLandHeightRenderer renderer(context, field, 1);
        const auto result = renderer.capture(0, 0, false);
        require(result.size() == 256 * 6 && result[2] == 51.f, "A8 producer/alpha height guard");
        rejects([&] { renderer.capture(1, 0, false); });
        auto invalid = projection(0, 0, 1, 1);
        invalid[2] = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { renderer.draw(0, 0, invalid); });
        for (std::size_t i = 0; i < fields.size(); ++i) {
            GLint actual = 0;
            api.GetIntegerv(fields[i], &actual);
            require(actual == hostile[i], "A8 unpack state not restored");
        }
        GLint actual = 0;
        api.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &actual);
        require(actual == GLint(pbo), "A8 PBO not restored");
        GLuint feedback = 0, buffer = 0;
        gl.CreateTransformFeedbacks(1, &feedback);
        gl.CreateBuffers(1, &buffer);
        gl.NamedBufferData(buffer, 24, nullptr, GL_STREAM_READ);
        gl.TransformFeedbackBufferBase(feedback, 0, buffer);
        gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback);
        gl.Enable(GL_RASTERIZER_DISCARD);
        renderer.capture(0, 0, false);
        api.GetIntegerv(GL_TRANSFORM_FEEDBACK_BINDING, &actual);
        require(actual == GLint(feedback) && gl.IsEnabled(GL_RASTERIZER_DISCARD),
                "Feedback state not restored");
        gl.BeginTransformFeedback(GL_POINTS);
        context->check("Active feedback guard setup");
        rejects([&] { renderer.capture(0, 0, false); });
        gl.EndTransformFeedback();
        gl.Disable(GL_RASTERIZER_DISCARD);
        gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        gl.DeleteTransformFeedbacks(1, &feedback);
        gl.DeleteBuffers(1, &buffer);
    }
    for (std::size_t i = 0; i < fields.size(); ++i)
        api.PixelStorei(fields[i], previous[i]);
    api.BindBuffer(GL_PIXEL_UNPACK_BUFFER, old_pbo);
    gl.DeleteBuffers(1, &pbo);
    {
        auto retained_context = std::make_shared<GlContext>();
        auto retained_field = std::make_shared<HeightField>(*field);
        GlLandHeightRenderer renderer(retained_context, retained_field, 1);
        retained_context.reset();
        retained_field.reset();
        require(renderer.capture(0, 0, false)[2] == 51.f, "LAND resource lifetime not retained");
    }
    context->check("LAND alpha/lifetime/input/feedback guards");
    std::cout << "PASS LAND A8 upload state, field/context lifetime and input/feedback guards\n";
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 2 || argc == 5,
                "Usage: civ5_gl_land_height fresh-directory [computed-field.u16 width height]");
        const fs::path output(argv[1]);
        require(!fs::exists(output), "Fresh LAND output directory required");
        const int width = argc == 5 ? std::stoi(argv[3]) : 2560,
                  height = argc == 5 ? std::stoi(argv[4]) : 1792;
        const HeightDomain domain(width, height);
        const LandLayout layout(domain);
        require(width >= 960 && height >= 960 && std::uint64_t(width) * height <= 16 * 1024 * 1024,
                "Validation field bound/coverage");
        std::vector<std::uint16_t> values(static_cast<std::size_t>(width) * height);
        if (argc == 5) {
            require(fs::file_size(argv[2]) == values.size() * 2, "Computed field length mismatch");
            std::ifstream in(argv[2], std::ios::binary);
            in.read(reinterpret_cast<char *>(values.data()), values.size() * 2);
            require(bool(in), "Computed field read failed");
        } else {
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                    values[y * width + x] = static_cast<std::uint16_t>(
                        (((x * 17 + y * 37 + (x / 64) * 13 + (y / 64) * 7) & 255) << 8) |
                        ((x + y * 3) & 255));
        }
        auto field = std::make_shared<HeightField>(domain, values);
        auto context = std::make_shared<GlContext>();
        GlRenderApi gl(*context);
        std::cout << context->description() << std::endl;
        guards(context);
        fs::create_directories(output);
        save(output / "input.u16", std::span<const std::uint16_t>(values));
        save(output / "vertex.glsl", std::span<const char>(land_height_vertex_shader()));
        std::ofstream out(output / "feedback.bin", std::ios::binary);
        out.write("C5LAND01", 8);
        word(out, width);
        word(out, height);
        word(out, layout.columns() * layout.rows() + 12);
        std::size_t samples = 0;
        const auto capture = [&](GlLandHeightRenderer &renderer, int step, int x, int y, bool indexed) {
            const auto data = renderer.capture(x, y, indexed);
            for (auto v : {step, x, y, int(indexed), static_cast<int>(data.size() / 6)})
                word(out, v);
            raw(out, std::span<const float>(data));
            samples += data.size() / 6;
        };
        {
            GlLandHeightRenderer renderer(context, field, 1);
            for (int x = 0; x < layout.columns(); ++x) {
                for (int y = 0; y < layout.rows(); ++y)
                    capture(renderer, 1, x, y, false);
                if (x % 8 == 0)
                    std::cout << "CAPTURED LAND columns " << x + 1 << '/' << layout.columns() << std::endl;
            }
            Target target(*context, gl, 1024, 768, GL_RGBA8);
            const auto vp = projection(-layout.center_x(), -layout.center_y(), float(width - 1),
                                       float(height - 1) * .866f);
            for (int x = 0; x < layout.columns(); ++x)
                for (int y = 0; y < layout.rows(); ++y)
                    renderer.draw(x, y, vp);
            std::vector<std::uint8_t> pixels(1024 * 768 * 4);
            context->api().GetTextureImage(target.texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.size(),
                                           pixels.data());
            context->check("Full native LAND overview readback");
            save(output / "overview.rgba", std::span<const std::uint8_t>(pixels));
        }
        for (int step = 1; step <= 3; ++step) {
            GlLandHeightRenderer renderer(context, field, step);
            for (auto [x, y] :
                 {std::pair{0, 0}, {13, 13}, {14, 14}, {layout.columns() - 1, layout.rows() - 1}})
                capture(renderer, step, x, y, true);
        }
        out.close();
        require(bool(out), "Full LAND feedback output close");
        std::ofstream raster(output / "constant-raster.bin", std::ios::binary);
        auto flat =
            std::make_shared<HeightField>(HeightDomain(80, 80), std::vector<std::uint16_t>(6400, 0x8000));
        for (int step = 1; step <= 3; ++step) {
            GlLandHeightRenderer renderer(context, flat, step);
            Target target(*context, gl, 64, 64, GL_RGBA32F);
            renderer.draw(0, 0, projection(-40, -40 * .866f, 64, 64 * .866f));
            std::vector<float> pixels(64 * 64 * 4);
            context->api().GetTextureImage(target.texture, 0, GL_RGBA, GL_FLOAT,
                                           pixels.size() * sizeof(float), pixels.data());
            context->check("Native LAND constant raster readback");
            raw(raster, std::span<const float>(pixels));
        }
        raster.close();
        require(bool(raster), "Constant raster output close");
        context->check("Final native LAND GPU execution");
        std::cout << "DONE " << samples
                  << " GPU vertices; compare full outputs separately; own hidden context only\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
