#pragma once
#include "civ5/assets/dds.hpp"
#include "civ5/assets/rgba8.hpp"
#include "civ5/assets/resource_mounts.hpp"

namespace civ5::ui {
struct UiTextureAssetSource {
    assets::AssetCandidate candidate;
    std::optional<std::uint64_t> mount_id;
    std::string sha256;
};
struct UiTextureAsset {
    // Ordinary DDS keeps authored storage/mips; FTXTInterface BC00n keeps the
    // original DDS as evidence and expands its paired logical pixels exactly.
    assets::DdsData dds;
    std::optional<assets::Rgba8Level> expanded;
    UiTextureAssetSource source;
    std::optional<UiTextureAssetSource> index;
};
UiTextureAsset load_ui_texture_asset(const assets::ResourceMountTable &resources,
                                    std::string_view requested_key);
} // namespace civ5::ui
