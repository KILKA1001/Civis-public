#include "civ5/ui/ui_texture_asset.hpp"
#include "civ5/assets/ui_texture_binding.hpp"
#include "civ5/runtime/file_identity.hpp"

namespace civ5::ui {
UiTextureAsset load_ui_texture_asset(const assets::ResourceMountTable &resources,
                                    std::string_view requested_key) {
    constexpr std::string_view prefix="indexed:";
    const bool required=requested_key.starts_with(prefix);
    const auto name=required?requested_key.substr(prefix.size()):requested_key;
    if(name.empty())throw std::invalid_argument("UI texture key is empty");
    auto binding=assets::bind_ui_texture(resources,name,required);
    UiTextureAssetSource source{binding.source_identity,binding.source_mount_id,
        runtime::identify_bytes(binding.source.original_bytes()).sha256};
    std::optional<UiTextureAssetSource> index;
    if(binding.index)index=UiTextureAssetSource{*binding.index_identity,binding.index_mount_id,
        runtime::identify_bytes(binding.index->original_bytes()).sha256};
    return {std::move(binding.source),std::move(binding.expanded),std::move(source),std::move(index)};
}
} // namespace civ5::ui
