#include "civ5/assets/dds.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace civ5::assets {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
std::uint32_t u32(const Bytes &b, std::size_t offset) {
    std::uint32_t result = 0;
    for (unsigned i = 0; i < 4; ++i)
        result |= std::uint32_t{b.at(offset + i)} << (i * 8);
    return result;
}
constexpr std::uint32_t cc(char a, char b, char c, char d) {
    return std::uint32_t(a) | (std::uint32_t(b) << 8) | (std::uint32_t(c) << 16) | (std::uint32_t(d) << 24);
}
} // namespace
DdsData DdsData::parse(Bytes bytes, NumericDdsProfile numeric, DdsHeaderProfile header) {
    require(bytes.size() >= 128 && u32(bytes, 0) == cc('D', 'D', 'S', ' '), "DDS header missing/truncated");
    require((u32(bytes, 4) == 124 || (header == DdsHeaderProfile::civ5_d3dx_size24 && u32(bytes, 4) == 24)) &&
                u32(bytes, 76) == 32,
            "Unsupported DDS header sizes");
    DdsData data;
    data.width_ = u32(bytes, 16);
    data.height_ = u32(bytes, 12);
    require(data.width_ >= 1 && data.width_ <= 4096 && data.height_ >= 1 && data.height_ <= 4096,
            "DDS dimensions exceed recovered profile");
    const auto caps2 = u32(bytes, 112), flags = u32(bytes, 8), pf = u32(bytes, 80);
    require((caps2 & 0x200000) == 0 && u32(bytes, 24) <= 1, "Volume DDS needs a separate layout");
    const bool cube = (caps2 & 0xfe00) != 0;
    require(!cube || ((caps2 & 0xfe00) == 0xfe00 && data.width_ == data.height_),
            "Complete square six-face DDS cube required");
    data.faces_ = cube ? 6 : 1;
    data.mips_ = (flags & 0x20000) != 0 ? u32(bytes, 28) : 1;
    require(data.mips_ >= 1 &&
                data.mips_ <= static_cast<std::uint32_t>(std::bit_width(std::max(data.width_, data.height_))),
            "Invalid authored DDS mip count");
    data.fourcc_ = u32(bytes, 84);
    data.bpp_ = u32(bytes, 88);
    for (std::size_t i = 0; i < 4; ++i)
        data.masks_[i] = u32(bytes, 92 + i * 4);
    std::uint32_t block_bytes = 0;
    if (pf & 4) {
        switch (data.fourcc_) {
        case cc('D', 'X', 'T', '1'):
            data.format_ = DdsFormat::dxt1;
            block_bytes = 8;
            break;
        case cc('D', 'X', 'T', '3'):
            data.format_ = DdsFormat::dxt3;
            block_bytes = 16;
            break;
        case cc('D', 'X', 'T', '5'):
            data.format_ = DdsFormat::dxt5;
            block_bytes = 16;
            break;
        default:
            require(numeric == NumericDdsProfile::legacy_d3d9,
                    "Unknown FourCC or unselected numeric DDS profile");
            // Microsoft legacy D3DFORMAT mappings. In particular 36 is four
            // U16 UNORM channels, NOT two half floats (that format is 112).
            switch (data.fourcc_) {
            case 36:
                data.format_ = DdsFormat::rgba16_unorm;
                data.bpp_ = 64;
                break;
            case 110:
                data.format_ = DdsFormat::rgba16_snorm;
                data.bpp_ = 64;
                break;
            case 111:
                data.format_ = DdsFormat::r16_float;
                data.bpp_ = 16;
                break;
            case 112:
                data.format_ = DdsFormat::rg16_float;
                data.bpp_ = 32;
                break;
            case 113:
                data.format_ = DdsFormat::rgba16_float;
                data.bpp_ = 64;
                break;
            case 114:
                data.format_ = DdsFormat::r32_float;
                data.bpp_ = 32;
                break;
            case 115:
                data.format_ = DdsFormat::rg32_float;
                data.bpp_ = 64;
                break;
            case 116:
                data.format_ = DdsFormat::rgba32_float;
                data.bpp_ = 128;
                break;
            default:
                throw std::invalid_argument("Unsupported numeric/DX10 DDS storage");
            }
        }
    } else {
        require(data.fourcc_ == 0, "Non-FourCC DDS has a nonzero format code");
        if (pf == 2 && data.bpp_ == 8 && data.masks_ == std::array<std::uint32_t, 4>{0, 0, 0, 255})
            data.format_ = DdsFormat::alpha8;
        else if ((pf & 0x40) != 0 && (data.bpp_ == 16 || data.bpp_ == 24 || data.bpp_ == 32))
            data.format_ = DdsFormat::masked_rgb;
        else if ((pf & 0x20000) != 0 && (data.bpp_ == 8 || data.bpp_ == 16))
            data.format_ = DdsFormat::luminance;
        else
            throw std::invalid_argument("Unsupported DDS pixel storage");
        // Preserve authored masks; no RGB/alpha reinterpretation in raw layout.
        const auto allowed = data.bpp_ == 32 ? 0xffffffffu : (1u << data.bpp_) - 1;
        for (auto mask : data.masks_)
            require((mask & ~allowed) == 0, "DDS mask exceeds declared pixel storage");
    }
    if (block_bytes)
        data.bpp_ = 0;
    else if (flags & 8)
        require(u32(bytes, 20) == data.width_ * (data.bpp_ / 8),
                "Padded DDS rows need a separately proven layout");
    std::uint64_t offset = 128;
    for (std::uint32_t face = 0; face < data.faces_; ++face) {
        auto width = data.width_, height = data.height_;
        for (std::uint32_t mip = 0; mip < data.mips_; ++mip) {
            const std::uint64_t row = block_bytes ? ((width + 3) / 4) * block_bytes : width * (data.bpp_ / 8);
            const std::uint64_t length = row * (block_bytes ? (height + 3) / 4 : height);
            require(offset <= bytes.size() && length <= bytes.size() - offset,
                    "Truncated DDS face/mip payload");
            data.levels_.push_back({face, mip, width, height, offset, length, row});
            offset += length;
            width = std::max(1u, width / 2);
            height = std::max(1u, height / 2);
        }
    }
    require(offset == bytes.size(), "Unaccounted DDS payload; no implicit truncation or guessed layout");
    data.bytes_ = std::move(bytes);
    return data;
}
std::span<const std::uint8_t> DdsData::payload(std::uint32_t face, std::uint32_t mip) const {
    if (face >= faces_ || mip >= mips_)
        throw std::out_of_range("DDS face/mip index");
    const auto &level = levels_[face * mips_ + mip];
    return std::span{bytes_}.subspan(static_cast<std::size_t>(level.offset),
                                     static_cast<std::size_t>(level.length));
}
} // namespace civ5::assets
