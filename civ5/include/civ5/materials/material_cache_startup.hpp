#pragma once
#include "material_cache.hpp"

namespace civ5::materials {
struct MaterialCacheAdapterInput {
    // Actual PCI VendorId and DeviceId, in D3DADAPTER_IDENTIFIER9 terminology.
    std::uint32_t vendor_id = 0, device_id = 0;
    std::int32_t renderer_code = 0;
    // Original OS predicate, not the pointer width of this native engine.
    bool os_32bit = false;
};
struct MaterialCacheDx9Capabilities {
    std::uint32_t pixel_shader_version = 0, ps20_caps = 0;
    std::int32_t ps20_num_temps = 0;
};
// Original DX9 classifier at 8E3242..8E32E8. The caller supplies actual
// D3DCAPS9 fields; this does not infer capabilities from an OpenGL version.
std::int32_t material_cache_dx9_renderer_code(const MaterialCacheDx9Capabilities &);
struct MaterialCacheStartup {
    std::uint32_t adapter_rule_index = 0, preset = 0, flags = 0;
    std::uint32_t terrain_detail = 0, moving_budget = 0, still_budget = 0;
    bool fast_when_still = true, force_fast = false;
    std::uint32_t derived_tessellation = 0;
    MaterialCacheQuality quality{};
    bool operator==(const MaterialCacheStartup &) const = default;
};
// Material projection of original adapter lookup/presets in 5C8FB4..5C9364,
// followed by 5E01D0 normalization. Uses the original ordered adapter table;
// unknown adapters retain its capability-dependent fallback. This is fresh
// startup without saved INI overrides, not a hardware query or full UI setup.
MaterialCacheStartup material_cache_startup(const MaterialCacheAdapterInput &);
} // namespace civ5::materials
