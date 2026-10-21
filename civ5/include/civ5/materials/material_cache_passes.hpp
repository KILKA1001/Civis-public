#pragma once
#include "material_cache.hpp"
#include "lighting_sources.hpp"
#include "terrain_decal_draw.hpp"

namespace civ5::materials {
struct MaterialCachePassSettings {
    std::uint32_t temporary_size, patch_size, shadow_samples, view;
    bool compression, cache_mode, alternate, specular;
    terrain::X87Precision precision;
};
struct MaterialCachePassPatch {
    std::uint32_t columns, rows;
    std::array<std::int32_t, 2> world_rebase;
    std::uint32_t continent_mask, world_height, alternate_height;
    std::array<std::uint32_t, 2> higher_cache, final_cache;
    // Cursors returned by the material/lighting geometry append operation.
    std::uint32_t material_base_vertex, lighting_base_vertex, start_index;
};
struct MaterialCachePassSource {
    // Resource handles in MaterialCacheRole order, retaining full U32 bits.
    std::array<std::uint32_t, 13> textures;
    // Per size (64/128/256/512): compressed D/S, uncompressed D/S.
    std::array<std::array<std::uint32_t, 4>, 4> compression_textures;
    std::uint32_t depth_surface, fallback_texture;
    std::uint32_t normal_vertex_buffer, normal_index_buffer, lighting_vertex_buffer;
    std::uint32_t material_vertex_buffer, patch_index_buffer;
    std::array<std::uint32_t, 4> shader_bindings; // blend, normal, lighting, compression
    MaterialLightingResources lighting;
};
struct MaterialCachePassPalette {
    std::span<const std::int32_t> selected, source_rows, source_modes;
    std::span<const float> row_scales;
    std::span<const std::uint32_t, 64> row_flags;
    std::span<const std::uint32_t> diffuse, specular, height;
    float global_minimum, global_maximum;
};
// Original 8E6D60 zeroes the entire packet before its producer writes fields.
// These are resource indices and draw parameters, never original-process
// pointers or GL object names. Geometry ordering follows the original consumer.
struct MaterialCachePacket {
    std::uint32_t key;
    std::array<std::uint32_t, 5> geometry; // IB, VB, primitives, start index, base vertex
    std::array<std::uint16_t, 16> pixel_textures, pixel_samplers;
    std::array<std::uint16_t, 2> vertex_textures, vertex_samplers;
    std::uint8_t pixel_count, vertex_count;
};
struct MaterialCachePassDraw {
    std::uint32_t packet, shader_binding, selector, constant_offset, constant_bytes;
};
enum class MaterialCachePassKind : std::uint32_t { height, normal, shadow, lighting, compress, copy, decal, decal_shadow };
struct MaterialCachePass {
    MaterialCachePassKind kind;
    std::array<std::uint32_t, 4> targets{};
    std::uint32_t mip = 0, depth_surface = 0, blend_state = 0, depth_state = 9, flags = 0;
    std::array<float, 4> clear_color{};
    float clear_depth = 1.f;
    std::uint32_t copy_source = 0, copy_destination = 0;
    std::vector<MaterialCachePassDraw> draws;
};
struct MaterialCachePassPlan {
    std::vector<MaterialCachePass> passes; // Original submission order, including copies.
    std::vector<MaterialCachePacket> packets; // Owned once; draws may share a packet.
    std::vector<std::uint8_t> constants;
    std::array<float, 8> normal_constants;
};
struct MaterialCacheDecalLayers {
    std::array<TerrainDecalDrawList, 2> layers;
    std::uint32_t shader_binding;
};
// Original 64A7B0 uses columns-2 for BOTH upper bounds, even on rectangular
// geometry. Origin is owner+DC/E0, distinct from the material world rebase.
// Projection uses the verified scalar D3DX path and explicit x87 precision.
// Snapshot the uploaded store immediately before building/executing its plan.
MaterialCacheDecalLayers prepare_material_cache_decals(
    const MaterialCachePassSettings &, const MaterialCachePassPatch &,
    std::array<std::int32_t, 2> origin, const TerrainDecalStore &,
    std::uint32_t index_buffer, std::uint32_t shader_binding,
    std::array<std::uint32_t, 2> available_commands);
std::size_t material_cache_constant_bytes(const MaterialCachePassSettings &settings,
                                         const MaterialCacheDecalLayers *decals = nullptr);
// Complete 64A7B0 pass/packet/constant producer. Its prepared geometry
// boundary is explicit. Source/depth/pool selection and GPU execution belong
// to the caller. Each arena byte and unused normal lane has explicit entering
// state; the plan owns the resulting snapshots for the lifetime of its draws.
MaterialCachePassPlan material_cache_pass_plan(const MaterialCachePassSettings &settings,
                                               const MaterialCachePassPatch &patch,
                                               const MaterialCachePassSource &source,
                                               const MaterialCachePassPalette &palette,
                                               std::span<const std::uint8_t> entering_constants,
                                               const std::array<float, 8> &entering_normal,
                                               const MaterialCacheDecalLayers *decals = nullptr);
} // namespace civ5::materials
