#pragma once
#include "civ5/assets/asset_source.hpp"
#include <array>
#include <functional>

namespace civ5::materials {
enum class ColorKeySystem { terrain, terrain_fow, terrain_shadow, buildings, rivers };
struct ColorKeyRequest {
    ColorKeySystem system;
    unsigned style;
    std::string native_path;
};
// Original 49DC50 order: system-major, Generic/America/Asia/Africa/Europe.
const std::array<ColorKeyRequest, 25> &color_key_requests();

class ColorKeyVolume {
  public:
    static constexpr unsigned size = 32;
    static constexpr std::size_t byte_count = std::size_t(size) * size * size * 4;
    // Original texel-center callback 426820 and 8E97B0 A8R8G8B8 conversion.
    static ColorKeyVolume identity();
    // Original 8E80E0 row order, then 695990's first 0x20000-byte copy.
    // Supported asset profile: uncompressed 32-bit TGA, no palette or ID,
    // bottom/top origin. Extra atlas pixels and the TGA footer are not a LUT.
    static ColorKeyVolume from_tga(std::span<const std::uint8_t> bytes);
    std::span<const std::uint8_t> bgra() const { return bgra_; }

  private:
    explicit ColorKeyVolume(assets::Bytes bytes) : bgra_(std::move(bytes)) {}
    assets::Bytes bgra_;
};

struct ColorKeyAsset {
    ColorKeyRequest request;
    std::optional<assets::AssetHandle> asset;
    bool authored;
    ColorKeyVolume volume;
};
class ColorKeyAssets {
  public:
    // Resolver chooses source precedence. Missing/empty files retain the
    // original generated identity; ambiguous or malformed assets are errors.
    static ColorKeyAssets
    load(const std::function<std::optional<assets::AssetHandle>(const ColorKeyRequest &)> &resolve);
    const ColorKeyAsset &at(ColorKeySystem system, unsigned style) const;
    const std::vector<ColorKeyAsset> &volumes() const { return volumes_; }

  private:
    std::vector<ColorKeyAsset> volumes_;
};
} // namespace civ5::materials
