#include "gl_minimap_viewport_ring.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
std::vector<std::uint8_t> read(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("Kind-8 GL probe input is missing");
    const auto size = stream.tellg();
    if (size < 28 || size > 65536) throw std::runtime_error("Kind-8 GL probe extent changed");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(reinterpret_cast<char *>(bytes.data()), size);
    if (!stream) throw std::runtime_error("Kind-8 GL probe input is truncated");
    return bytes;
}
std::vector<std::uint8_t> background_rgba(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    std::string magic, extent, range;
    if (!stream || !std::getline(stream, magic) || !std::getline(stream, extent) ||
        !std::getline(stream, range) || magic != "P6" || extent != "320 190" || range != "255")
        throw std::runtime_error("Kind-8 GL background PPM header changed");
    std::vector<std::uint8_t> rgb(320 * 190 * 3);
    stream.read(reinterpret_cast<char *>(rgb.data()), rgb.size());
    char extra = 0;
    if (!stream || stream.get(extra))
        throw std::runtime_error("Kind-8 GL background PPM extent changed");
    std::vector<std::uint8_t> rgba(320 * 190 * 4);
    for (int y = 0; y < 190; ++y)
        for (int x = 0; x < 320; ++x) {
            const auto src = (y * 320 + x) * 3;
            const auto dst = ((189 - y) * 320 + x) * 4;
            rgba[dst] = rgb[src];
            rgba[dst + 1] = rgb[src + 1];
            rgba[dst + 2] = rgb[src + 2];
            rgba[dst + 3] = 255;
        }
    return rgba;
}
template <class T> T at(const std::vector<std::uint8_t> &data, std::size_t offset) {
    if (offset + sizeof(T) > data.size()) throw std::runtime_error("Kind-8 GL field is truncated");
    T result{};
    std::memcpy(&result, data.data() + offset, sizeof(T));
    return result;
}
void save_ppm(const civ5::graphics::GlTexture &texture,
              const std::filesystem::path &path) {
    const auto raw = texture.read_mip_raw(0);
    if (raw.size() != 320 * 190 * 4)
        throw std::runtime_error("Kind-8 GL readback extent changed");
    std::ofstream image(path, std::ios::binary);
    image << "P6\n320 190\n255\n";
    for (int y = 189; y >= 0; --y)
        for (int x = 0; x < 320; ++x) {
            const auto base = (static_cast<std::size_t>(y) * 320 + x) * 4;
            const char rgb[3]{static_cast<char>(raw[base + 2]),
                              static_cast<char>(raw[base + 1]),
                              static_cast<char>(raw[base])};
            image.write(rgb, 3);
        }
    if (!image) throw std::runtime_error("Kind-8 GL PPM write failed");
}
} // namespace

int main(int argc, char **argv) {
    try {
        const bool rebuild_ring = argc >= 5 && std::string_view(argv[argc - 1]) == "--rebuild-ring";
        const bool has_background = argc == 6 || (argc == 5 && !rebuild_ring);
        if (argc < 4 || argc > 6 || (argc == 6 && !rebuild_ring))
            throw std::invalid_argument("Usage: civ5_gl_minimap_viewport viewport.bin output.ppm pixel-shift [background.ppm] [--rebuild-ring]");
        const auto payload = read(argv[1]);
        if (std::memcmp(payload.data(), "MMV1", 4))
            throw std::runtime_error("Kind-8 GL probe magic changed");
        const civ5::ui::MinimapWorldBounds bounds{at<float>(payload, 4), at<float>(payload, 8),
                                                  at<float>(payload, 12), at<float>(payload, 16)};
        const auto vertex_count = at<std::uint32_t>(payload, 20);
        const auto index_count = at<std::uint32_t>(payload, 24);
        if (vertex_count == 0 || vertex_count > 36 || vertex_count % 12 ||
            index_count != vertex_count * 4 ||
            payload.size() != 28 + std::size_t(vertex_count) * 40 + std::size_t(index_count) * 2)
            throw std::runtime_error("Kind-8 GL vertex/index extent changed");
        civ5::ui::MinimapViewportRing ring;
        ring.vertices.reserve(vertex_count);
        ring.indices.reserve(index_count);
        for (std::size_t i = 0; i < vertex_count; ++i) {
            const auto base = 28 + i * 40;
            ring.vertices.push_back({{at<float>(payload, base), at<float>(payload, base + 4),
                                      at<float>(payload, base + 8)},
                                     at<std::uint32_t>(payload, base + 24)});
        }
        for (std::size_t i = 0; i < index_count; ++i)
            ring.indices.push_back(at<std::uint16_t>(payload, 28 + vertex_count * 40 + i * 2));
        if (rebuild_ring) {
            std::vector<civ5::ui::MinimapViewportCorners> quads(vertex_count / 12);
            for (std::size_t quad = 0; quad < quads.size(); ++quad)
                for (std::size_t corner = 0; corner < 4; ++corner)
                    quads[quad][corner] = ring.vertices[quad * 12 + corner].position;
            auto rebuilt = civ5::ui::build_minimap_viewport_ring(quads, bounds, 313);
            if (rebuilt.indices != ring.indices || rebuilt.vertices.size() != ring.vertices.size())
                throw std::runtime_error("Ported kind-8 mesh topology differs from retail");
            for (std::size_t vertex = 0; vertex < ring.vertices.size(); ++vertex)
                if (rebuilt.vertices[vertex].color != ring.vertices[vertex].color ||
                    std::memcmp(rebuilt.vertices[vertex].position.data(),
                                ring.vertices[vertex].position.data(), 3 * sizeof(float)))
                    throw std::runtime_error("Ported kind-8 vertex bits differ from retail");
            ring = std::move(rebuilt);
        }
        const float shift = std::stof(argv[3]);
        auto context = std::make_shared<civ5::graphics::GlContext>();
        civ5::graphics::GlRenderApi gl(*context);
        auto target = civ5::graphics::GlTexture::render_target(
            *context, 320, 190, 1, civ5::graphics::RenderTextureFormat::rgba8);
        GLuint framebuffer = 0;
        gl.CreateFramebuffers(1, &framebuffer);
        gl.NamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, target.id(), 0);
        const GLenum attachment = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(framebuffer, 1, &attachment);
        if (gl.CheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("Kind-8 GL framebuffer is incomplete");
        gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        if (has_background) {
            const auto rgba = background_rgba(argv[4]);
            context->api().TextureSubImage2D(target.id(), 0, 0, 0, 320, 190,
                                             GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
            context->check("Upload kind-8 GL minimap background");
        } else {
            const float background[4]{32 / 255.0f, 48 / 255.0f, 64 / 255.0f, 1.0f};
            gl.ClearNamedFramebufferfv(framebuffer, GL_COLOR, 0, background);
        }
        {
            GlMinimapViewportRingPass pass(context);
            pass.draw(ring, bounds, shift);
        }
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        save_ppm(target, argv[2]);
        gl.DeleteFramebuffers(1, &framebuffer);
        context->check("Kind-8 GL offscreen probe");
        std::cout << "GL kind-8 frame: " << vertex_count << " vertices, "
                  << index_count / 3 << " triangles, shift=" << shift
                  << ", nativeMeshExact=" << (rebuild_ring ? 1 : 0) << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
