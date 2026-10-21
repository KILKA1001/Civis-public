#include "civ5/materials/material_cache_settings.hpp"
#include <algorithm>

namespace civ5::materials {
namespace {
struct AdapterRule {
    std::uint32_t device_id, vendor_id, preset, flags;
};
constexpr std::array<AdapterRule, 515> adapter_rules{{
#include "material_adapter_rules.inc"
}};
static_assert(adapter_rules.back().device_id == 0 && adapter_rules.back().preset == 6);
struct Preset {
    std::uint32_t detail, tessellation, moving_budget, still_budget;
    std::array<std::uint32_t, 5> pools;
};
constexpr std::array<Preset, 6> presets{{
    {1, 0, 0, 1, {2048, 256, 128, 64, 0}},
    {2, 1, 0, 1, {2048, 256, 128, 64, 0}},
    {2, 0, 0, 1, {4096, 512, 256, 64, 0}},
    {3, 1, 1, 2, {4096, 512, 256, 64, 0}},
    {3, 2, 2, 4, {4096, 512, 256, 64, 0}},
    {3, 2, 3, 6, {3072, 512, 256, 64, 0}},
}};
} // namespace
MaterialCacheConfiguredStartup material_cache_configure(const MaterialCacheAdapterInput &input,
                                                        const MaterialCacheSettingsOverrides &saved) {
    const auto rule = std::find_if(adapter_rules.begin(), adapter_rules.end(), [&](const auto &candidate) {
        return !candidate.device_id ||
               (candidate.device_id == input.device_id && candidate.vendor_id == input.vendor_id);
    });
    MaterialCacheStartup result;
    result.adapter_rule_index = static_cast<std::uint32_t>(rule - adapter_rules.begin());
    result.preset = rule->preset;
    result.flags = rule->flags;
    if (result.preset == 6) {
        if (input.renderer_code == 10)
            result.preset = 3;
        else if (input.renderer_code >= 7 && input.renderer_code <= 9)
            result.preset = 1;
        else {
            result.preset = 0;
            result.flags |= 2;
        }
    }
    if ((input.renderer_code < 7 && result.preset > 1) || input.os_32bit)
        result.preset = 1;
    const auto &preset = presets[result.preset];
    auto pools = preset.pools;
    for (std::size_t i = 0; i < saved.pool_counts.size(); ++i)
        if (saved.pool_counts[i])
            pools[i] = *saved.pool_counts[i];
    result.terrain_detail = saved.detail.value_or(preset.detail);
    result.moving_budget = std::min(saved.moving_budget.value_or(preset.moving_budget), 8u);
    result.still_budget = std::min(saved.still_budget.value_or(preset.still_budget), 8u);
    const auto minimize = saved.minimize_gray_tiles.value_or(0);
    result.force_fast = minimize != 0;
    result.quality = normalize_material_cache_quality({input.renderer_code, result.terrain_detail,
                                                       saved.tessellation.value_or(preset.tessellation), 0,
                                                       result.flags, pools});
    result.derived_tessellation =
        result.quality.tessellation && input.renderer_code >= 10 ? saved.derived_tessellation.value_or(0) : 0;
    return {result, minimize, saved.block_on_load.value_or(0)};
}
std::int32_t material_cache_dx9_renderer_code(const MaterialCacheDx9Capabilities &caps) {
    const auto version = caps.pixel_shader_version & 0xffffu;
    if (version >= 0x300u)
        return 6;
    if (version >= 0x200u) {
        if (caps.ps20_num_temps >= 22 && (caps.ps20_caps & 31u) == 31u)
            return 4;
        if (caps.ps20_num_temps >= 32 && (caps.ps20_caps & 16u) != 0)
            return 5;
        return 3;
    }
    if (version >= 0x101u && version <= 0x103u)
        return 1;
    return version == 0x104u ? 2 : 0;
}
MaterialCacheStartup material_cache_startup(const MaterialCacheAdapterInput &input) {
    return material_cache_configure(input, {}).material;
}
} // namespace civ5::materials
