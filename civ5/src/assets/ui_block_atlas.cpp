#include "civ5/assets/ui_block_atlas.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace civ5::assets {
namespace {
void require(bool ok, const char *message) {
    if (!ok) throw std::invalid_argument(message);
}
bool tag_at(const Bytes &bytes, std::size_t offset, std::string_view tag) {
    return offset <= bytes.size() && tag.size() <= bytes.size() - offset &&
           std::equal(tag.begin(), tag.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset));
}
} // namespace

bool has_ui_block_atlas_header(const DdsData &data) noexcept {
    return tag_at(data.original_bytes(), 32, "FTXTInterface") &&
           tag_at(data.original_bytes(), 116, "BC");
}

UiTextureStorageProfile ui_texture_storage_profile(const DdsData &data) {
    const auto &bytes=data.original_bytes();
    if(!tag_at(bytes,32,"FTXTInterface"))
        return UiTextureStorageProfile::ordinary_dds;
    if(tag_at(bytes,116,"COLOR"))
        return UiTextureStorageProfile::interface_color;
    if(has_ui_block_atlas_header(data))
        return UiTextureStorageProfile::indexed_interface;
    throw std::invalid_argument("Unsupported FTXTInterface texture storage profile");
}

Rgba8Level expand_ui_block_atlas(const DdsData &atlas, const DdsData &index) {
    const auto &atlas_bytes = atlas.original_bytes();
    const auto &index_bytes = index.original_bytes();
    require(tag_at(atlas_bytes, 32, "FTXTInterface") &&
                tag_at(index_bytes, 32, "FTXTInterface"),
            "UI block atlas requires the authored FTXTInterface header");
    const auto block_size = tag_at(atlas_bytes, 116, "BC001") && tag_at(index_bytes, 116, "BC001")
                                ? 1u
                                : tag_at(atlas_bytes, 116, "BC002") && tag_at(index_bytes, 116, "BC002")
                                      ? 2u
                                : tag_at(atlas_bytes, 116, "BC004") && tag_at(index_bytes, 116, "BC004")
                                      ? 4u
                                      : tag_at(atlas_bytes, 116, "BC008") && tag_at(index_bytes, 116, "BC008")
                                            ? 8u
                                      : tag_at(atlas_bytes, 116, "BC010") && tag_at(index_bytes, 116, "BC010")
                                            ? 16u
                                      : tag_at(atlas_bytes, 116, "BC020") && tag_at(index_bytes, 116, "BC020")
                                            ? 32u
                                      : tag_at(atlas_bytes, 116, "BC040") && tag_at(index_bytes, 116, "BC040")
                                            ? 64u : 0u;
    require(block_size != 0, "UI block atlas/index tile profiles differ or are unsupported");
    require(atlas.face_count() == 1 && index.face_count() == 1 &&
                atlas.mip_count() == 1 && index.mip_count() == 1 &&
                atlas.format() == DdsFormat::masked_rgb && atlas.bits_per_pixel() == 32 &&
                atlas.channel_masks() ==
                    (std::array<std::uint32_t, 4>{0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000}) &&
                index.format() == DdsFormat::luminance &&
                (index.bits_per_pixel() == 8 || index.bits_per_pixel() == 16),
            "Unsupported FTXTInterface atlas/index DDS storage");
    require(atlas.width() % block_size == 0 && atlas.height() % block_size == 0 &&
                std::uint64_t{index.width()} * block_size <= 4096 &&
                std::uint64_t{index.height()} * block_size <= 4096,
            "Invalid FTXTInterface tile geometry");
    const auto columns = atlas.width() / block_size;
    const auto tile_count = std::uint64_t{columns} * (atlas.height() / block_size);
    const auto source = atlas.payload(0, 0);
    const auto indices = index.payload(0, 0);
    const auto index_stride = index.bits_per_pixel() / 8;
    const auto width = index.width() * block_size, height = index.height() * block_size;
    Rgba8Level output{width, height, Bytes(std::uint64_t{width} * height * 4)};
    for (std::uint32_t tile_y = 0; tile_y < index.height(); ++tile_y) {
        for (std::uint32_t tile_x = 0; tile_x < index.width(); ++tile_x) {
            const auto entry = (std::uint64_t{tile_y} * index.width() + tile_x) * index_stride;
            const auto tile = index_stride == 1 ? std::uint32_t{indices[entry]} :
                              std::uint32_t{indices[entry]} | (std::uint32_t{indices[entry + 1]} << 8);
            require(tile < tile_count, "FTXTInterface index refers outside its atlas");
            const auto source_x = (tile % columns) * block_size;
            const auto source_y = (tile / columns) * block_size;
            for (std::uint32_t y = 0; y < block_size; ++y) {
                for (std::uint32_t x = 0; x < block_size; ++x) {
                    const auto src = (std::uint64_t{source_y + y} * atlas.width() + source_x + x) * 4;
                    const auto dst = (std::uint64_t{tile_y * block_size + y} * width + tile_x * block_size + x) * 4;
                    output.rgba[dst + 0] = source[src + 2];
                    output.rgba[dst + 1] = source[src + 1];
                    output.rgba[dst + 2] = source[src + 0];
                    output.rgba[dst + 3] = source[src + 3];
                }
            }
        }
    }
    return output;
}
} // namespace civ5::assets
