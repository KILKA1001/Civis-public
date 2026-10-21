#pragma once
#include "civ5/assets/dds.hpp"
#include "civ5/terrain/native_math.hpp"
#include <functional>

namespace civ5::materials {
struct DistanceFogState {
    bool loaded = false, enabled = false;
    float density = 0, strength = 0;
    terrain::Vector3 color{};
};
struct DistanceFogFrame {
    terrain::Vector4 color{}, parameters{};
};
// Original 777AC0: first three cells of FogColor.csv, atof -> float, then
// MULSS by the stored float reciprocal of 255. Missing/nonfinite cells and
// unsafe CR-only line endings fail before publishing state. Authored LF/CRLF
// files avoid the original warning/uninitialized-color and index-overflow
// paths.
DistanceFogState distance_fog_startup(std::span<const std::uint8_t> csv);
// Original 453E96..453F2C. An unloaded owner preserves all prior bits.
// The fourth parameter is the camera's normalized zoom (D80DFC), without clamp.
DistanceFogFrame distance_fog_frame(const DistanceFogState &state, float normalized_zoom,
                                    const DistanceFogFrame &prior);

class DistanceFogAssets {
  public:
    static constexpr const char *color_path = "assets/DistanceFog/FogColor.csv";
    static constexpr const char *ramp_path = "assets/DistanceFog/FogRamp.dds";
    static DistanceFogAssets load(const std::function<assets::AssetHandle(const char *)> &resolve);
    const DistanceFogState &state() const { return state_; }
    const assets::DdsData &ramp() const { return ramp_; }
    const assets::AssetHandle &color_asset() const { return color_asset_; }
    const assets::AssetHandle &ramp_asset() const { return ramp_asset_; }
    const assets::Bytes &color_bytes() const { return color_bytes_; }

  private:
    DistanceFogAssets(assets::AssetHandle color, assets::AssetHandle ramp, DistanceFogState state,
                      assets::DdsData data, assets::Bytes color_bytes);
    assets::AssetHandle color_asset_, ramp_asset_;
    DistanceFogState state_;
    assets::DdsData ramp_;
    assets::Bytes color_bytes_;
};
} // namespace civ5::materials
