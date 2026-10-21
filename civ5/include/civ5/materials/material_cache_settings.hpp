#pragma once
#include "civ5/materials/material_cache_startup.hpp"
#include <filesystem>
#include <optional>
#include <string_view>

namespace civ5::materials {
// Raw DWORD overrides. Absence retains the original adapter preset; a present
// zero is an actual override. Values are normalized only after INI lookup.
struct MaterialCacheSettingsOverrides {
    std::optional<std::uint32_t> moving_budget, still_budget;
    std::optional<std::uint32_t> minimize_gray_tiles, block_on_load;
    std::optional<std::uint32_t> detail, tessellation, derived_tessellation;
    // Separate from terrain detail; zero enables the original DX9_LOW FPK loop.
    std::optional<std::uint32_t> texture_quality;
    std::array<std::optional<std::uint32_t>, 4> pool_counts;
};
struct MaterialCacheConfiguredStartup {
    MaterialCacheStartup material;
    // Preserve the original globals as DWORDs, including noncanonical values.
    std::uint32_t minimize_gray_tiles = 0, block_on_load = 0;
};
struct MaterialCacheIniLoad {
    bool loaded = false;
    MaterialCacheSettingsOverrides overrides;
};
// Fresh original startup, saved overrides, then original quality normalization.
// Does not query hardware or modify the INI document/file.
MaterialCacheConfiguredStartup material_cache_configure(const MaterialCacheAdapterInput &,
                                                        const MaterialCacheSettingsOverrides &);
// Original flags-0 byte/BOM/CR/NUL/DEL behavior and first case-insensitive
// section/key match. Wide lines use Windows CP_ACP, not assumed UTF-8.
MaterialCacheSettingsOverrides parse_material_cache_ini(std::string_view bytes);
// An absent/unopenable file returns loaded=false with no overrides. Empty
// opened files are loaded=true. Read/conversion errors throw; no file writes.
MaterialCacheIniLoad load_material_cache_ini(const std::filesystem::path &);
// GameSettings belongs to a separate INI owner from GraphicsDetailSettings.
struct MaterialLoadSettings {
    bool minimal_build_on_load = true;
};
struct MaterialLoadIniResult {
    bool loaded = false;
    MaterialLoadSettings settings;
};
MaterialLoadSettings parse_material_load_ini(std::string_view bytes);
MaterialLoadIniResult load_material_load_ini(const std::filesystem::path &);
// Original 403767 caller prerequisite. When true, the caller must prepare
// the camera frame before submitting the initial queue with culling enabled.
bool material_cache_initial_cull(const MaterialLoadSettings &, bool renderer_present,
                                 std::uint32_t renderer_mode);
} // namespace civ5::materials
