#pragma once
#include "civ5/assets/dds.hpp"
#include "terrain_decal_updates.hpp"

namespace civ5::terrain {
inline constexpr std::array<std::string_view, 10> terrain_decal_families{
    "Detail_Road", "Main_Road", "Crops", "Tree_Shadow", "Floodplains", "Floodplains_EndCap1",
    "Floodplains_EndCap2", "Riverbank", "Riverbank_EndCap1", "Riverbank_EndCap2"};
struct TerrainDecalRow {
    float priority;
    std::optional<std::string> albedo, heightmap, specular;
};
struct TerrainDecalDefinition {
    // Absent families leave the prior native range unchanged on reload.
    std::array<std::optional<std::vector<TerrainDecalRow>>, 10> families;
};
// The authored attribute/element-only XML subset. A present family must have
// at least one entry. Malformed numeric/path values and duplicate families are
// rejected; unknown families/attributes are ignored. Child tag names carry no
// selection semantics. Family enumeration follows the native fixed table.
TerrainDecalDefinition parse_terrain_decal_definition(std::string_view xml);
struct TerrainDecalRecord {
    float priority;
    std::uint32_t albedo, heightmap, specular;
};
struct TerrainDecalPalette {
    std::array<TerrainDecalRange, 10> ranges{};
    std::vector<TerrainDecalRecord> records;
};
using TerrainDecalResource = std::function<std::uint32_t(std::string_view)>;
// Original 714FD0 record/family writes with explicit resource transport.
// Missing albedo/height use zero; missing specular uses the caller's blank
// resource. Existing records are retained, even when a family is replaced.
// Resource IDs belong to the caller; this function never acquires/releases them.
TerrainDecalPalette append_terrain_decal_definition(const TerrainDecalPalette &prior,
    const TerrainDecalDefinition &definition, std::uint32_t blank_specular,
    const TerrainDecalResource &resolve);

struct TerrainDecalTexture {
    assets::AssetHandle asset;
    assets::DdsData data;
};
class TerrainDecalAssets {
  public:
    // Sources must resolve exact authored resource paths. Any VFS aliases and
    // package precedence belong to that supplied source, not this loader.
    static TerrainDecalAssets load(assets::AssetHandle definition,
                                    const assets::AssetSource &textures);
    const assets::AssetHandle &definition() const { return definition_; }
    const TerrainDecalPalette &palette() const { return palette_; }
    // Local IDs 1..N; missing attributes use the shared renderer's resource
    // zero, not a null GPU binding. Every decoded texture retains its source
    // handle and is decoded once per normalized key.
    std::span<const TerrainDecalTexture> textures() const { return textures_; }
    std::uint32_t blank_specular() const { return blank_specular_; }
  private:
    explicit TerrainDecalAssets(assets::AssetHandle definition) : definition_(std::move(definition)) {}
    assets::AssetHandle definition_;
    TerrainDecalPalette palette_;
    std::vector<TerrainDecalTexture> textures_;
    std::uint32_t blank_specular_ = 0;
};
} // namespace civ5::terrain
