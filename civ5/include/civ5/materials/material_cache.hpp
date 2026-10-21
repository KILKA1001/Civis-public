#pragma once
#include "civ5/terrain/native_math.hpp"
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace civ5::materials {
// Explicit entering quality/capability controls. No assumed live-game settings.
// Pool order is 64, 128, 256, 512, compressed; counts may be normalized before
// allocation. The renderer capability code uses the original signed ordering.
struct MaterialCacheQualityInput {
    std::int32_t renderer_code;
    std::uint32_t detail, tessellation, compression, flags;
    std::array<std::uint32_t, 5> pool_counts;
};
struct MaterialCacheQuality {
    std::uint32_t temporary_size, cache_size, base_size, cache_mode, compression, tessellation;
    std::array<std::uint32_t, 5> pool_counts;
    bool operator==(const MaterialCacheQuality &) const = default;
};
// Material-related projection of 5E01D0. Does not claim to normalize unrelated
// shadow/leader/UI settings. 5A1B60 does not modify any of these output fields.
MaterialCacheQuality normalize_material_cache_quality(const MaterialCacheQualityInput &input);

enum class MaterialCacheFormat : std::uint32_t {
    compression_intermediate = 2, // Raw native code; physical semantics still unproven.
    rgba8 = 21,
    rgb565 = 23,
    rgba16_float = 113,
    dxt1 = 0x31545844
};
enum class MaterialCacheRole : std::uint32_t {
    render_temporary,
    height_temporary,
    specular_temporary,
    shadow_temporary,
    normal_temporary,
    albedo_temporary,
    specular_color_temporary,
    normal_cache,
    albedo_cache,
    decal_cache,
    render_cache,
    height_cache,
    shadow_cache,
    compression_diffuse,
    compression_specular,
    compression_diffuse_color,
    compression_specular_color,
    patch_64,
    patch_128,
    patch_256,
    patch_512,
    patch_compressed
};
struct MaterialCacheTexturePlan {
    MaterialCacheRole role;
    std::uint32_t index, width, height, requested_levels;
    MaterialCacheFormat format;
    std::uint32_t native_usage, native_pool, native_accounted_bytes;
    bool operator==(const MaterialCacheTexturePlan &) const = default;
};
// Thirteen original temporary/cache allocations plus sixteen compression
// intermediates when enabled. Requested level 0 retains the native autogen
// request; effective device mip count is a separate backend decision.
std::vector<MaterialCacheTexturePlan> material_cache_texture_plan(std::uint32_t temporary_size,
                                                                  std::uint32_t cache_size, bool compression);
// Game initializer 549710..5498E7, not the editor's fixed pool sizes. Validates
// the actual loading-owner capacities before constructing descriptors.
std::vector<MaterialCacheTexturePlan> material_cache_pool_plan(std::span<const std::uint32_t, 5> counts,
                                                               bool compression, std::uint32_t base_size);
// Fixed-seed 49B846..49B8D7 producer, including unsigned DIV modulus and
// instruction-level x87 rounding. Entry zero is the unjittered sample.
std::array<std::array<float, 2>, 32> material_shadow_jitter(terrain::X87Precision precision);
} // namespace civ5::materials
