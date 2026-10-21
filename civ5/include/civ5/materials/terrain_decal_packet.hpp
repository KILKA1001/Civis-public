#pragma once
#include "civ5/terrain/terrain_decal_geometry.hpp"

namespace civ5::materials {
struct TerrainDecalPackedGeometry {
    // Clipped source AABB: min X/Y, max X/Y. Unused source vertices count.
    std::array<float, 4> bounds;
    bool large;
    // Original triangle order, with shared vertices expanded. Each vertex is
    // D3DX half X/Y/U/V (8 bytes), not a MaterialPatchMesh vertex.
    std::vector<std::array<std::uint16_t, 4>> vertices;
};
// Geometry portion of original 69E140. Domain is provider+6C..78, in the same
// coordinates as the input. XY is normalized relative to the clipped AABB;
// original XY/UV are not clipped, so packed values may lie outside [0,1].
// Empty or excessive triangle counts return nullopt as in the original.
// Malformed indices, nonfinite XY/domain or a nonpositive/nonfinite clipped
// extent/inverse are rejected before producing a packet. Source count <=65536.
// UV accepts every float bit pattern, using original D3DX saturation semantics.
// No handles, batch allocation, refresh callbacks or GPU resources are owned.
std::optional<TerrainDecalPackedGeometry> pack_terrain_decal_geometry(
    const terrain::TerrainDecalGeometry &geometry, std::array<float, 4> domain);
} // namespace civ5::materials
