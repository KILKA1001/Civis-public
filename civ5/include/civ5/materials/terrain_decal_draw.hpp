#pragma once
#include "civ5/materials/terrain_decal_store.hpp"

namespace civ5::materials {
struct TerrainDecalDraw {
    std::uint32_t batch = 0, record = 0;
    std::uint32_t index_buffer = 0, vertex_buffer = 0, first_vertex = 0, triangles = 0;
    std::uint16_t albedo = 0, heightmap = 0;
    // Original 71DF50 writes 100 bytes of the 112-byte shader allocation:
    // matrix rows, texture scale/offset (1,1,0,0), bounds, Z/priority.
    // The final three padding floats are not initialized by this producer.
    std::array<float, 25> constants{};
};
struct TerrainDecalDrawList {
    std::uint32_t selected = 0;
    bool capacity_exhausted = false;
    std::vector<TerrainDecalDraw> draws;
};
// Original 71DF50 selection, MSVC priority ordering and draw payloads.
// available_commands is the explicit 8E3570 result; the original reserves
// 1,000 commands using unsigned subtraction and requires a STRICT surplus.
// mode is compared as a full word against the stored one-bit mode.
// The caller owns the identity U16 index buffer, shader 0x4E/technique 0,
// render target and submission. Consume before mutating/uploading the store.
// Finite ordered query bounds and finite selected priorities are required.
// Queries whose expanded minimum maps beyond the grid are rejected: the
// original iterator reads its first cell without an upper-bound check.
TerrainDecalDrawList prepare_terrain_decal_draws(
    const TerrainDecalStore &, std::array<float, 4> bounds, std::uint32_t mode,
    std::uint32_t available_commands, const std::array<float, 16> &matrix,
    std::uint32_t index_buffer);
} // namespace civ5::materials
