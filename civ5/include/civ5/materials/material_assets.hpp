#pragma once
#include "civ5/assets/dds.hpp"
#include "material_sources.hpp"

namespace civ5::materials {
struct MaterialRow {
    std::int32_t continent;
    std::optional<std::string> diffuse, height, specular;
    float dominance, scale;
    std::int32_t projection;
    std::uint32_t local_id;
};
// Bounded unquoted eight-column authored subset. Never skip malformed rows.
std::vector<MaterialRow> parse_material_rows(std::string_view csv);
struct MaterialTexture {
    assets::AssetHandle asset;
    assets::DdsData data;
};
class MaterialAssets {
  public:
    // Exact caller-supplied definition and texture source; no DLC precedence.
    static MaterialAssets load(assets::AssetHandle definition, const assets::AssetSource &textures);
    // Normal terrain provider 680130: authored rows followed by the original
    // Checkerboard.dds row (shared diffuse/height resource, no specular).
    // A later DX9_Low archive replaces only names it contains; absent names
    // continue to resolve from the ordinary DX9 archive.
    static MaterialAssets load_terrain_provider(assets::AssetHandle definition,
                                                const assets::AssetSource &textures,
                                                const assets::AssetSource *later_textures = nullptr);
    const assets::AssetHandle &definition() const { return definition_; }
    std::span<const MaterialRow> rows() const { return rows_; }
    const MaterialExpansion &expansion() const { return expansion_; }
    std::span<const MaterialTexture> textures() const { return textures_; }
    std::span<const std::uint32_t> specular_resources() const { return specular_ids_; }
    const MaterialTexture &texture(const std::string &name) const;
    // Local transport IDs1..N, NOT captured original allocations or GL names.
    // Zero resolves to the recovered resource0; no silent replacement of missing
    // assets.
    std::array<std::uint16_t, 16> selected_resources(std::span<const std::int32_t> selected) const;
    void write_constants(float global_span, std::span<float, 384> destination) const;

  private:
    MaterialAssets(assets::AssetHandle definition, std::vector<MaterialRow> rows);
    static MaterialAssets load_rows(assets::AssetHandle definition, const assets::AssetSource &textures,
                                    bool terrain_provider, const assets::AssetSource *later_textures);
    assets::AssetHandle definition_;
    std::vector<MaterialRow> rows_;
    MaterialExpansion expansion_;
    std::vector<MaterialTexture> textures_;
    std::unordered_map<std::string, std::uint32_t> resource_ids_;
    std::vector<std::uint32_t> diffuse_ids_, height_ids_, specular_ids_;
};
} // namespace civ5::materials
