#pragma once
#include "asset_source.hpp"
#include <array>
#include <bit>

namespace civ5::assets {
// Recovered LEAN type 2: two separately authored mip chains. Their D3D9
// storage is A16B16G16R16 UNORM and L16 UNORM, respectively.
enum class LeanMap : unsigned { rgba16, luminance16 };
struct LeanLevel {
    std::uint32_t mip, width, height;
    std::uint64_t offset, length, row_pitch;
};
class LeanData {
  public:
    // Bounded contiguous type-2 profile. No conversion, channel swap,
    // inferred sRGB or regenerated mip levels. Other LEAN types fail.
    static LeanData parse(Bytes bytes);
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }
    std::uint32_t mip_count() const { return mips_; }
    std::uint32_t gradient_scale_bits() const { return gradient_bits_; }
    float gradient_scale() const { return std::bit_cast<float>(gradient_bits_); }
    std::uint8_t source_swap_flag() const { return bytes_.at(32); }
    const Bytes &original_bytes() const { return bytes_; }
    const std::vector<LeanLevel> &levels(LeanMap map) const;
    std::span<const std::uint8_t> payload(LeanMap map, std::uint32_t mip) const;

  private:
    LeanData() = default;
    Bytes bytes_;
    std::uint32_t width_ = 0, height_ = 0, mips_ = 0, gradient_bits_ = 0;
    std::array<std::vector<LeanLevel>, 2> levels_;
};
} // namespace civ5::assets
