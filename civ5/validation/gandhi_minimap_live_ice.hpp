#pragma once

#include "civ5/ui/minimap_native_ice_records.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::graphics {

// This is the selected Gandhi diagnostic's passive kind-9 VB/IB replay. The
// capture was made from the original DX9 process at this map state; it is not
// a general ice-region generator and its postprocess graph/RNG are unknown.
inline constexpr std::string_view gandhi_live_ice_save_sha256 =
    "b3a54d5c54d2085236e1c156830643f82dfd78853aee5e1b661c32530a6e82ab";
inline constexpr std::string_view gandhi_live_ice_capture_sha256 =
    "73315af7a35c70e949421a9f08614a6e3aed5a58201b9647f8b4f3e467bc48ce";
inline constexpr std::string_view gandhi_live_ice_record_sha256 =
    "1530f4cb3104131a4c0719463b73fbc52f2baae52b30c74c8fac816b6180605e";

inline void validate_gandhi_live_ice_record(
    std::string_view save_sha256, std::string_view record_sha256,
    const civ5::ui::MinimapNativeIceRecords &records) {
    if (save_sha256 != gandhi_live_ice_save_sha256 ||
        record_sha256 != gandhi_live_ice_record_sha256)
        throw std::runtime_error("Captured Gandhi live ice source identity differs");
    if (!records.shader_box || records.regions.size() != 1 ||
        records.packed_vertices != 19465 || records.indices != 93126 ||
        records.regions[0].vertices.size() != 19465 ||
        records.regions[0].indices.size() != 93126 ||
        records.seed != 0)
        throw std::runtime_error("Captured Gandhi live ice record shape differs");
}

} // namespace civ5::graphics
