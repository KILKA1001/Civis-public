#pragma once
#include "terrain_occurrences.hpp"

namespace civ5::terrain {
struct TerrainDecalRange {
    std::uint32_t first, end;
};
// Original 5E4AC0 uses one 15-bit rand, even for ranges larger than 32767.
// Empty/singleton ranges return first without advancing the stream. This only
// selects an index; the caller must resolve it against its own record storage.
std::uint32_t select_terrain_decal(TerrainDecalRange range, TerrainRandom &random);

struct TerrainDecalCell {
    // The geometry owner's first four words are retained without interpreting
    // its pointers/list links as owned x64 addresses.
    std::array<std::uint32_t, 4> geometry;
    std::uint32_t handle;
    std::uint8_t dirty;
    std::array<std::uint8_t, 3> reserved;
};
inline constexpr std::uint32_t absent_terrain_decal = 0xffffffffu;
using TerrainDecalRemove = std::function<void(std::size_t, std::uint32_t)>;
using TerrainDecalRebuild = std::function<std::uint32_t(std::size_t, const TerrainDecalCell &)>;

// Original 76DF40 + 79CC10: y-then-x traversal of x-major cells, removal,
// rebuild, then dirty-byte clearing. Rebuild owns geometry/variant selection;
// no random draws or geometry creation are silently skipped here. Callbacks
// must not resize/reenter or mutate the grid. If one throws, completed cells
// remain committed and the current dirty byte stays set; a successfully removed
// handle is cleared before rebuilding. Scene grid/palette initialization and
// geometry inputs remain separate; refresh_terrain_decal_geometry supplies the
// owned 768DA0 implementation and loaded family-zero palette.
void refresh_terrain_decals(std::int32_t width, std::int32_t height,
                            std::span<TerrainDecalCell> cells,
                            const TerrainDecalRemove &remove, const TerrainDecalRebuild &rebuild);
} // namespace civ5::terrain
