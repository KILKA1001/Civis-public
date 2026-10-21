#include "civ5/materials/color_keys.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::materials {
const std::array<ColorKeyRequest, 25> &color_key_requests() {
    static const auto result = [] {
        const std::array<const char *, 5> systems{"Terrain", "TerrainFOW", "TerrainShadow", "Buildings",
                                                  "Rivers"};
        const std::array<const char *, 5> styles{"Generic", "America", "Asia", "Africa", "Europe"};
        std::array<ColorKeyRequest, 25> values;
        for (unsigned system = 0; system < 5; ++system)
            for (unsigned style = 0; style < 5; ++style)
                values[system * 5 + style] = {static_cast<ColorKeySystem>(system), style,
                                              std::string("assets\\ColorKeys\\") + systems[system] + "_" +
                                                  styles[style] + "_ColorKey.tga"};
        return values;
    }();
    return result;
}
ColorKeyVolume ColorKeyVolume::identity() {
    assets::Bytes bytes(byte_count);
    std::array<std::uint8_t, size> ramp;
    for (unsigned i = 0; i < size; ++i) {
        const float coordinate = (static_cast<float>(i) + .5f) / static_cast<float>(size);
        ramp[i] = static_cast<std::uint8_t>(std::min(255.0, static_cast<double>(coordinate) * 255.1));
    }
    for (unsigned z = 0; z < size; ++z)
        for (unsigned y = 0; y < size; ++y)
            for (unsigned x = 0; x < size; ++x) {
                const auto at = ((z * size + y) * size + x) * 4;
                bytes[at] = ramp[z];
                bytes[at + 1] = ramp[y];
                bytes[at + 2] = ramp[x];
                bytes[at + 3] = 255;
            }
    return ColorKeyVolume(std::move(bytes));
}
ColorKeyVolume ColorKeyVolume::from_tga(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 18 || bytes[0] != 0 || bytes[1] != 0 || bytes[2] != 2 || bytes[16] != 32 ||
        (bytes[17] != 8 && bytes[17] != 0x28) ||
        std::any_of(bytes.begin() + 3, bytes.begin() + 8, [](auto b) { return b != 0; }))
        throw std::invalid_argument("ColorKey requires uncompressed BGRA32 TGA without ID or palette");
    const auto width = std::uint32_t(bytes[12]) | std::uint32_t(bytes[13]) << 8;
    const auto height = std::uint32_t(bytes[14]) | std::uint32_t(bytes[15]) << 8;
    const auto row_bytes = std::size_t(width) * 4;
    const auto atlas_bytes = std::uint64_t(width) * height * 4;
    if (!width || !height || atlas_bytes < byte_count || atlas_bytes > bytes.size() - 18)
        throw std::invalid_argument("ColorKey TGA cannot supply the complete original volume");
    assets::Bytes result(byte_count);
    for (std::size_t offset = 0; offset < byte_count;) {
        const auto row = offset / row_bytes;
        const auto source_row = bytes[17] == 8 ? height - 1 - row : row;
        const auto count = std::min(row_bytes, byte_count - offset);
        std::copy_n(bytes.begin() + 18 + source_row * row_bytes, count, result.begin() + offset);
        offset += count;
    }
    return ColorKeyVolume(std::move(result));
}
ColorKeyAssets ColorKeyAssets::load(
    const std::function<std::optional<assets::AssetHandle>(const ColorKeyRequest &)> &resolve) {
    if (!resolve)
        throw std::invalid_argument("ColorKey asset resolver is required");
    ColorKeyAssets result;
    result.volumes_.reserve(25);
    for (const auto &request : color_key_requests()) {
        auto asset = resolve(request);
        const auto raw = asset ? asset->read_bytes() : assets::Bytes{};
        const bool authored = !raw.empty();
        auto volume = authored ? ColorKeyVolume::from_tga(raw) : ColorKeyVolume::identity();
        result.volumes_.push_back({request, std::move(asset), authored, std::move(volume)});
    }
    return result;
}
const ColorKeyAsset &ColorKeyAssets::at(ColorKeySystem system, unsigned style) const {
    const auto index = static_cast<unsigned>(system);
    if (index >= 5 || style >= 5)
        throw std::out_of_range("ColorKey system and style must be 0..4");
    return volumes_.at(index * 5 + style);
}
} // namespace civ5::materials
