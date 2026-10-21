#pragma once

#include "civ5/ui/minimap_hex_geometry.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace civ5::ui {

// Validation-only kind-9 vertex stream. The saved-plot hex used by the probe
// is a candidate geometry source, not a captured native ice-region handle.
struct MinimapIceProbeMesh {
    std::array<float, 4> box{};
    std::vector<MinimapPoint2> normalized_triangles;
};

// A recovered retail region can contain over 300,000 triangle vertices.
// This remains a validation artifact with a bounded on-disk extent.
inline constexpr std::uint32_t minimap_ice_probe_max_vertices = 1'000'000;

inline void validate_minimap_ice_probe_mesh(const MinimapIceProbeMesh &mesh) {
    if (!(std::isfinite(mesh.box[0]) && std::isfinite(mesh.box[1]) &&
          std::isfinite(mesh.box[2]) && std::isfinite(mesh.box[3]) &&
          mesh.box[2] > mesh.box[0] && mesh.box[3] > mesh.box[1]) ||
        mesh.normalized_triangles.empty() ||
        mesh.normalized_triangles.size() > minimap_ice_probe_max_vertices ||
        mesh.normalized_triangles.size() % 3)
        throw std::invalid_argument("Invalid minimap ice probe mesh");
    for (const auto &point : mesh.normalized_triangles)
        if (!(std::isfinite(point.x) && std::isfinite(point.y) &&
              point.x >= -1.001f && point.x <= 1.001f &&
              point.y >= -1.001f && point.y <= 1.001f))
            throw std::invalid_argument("Invalid minimap ice probe vertex");
}

inline void write_minimap_ice_probe_mesh(const std::filesystem::path &path,
                                         const MinimapIceProbeMesh &mesh) {
    static_assert(std::endian::native == std::endian::little);
    static_assert(sizeof(MinimapPoint2) == 2 * sizeof(float));
    validate_minimap_ice_probe_mesh(mesh);
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot write minimap ice probe mesh");
    constexpr std::array<char, 8> magic{'C', '5', 'I', 'M', 'S', 'H', '0', '1'};
    const auto count = static_cast<std::uint32_t>(mesh.normalized_triangles.size());
    out.write(magic.data(), magic.size());
    out.write(reinterpret_cast<const char *>(&count), sizeof(count));
    out.write(reinterpret_cast<const char *>(mesh.box.data()), sizeof(mesh.box));
    out.write(reinterpret_cast<const char *>(mesh.normalized_triangles.data()),
              mesh.normalized_triangles.size() * sizeof(MinimapPoint2));
    if (!out) throw std::runtime_error("Incomplete minimap ice probe mesh");
}

inline MinimapIceProbeMesh read_minimap_ice_probe_mesh(const std::filesystem::path &path) {
    static_assert(std::endian::native == std::endian::little);
    static_assert(sizeof(MinimapPoint2) == 2 * sizeof(float));
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) throw std::runtime_error("Cannot read minimap ice probe mesh");
    const auto size = in.tellg();
    if (size < 28 || size > 28 + minimap_ice_probe_max_vertices * 8ull)
        throw std::runtime_error("Invalid minimap ice probe mesh length");
    in.seekg(0);
    std::array<char, 8> magic{};
    std::uint32_t count = 0;
    MinimapIceProbeMesh mesh;
    in.read(magic.data(), magic.size());
    in.read(reinterpret_cast<char *>(&count), sizeof(count));
    if (magic != std::array<char, 8>{'C', '5', 'I', 'M', 'S', 'H', '0', '1'} ||
        count == 0 || count > minimap_ice_probe_max_vertices || count % 3 ||
        size != static_cast<std::streamoff>(28 + count * 8))
        throw std::runtime_error("Invalid minimap ice probe mesh header");
    in.read(reinterpret_cast<char *>(mesh.box.data()), sizeof(mesh.box));
    mesh.normalized_triangles.resize(count);
    in.read(reinterpret_cast<char *>(mesh.normalized_triangles.data()), count * 8);
    if (!in) throw std::runtime_error("Truncated minimap ice probe mesh");
    validate_minimap_ice_probe_mesh(mesh);
    return mesh;
}

} // namespace civ5::ui
