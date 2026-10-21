#pragma once
#include "civ5/materials/material_cache_settings.hpp"
#include <array>
#include <d3d9.h>
#include <filesystem>
#include <memory>
#include <shlobj.h>
#include <shlwapi.h>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>

namespace material_preview_settings {
using namespace civ5::materials;
struct Paths {
    std::filesystem::path graphics, game;
};
struct Profile {
    MaterialCacheAdapterInput adapter;
    MaterialCacheDx9Capabilities capabilities;
    MaterialCacheConfiguredStartup configured;
    MaterialLoadSettings loading;
    Paths paths;
    bool graphics_loaded = false, game_loaded = false;
    bool documents_found = false;
    std::optional<std::uint32_t> texture_quality_override;
};
inline void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
inline std::filesystem::path ini_path(const std::wstring &directory, const wchar_t *filename) {
    require(directory.size() < MAX_PATH, "Material profile directory exceeds original MAX_PATH");
    std::array<wchar_t, MAX_PATH> path{}, normalized{};
    std::copy(directory.begin(), directory.end(), path.begin());
    // Original 7A0030 and 4DC8C0 both call 407FB0 around PathAppendW.
    PathRemoveBlanksW(path.data());
    if (PathCanonicalizeW(normalized.data(), path.data()))
        path = normalized;
    require(PathAppendW(path.data(), filename) != FALSE, "Cannot append material INI filename");
    PathRemoveBlanksW(path.data());
    if (PathCanonicalizeW(normalized.data(), path.data()))
        path = normalized;
    return path.data();
}
inline Paths profile_paths(std::wstring documents, bool found) {
    std::wstring directory;
    if (found) {
        directory = std::move(documents);
        if (directory.empty() || directory.back() != L'\\')
            directory += L'\\';
        directory += L"My Games\\Sid Meier's Civilization 5\\";
    }
    return {ini_path(directory, L"GraphicsSettingsDX9.ini"), ini_path(directory, L"UserSettings.ini")};
}
inline Profile load(const MaterialCacheAdapterInput &adapter, Paths paths) {
    Profile result;
    result.adapter = adapter;
    result.paths = std::move(paths);
    const auto graphics = load_material_cache_ini(result.paths.graphics);
    const auto game = load_material_load_ini(result.paths.game);
    result.configured = material_cache_configure(adapter, graphics.overrides);
    result.texture_quality_override = graphics.overrides.texture_quality;
    result.loading = game.settings;
    result.graphics_loaded = graphics.loaded;
    result.game_loaded = game.loaded;
    return result;
}
inline Profile from_observed_adapter(const MaterialCacheAdapterInput &adapter,
                                     const MaterialCacheDx9Capabilities &capabilities,
                                     const Paths *explicit_paths = nullptr) {
    require(material_cache_dx9_renderer_code(capabilities) == adapter.renderer_code,
            "Observed adapter classifier differs from recorded capabilities");
    std::array<wchar_t, MAX_PATH> documents{};
    // Explicit adapter data still uses the current read-only game/graphics INI paths.
    const bool found = SHGetSpecialFolderPathW(nullptr, documents.data(), CSIDL_PERSONAL, FALSE) != FALSE;
    auto result = load(adapter, explicit_paths ? *explicit_paths : profile_paths(documents.data(), found));
    result.capabilities = capabilities;
    result.documents_found = explicit_paths ? true : found;
    return result;
}
inline Profile discover() {
    struct Release {
        void operator()(IDirect3D9 *value) const { value->Release(); }
    };
    const std::unique_ptr<IDirect3D9, Release> api(Direct3DCreate9(D3D_SDK_VERSION));
    require(bool(api), "Cannot query original DX9 adapter");
    D3DADAPTER_IDENTIFIER9 identifier{};
    D3DCAPS9 caps{};
    require(SUCCEEDED(api->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &identifier)),
            "Cannot query original adapter identity");
    require(SUCCEEDED(api->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps)),
            "Cannot query original adapter capabilities");
    MaterialCacheAdapterInput adapter;
    adapter.vendor_id = identifier.VendorId;
    adapter.device_id = identifier.DeviceId;
    const MaterialCacheDx9Capabilities capabilities{caps.PixelShaderVersion, caps.PS20Caps.Caps,
                                                    caps.PS20Caps.NumTemps};
    adapter.renderer_code = material_cache_dx9_renderer_code(capabilities);
    // The original x86 WOW64 predicate describes the OS, not this process.
    SYSTEM_INFO system{};
    GetNativeSystemInfo(&system);
    switch (system.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
    case PROCESSOR_ARCHITECTURE_IA64:
    case PROCESSOR_ARCHITECTURE_ARM64:
        adapter.os_32bit = false;
        break;
    case PROCESSOR_ARCHITECTURE_INTEL:
    case PROCESSOR_ARCHITECTURE_ARM:
        adapter.os_32bit = true;
        break;
    default:
        throw std::runtime_error("Unknown native OS architecture");
    }
    return from_observed_adapter(adapter, capabilities);
}
inline std::string report(const Profile &profile) {
    const auto &s = profile.configured.material;
    const auto &q = s.quality;
    std::ostringstream out;
    out << "{\"vendorId\":" << profile.adapter.vendor_id << ",\"deviceId\":" << profile.adapter.device_id
        << ",\"rendererCode\":" << profile.adapter.renderer_code << ",\"os32\":" << profile.adapter.os_32bit
        << ",\"pixelShaderVersion\":" << profile.capabilities.pixel_shader_version
        << ",\"ps20Caps\":" << profile.capabilities.ps20_caps
        << ",\"ps20NumTemps\":" << profile.capabilities.ps20_num_temps
        << ",\"documentsFound\":" << profile.documents_found
        << ",\"graphicsLoaded\":" << profile.graphics_loaded << ",\"gameLoaded\":" << profile.game_loaded
        << ",\"adapterRule\":" << s.adapter_rule_index << ",\"preset\":" << s.preset
        << ",\"flags\":" << s.flags << ",\"detail\":" << s.terrain_detail
        << ",\"textureQualityOverride\":";
    if (profile.texture_quality_override)
        out << *profile.texture_quality_override;
    else
        out << "null";
    out
        << ",\"temporarySize\":" << q.temporary_size << ",\"cacheSize\":" << q.cache_size
        << ",\"baseSize\":" << q.base_size << ",\"cacheMode\":" << q.cache_mode
        << ",\"compression\":" << q.compression << ",\"tessellation\":" << q.tessellation
        << ",\"movingBudget\":" << s.moving_budget << ",\"stillBudget\":" << s.still_budget
        << ",\"minimizeGrayTiles\":" << profile.configured.minimize_gray_tiles
        << ",\"blockOnLoad\":" << profile.configured.block_on_load
        << ",\"minimalBuildOnLoad\":" << profile.loading.minimal_build_on_load << ",\"pools\":[";
    for (std::size_t i = 0; i < q.pool_counts.size(); ++i)
        out << (i ? "," : "") << q.pool_counts[i];
    out << "],\"createdProbeDevice\":false,\"profileReadOnly\":true,\"fullGraphicsParity\":false}\n";
    return out.str();
}
} // namespace material_preview_settings
