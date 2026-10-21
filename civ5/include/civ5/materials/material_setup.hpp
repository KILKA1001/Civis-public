#pragma once
#include "civ5/terrain/height_owner.hpp"
#include "civ5/terrain/river_raster.hpp"
#include "material_assets.hpp"
#include "material_generation.hpp"

namespace civ5::materials {
// Original 680130 metadata producer. Authored continent rows use the five
// executable prefixes. The appended provider row has code FFFFFFFF.
std::vector<MaterialRow> terrain_provider_rows(std::span<const MaterialRow> authored);
std::vector<std::uint32_t> terrain_material_codes(std::span<const MaterialRow> provider);
// Original 4E2F30 normalized 5x5 factor kernel, in native Y-major sample order.
std::array<MaterialSample, 25> native_material_samples();

struct MaterialRiverSettings {
    std::array<std::uint8_t, 4> remap_from, remap_to;
    float level;
    std::uint32_t river_code;
    terrain::RiverRasterSettings raster(std::span<const std::uint32_t> codes,
                                        terrain::X87Precision precision) const;
};
// Original 6114D0 first-match table searches. A missing destination disables
// its source remap; code 0x140030 chooses the original channel material.
MaterialRiverSettings terrain_river_settings(std::span<const std::uint32_t> codes);

struct MaterialCachePatch {
    MaterialOrigin origin;
    // 6A97D0 passes 65 interior vertices except at the final map edges. Each
    // 701EF0 material stream adds one vertex at both sides (up to 67x67).
    int interior_columns, interior_rows;
    std::array<float, 4> position;
    int columns() const { return interior_columns + 2; }
    int rows() const { return interior_rows + 2; }
};
struct MaterialCacheLayout {
    int columns, rows;
    std::vector<MaterialCachePatch> patches; // X outer, Y inner
};
MaterialCacheLayout material_cache_layout(int width, int height);
// Validate that the consumer sees the owned final U16/U8 page domains. Scales
// come from the same owner metadata used by 49B3E0.
MaterialSourceDomain material_world_domain(const terrain::HeightOwnerState &owner, int normal_x_remap);
} // namespace civ5::materials
