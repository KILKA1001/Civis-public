#pragma once
#include "civ5/graphics/gl_texture.hpp"
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace civ5::graphics::detail {
// Private source proposal. Resource selection, pairing, provenance and command
// lifetime belong to the renderer's actual issuer. This class grants no draw
// authority. Pass the selected UiTextureBinding::index, never its expanded image.
// Native 8e8970 maps DDS L16 (81) to A8L8 (51) WITHOUT changing its payload.
// The low byte is luminance (RRR); the high byte is alpha. GL_R16 is unsuitable.
class NotificationNativeUiIndexSupplier {
  public:
    static NotificationNativeUiIndexSupplier prepare(const assets::DdsData &index) {
        using Mask = std::array<std::uint32_t, 4>;
        const auto &bytes = index.original_bytes();
        const auto u32 = [&](std::size_t offset) {
            if (offset > bytes.size() || bytes.size() - offset < 4)
                throw std::invalid_argument("Truncated native UI index header");
            std::uint32_t value = 0;
            for (unsigned i = 0; i < 4; ++i)
                value |= std::uint32_t(bytes[offset + i]) << (i * 8);
            return value;
        };
        if (index.face_count() != 1 || u32(24) > 1 || u32(112) != 0 ||
            index.format() != assets::DdsFormat::luminance || index.fourcc() != 0)
            throw std::invalid_argument("Native UI index needs a supported 2D luminance DDS");
        const auto masks = index.channel_masks();
        const bool l8 = u32(80) == 0x20000 && index.bits_per_pixel() == 8 &&
                        masks == Mask{255, 0, 0, 0};
        const bool l16 = u32(80) == 0x20000 && index.bits_per_pixel() == 16 &&
                         masks == Mask{65535, 0, 0, 0};
        const bool a8l8 = u32(80) == 0x20001 && index.bits_per_pixel() == 16 &&
                          masks == Mask{255, 0, 0, 0xff00};
        if (!l8 && !l16 && !a8l8)
            throw std::invalid_argument("UI index pixel flags/masks lack the native table match");

        NotificationNativeUiIndexSupplier result;
        result.native_format_ = l8 ? 50u : 51u;
        // 8e8280 returns TOTAL native bits per texel, not luminance channel bits.
        // 6da933..949 and 6daa30..46 write this value to c2.x and c6.x.
        const unsigned native_bits = l8 ? 8u : 16u;
        result.index_scale_x_ = static_cast<float>((1u << native_bits) - 1u);
        result.levels_.reserve(index.mip_count());
        const std::uint64_t bytes_per_texel = l8 ? 1u : 2u;
        if (index.levels().size() != index.mip_count())
            throw std::invalid_argument("UI index authored mip count mismatch");
        for (std::uint32_t mip = 0; mip < index.mip_count(); ++mip) {
            const auto &level = index.levels()[mip];
            const auto payload = index.payload(0, mip);
            const std::uint64_t count = std::uint64_t(level.width) * level.height;
            if (level.face != 0 || level.mip != mip ||
                level.row_pitch != std::uint64_t(level.width) * bytes_per_texel ||
                payload.size() != count * bytes_per_texel ||
                count > std::numeric_limits<std::size_t>::max() / 4)
                throw std::invalid_argument("UI index payload needs exact authored tight rows");
            assets::Rgba8Level pixels{level.width, level.height,
                                     assets::Bytes(static_cast<std::size_t>(count * 4))};
            for (std::size_t texel = 0; texel < static_cast<std::size_t>(count); ++texel) {
                const auto low = payload[texel * static_cast<std::size_t>(bytes_per_texel)];
                const auto high = l8 ? std::uint8_t{255} : payload[texel * 2 + 1];
                pixels.rgba[texel * 4] = low;
                pixels.rgba[texel * 4 + 1] = low;
                pixels.rgba[texel * 4 + 2] = low;
                pixels.rgba[texel * 4 + 3] = high;
            }
            result.levels_.push_back(std::move(pixels));
        }
        return result;
    }

    std::uint32_t native_format() const noexcept { return native_format_; }
    float index_scale_x() const noexcept { return index_scale_x_; }
    const std::vector<assets::Rgba8Level> &levels() const noexcept { return levels_; }
    // Same physical resolution and every authored mip. Only channel storage is
    // materialized in memory; no tile expansion, DDS rewrite, asset file copy,
    // sRGB conversion, generated levels or sampler/LOD policy is introduced.
    // RGBA8 UNORM makes precisely the native L8/A8L8 sampled components explicit.
    GlTexture upload(const GlContext &context) const {
        return GlTexture::rgba8(context, levels_, DdsSamplingColor::linear);
    }

  private:
    NotificationNativeUiIndexSupplier() = default;
    std::uint32_t native_format_ = 0;
    float index_scale_x_ = 0;
    std::vector<assets::Rgba8Level> levels_;
};
} // namespace civ5::graphics::detail
