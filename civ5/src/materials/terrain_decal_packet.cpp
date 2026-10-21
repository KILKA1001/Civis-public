#include "civ5/materials/terrain_decal_packet.hpp"
#include "civ5/materials/material_generation.hpp"
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
std::optional<TerrainDecalPackedGeometry> pack_terrain_decal_geometry(
    const terrain::TerrainDecalGeometry &geometry, std::array<float, 4> domain) {
    if (geometry.indices.size() % 3)
        throw std::invalid_argument("Incomplete terrain decal triangle");
    if (geometry.indices.empty() || geometry.indices.size() > 65534) return std::nullopt;
    if (geometry.vertices.empty() || geometry.vertices.size() > 65536)
        throw std::invalid_argument("Terrain decal source vertex count");
    for (float f : domain)
        if (!std::isfinite(f)) throw std::invalid_argument("Nonfinite terrain decal domain");
    if (!(domain[0] < domain[2] && domain[1] < domain[3]))
        throw std::invalid_argument("Invalid terrain decal domain");
    for (auto i : geometry.indices)
        if (i >= geometry.vertices.size()) throw std::invalid_argument("Terrain decal index out of bounds");
    const auto &first = geometry.vertices.front();
    TerrainDecalPackedGeometry result{{first.x, first.y, first.x, first.y}, false, {}};
    auto &b = result.bounds;
    for (const auto &v : geometry.vertices) {
        if (!std::isfinite(v.x) || !std::isfinite(v.y))
            throw std::invalid_argument("Nonfinite terrain decal position");
        // Strict comparisons retain the first signed zero on equality.
        if (b[2] < v.x) b[2] = v.x;
        if (b[3] < v.y) b[3] = v.y;
        if (v.x < b[0]) b[0] = v.x;
        if (v.y < b[1]) b[1] = v.y;
    }
    if (b[0] < domain[0]) b[0] = domain[0];
    if (b[1] < domain[1]) b[1] = domain[1];
    if (domain[2] < b[2]) b[2] = domain[2];
    if (domain[3] < b[3]) b[3] = domain[3];
    const float width = -b[0] + b[2], height = -b[1] + b[3];
    if (!(width > 0 && height > 0) || !std::isfinite(width) || !std::isfinite(height))
        throw std::invalid_argument("Invalid clipped terrain decal extent");
    const float inverse_x = 1.f / width, inverse_y = 1.f / height;
    if (!std::isfinite(inverse_x) || !std::isfinite(inverse_y))
        throw std::invalid_argument("Terrain decal reciprocal overflow");
    // Original constant 9F8D5C, verified by the submission oracle.
    result.large = width > 256.f || height > 256.f;
    result.vertices.reserve(geometry.indices.size());
    for (auto i : geometry.indices) {
        const auto &v = geometry.vertices[i];
        const float x = (-b[0] + v.x) * inverse_x;
        const float y = (v.y + -b[1]) * inverse_y;
        result.vertices.push_back({material_half(x), material_half(y), material_half(v.u), material_half(v.v)});
    }
    return result;
}
} // namespace civ5::materials
