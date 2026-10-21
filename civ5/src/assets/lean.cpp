#include "civ5/assets/lean.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::assets {
namespace {
void require(bool ok, const char *reason) {
    if (!ok)
        throw std::invalid_argument(reason);
}
std::uint32_t u32(const Bytes &bytes, std::size_t offset) {
    std::uint32_t result = 0;
    for (unsigned i = 0; i < 4; ++i)
        result |= std::uint32_t{bytes.at(offset + i)} << (8 * i);
    return result;
}
} // namespace
LeanData LeanData::parse(Bytes bytes) {
    require(bytes.size() >= 36 && u32(bytes, 0) == 0x4e41454c, "Missing/truncated LEAN header");
    require(u32(bytes, 20) == 2, "Only recovered LEAN type 2 is supported");
    LeanData result;
    result.gradient_bits_ = u32(bytes, 4);
    result.width_ = u32(bytes, 8);
    result.height_ = u32(bytes, 12);
    result.mips_ = u32(bytes, 16);
    require(result.width_ >= 1 && result.width_ <= 4096 && result.height_ >= 1 && result.height_ <= 4096,
            "LEAN dimensions exceed bounded profile");
    require(result.mips_ >= 1 && result.mips_ <= static_cast<std::uint32_t>(
                                                     std::bit_width(std::max(result.width_, result.height_))),
            "Invalid authored LEAN mip count");
    std::uint64_t offset = 36;
    for (unsigned map = 0; map < 2; ++map) {
        require(u32(bytes, 24 + map * 4) == offset, "Noncontiguous LEAN map needs a separate profile");
        auto width = result.width_, height = result.height_;
        const auto bytes_per_pixel = map == 0 ? 8u : 2u;
        for (std::uint32_t mip = 0; mip < result.mips_; ++mip) {
            const std::uint64_t row = std::uint64_t{width} * bytes_per_pixel, length = row * height;
            require(offset <= bytes.size() && length <= bytes.size() - offset, "Truncated LEAN mip");
            result.levels_[map].push_back({mip, width, height, offset, length, row});
            offset += length;
            width = std::max(1u, width / 2);
            height = std::max(1u, height / 2);
        }
    }
    require(offset == bytes.size(), "Unaccounted LEAN payload");
    // Native 0x962730 clears the descriptor flag, but does not swap type-2
    // pixels. Preserve the source header and every original channel byte.
    result.bytes_ = std::move(bytes);
    return result;
}
const std::vector<LeanLevel> &LeanData::levels(LeanMap map) const {
    const auto index = static_cast<unsigned>(map);
    if (index >= levels_.size())
        throw std::out_of_range("LEAN map index");
    return levels_[index];
}
std::span<const std::uint8_t> LeanData::payload(LeanMap map, std::uint32_t mip) const {
    const auto &level = levels(map).at(mip);
    return std::span{bytes_}.subspan(static_cast<std::size_t>(level.offset),
                                     static_cast<std::size_t>(level.length));
}
} // namespace civ5::assets
