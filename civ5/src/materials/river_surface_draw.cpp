#include "civ5/materials/river_surface_draw.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::materials {
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
std::uint32_t capacity(std::size_t count, std::uint32_t stride, std::uint32_t alignment) {
    const auto limit = (std::numeric_limits<std::uint32_t>::max() - (alignment - 1)) / stride;
    require(count <= limit, "River buffer capacity exceeds original uint32 allocation");
    return (std::uint32_t(count) * stride + alignment - 1) & ~(alignment - 1);
}
} // namespace
RiverSurfaceUpload river_surface_upload(const terrain::RiverSurfaceMesh &mesh) {
    const auto vb = capacity(mesh.vertices.size(), 32, 8192);
    const auto ib = capacity(mesh.indices.size(), 4, 4096);
    require(mesh.indices.size() % 3 == 0, "Incomplete river triangle");
    for (const auto &vertex : mesh.vertices)
        for (unsigned lane = 0; lane < 7; ++lane)
            require(std::isfinite(vertex[lane]), "Nonfinite consumed river vertex");
    // Original 5654B0 suppresses the whole upload if either stream is empty.
    const bool active = !mesh.vertices.empty() && !mesh.indices.empty();
    if (active)
        for (const auto index : mesh.indices)
            require(index < mesh.vertices.size(), "River index outside owned vertex stream");
    return {std::uint32_t(mesh.vertices.size()), std::uint32_t(mesh.indices.size()), active ? vb : 0,
            active ? ib : 0, active};
}
std::optional<RiverSurfaceDraw> river_surface_draw(const terrain::RiverSurfaceMesh &mesh,
                                                   const RiverSurfaceResources &resources,
                                                   std::uint32_t quality) {
    const auto upload = river_surface_upload(mesh);
    if (!upload.active)
        return {};
    require(resources.vertex_buffer != 0 && resources.index_buffer != 0,
            "River draw requires allocated vertex and index handles");
    RiverSurfaceDraw draw;
    draw.selector = quality == 1 || quality == 2 ? 4 : 1;
    draw.triangle_count = upload.index_count / 3;
    draw.vertex_capacity = upload.vertex_bytes / 32;
    const auto put = [&](unsigned at, std::uint32_t value, unsigned bytes) {
        for (unsigned i = 0; i < bytes; ++i)
            draw.packet[at + i] = std::uint8_t(value >> (i * 8));
    };
    put(0, 0x5b, 4);
    put(4, resources.index_buffer, 4);
    put(8, resources.vertex_buffer, 4);
    put(12, draw.triangle_count, 4);
    put(0x60, 14, 1);
    constexpr std::array<std::uint16_t, 13> samplers{0, 0, 0, 0, 0, 5, 3, 0, 5, 0, 0, 5, 5};
    for (unsigned slot = 0; slot < resources.pixel_textures.size(); ++slot) {
        put(0x18 + slot * 2, resources.pixel_textures[slot], 2);
        put(0x38 + slot * 2, samplers[slot], 2);
    }
    // Slot 13 remains default texture 0 / sampler 0 in the original packet.
    return draw;
}
} // namespace civ5::materials
