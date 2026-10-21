#pragma once
#include "asset_source.hpp"
#include <array>

namespace civ5::assets {
// Raw GPU-ready storage. No decompression, resampling, alpha replacement,
// inferred sRGB, preview normalization or generated mip levels.
enum class DdsFormat : std::uint32_t {
    dxt1 = 1,
    dxt3,
    dxt5,
    masked_rgb,
    alpha8,
    luminance,
    rgba16_unorm,
    rgba16_snorm,
    r16_float,
    rg16_float,
    rgba16_float,
    r32_float,
    rg32_float,
    rgba32_float
};
// Numeric FourCC values are ambiguous between API enums in general DDS files.
// Only a caller with known D3D9 provenance may select the legacy D3D9 profile.
enum class NumericDdsProfile { reject, legacy_d3d9 };
// Some owned expansion DDS files store dwSize=24 in a full 128-byte header.
// D3DX accepts them. This explicit profile never changes the original bytes.
enum class DdsHeaderProfile { strict, civ5_d3dx_size24 };
struct DdsLevel {
    std::uint32_t face, mip, width, height;
    std::uint64_t offset, length, row_pitch;
};
class DdsData {
  public:
    static DdsData parse(Bytes bytes, NumericDdsProfile numeric = NumericDdsProfile::reject,
                         DdsHeaderProfile header = DdsHeaderProfile::strict);
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }
    std::uint32_t face_count() const { return faces_; }
    std::uint32_t mip_count() const { return mips_; }
    std::uint32_t bits_per_pixel() const { return bpp_; }
    std::uint32_t fourcc() const { return fourcc_; }
    DdsFormat format() const { return format_; }
    const std::array<std::uint32_t, 4> &channel_masks() const { return masks_; }
    const std::vector<DdsLevel> &levels() const { return levels_; }
    const Bytes &original_bytes() const { return bytes_; }
    // Cube face order: +X,-X,+Y,-Y,+Z,-Z; each face owns its authored mip chain.
    std::span<const std::uint8_t> payload(std::uint32_t face, std::uint32_t mip) const;

  private:
    DdsData() = default;
    Bytes bytes_;
    std::uint32_t width_ = 0, height_ = 0, faces_ = 0, mips_ = 0, bpp_ = 0, fourcc_ = 0;
    DdsFormat format_ = DdsFormat::dxt1;
    std::array<std::uint32_t, 4> masks_{};
    std::vector<DdsLevel> levels_;
};
} // namespace civ5::assets
