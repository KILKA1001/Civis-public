#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/assets/ui_block_atlas.hpp"
#include <stdexcept>

namespace civ5::assets {
// Retain selected source bytes and provenance alongside the logical image.
// Packed atlas dimensions are not final UI dimensions. Native layout/UV cache
// ownership remains the caller's responsibility.
struct UiTextureBinding {
    AssetCandidate source_identity;
    std::optional<std::uint64_t> source_mount_id;
    DdsData source;
    std::optional<AssetCandidate> index_identity;
    std::optional<std::uint64_t> index_mount_id;
    std::optional<DdsData> index;
    std::optional<Rgba8Level> expanded;
    std::uint32_t width() const noexcept {
        return expanded ? expanded->width : source.width();
    }
    std::uint32_t height() const noexcept {
        return expanded ? expanded->height : source.height();
    }
};

inline UiTextureBinding bind_ui_texture(const ResourceMountTable &resources,
                                       std::string_view name,
                                       bool require_indexed = false) {
    const std::string key(name);
    const auto resolve = [&](const std::string &requested) {
        const auto basename = resource_basename(requested);
        auto result = resources.resolve(AssetKey(requested));
        // Preserve an authored mounted key before the compatibility basename
        // lookup for basename-only archives. Never bypass an ambiguous key.
        // This mount policy is not a proof of all native VFS precedence.
        if (std::holds_alternative<std::monostate>(result.asset)) {
            if (basename != requested)
                result = resources.resolve(AssetKey(basename));
        }
        return result;
    };
    const auto selected = resolve(key);
    const auto *asset = std::get_if<AssetHandle>(&selected.asset);
    if (!asset)
        throw std::runtime_error("Active UI DDS is unavailable or ambiguous: " + key);
    UiTextureBinding binding{asset->candidate(), selected.mount_id,
                            DdsData::parse(asset->read_bytes(256ull*1024*1024)), {}, {}, {}, {}};
    const auto profile=ui_texture_storage_profile(binding.source);
    if (require_indexed && profile!=UiTextureStorageProfile::indexed_interface)
        throw std::runtime_error("Explicit indexed UI key does not name authored block storage");
    if (!require_indexed && profile!=UiTextureStorageProfile::indexed_interface)
        return binding;
    const auto original=binding.source_identity.original_name;
    if (original.size() < 4 || normalize_asset_name(original.substr(original.size()-4)) != ".dds")
        throw std::runtime_error("Indexed UI source is not a DDS: " + key);
    const auto index_name = original.substr(0, original.size()-4) + "-index.dds";
    const auto selected_index = resolve(index_name);
    const auto *index_asset = std::get_if<AssetHandle>(&selected_index.asset);
    if (!index_asset)
        throw std::runtime_error("Paired UI index DDS is unavailable or ambiguous: " + index_name);
    binding.index_identity = index_asset->candidate();
    binding.index_mount_id = selected_index.mount_id;
    const auto &atlas_source=binding.source_identity.provenance;
    const auto &index_source=binding.index_identity->provenance;
    if(binding.source_mount_id!=binding.index_mount_id ||
       atlas_source.kind!=index_source.kind || atlas_source.source_id!=index_source.source_id ||
       atlas_source.archive_relative_path!=index_source.archive_relative_path)
        throw std::runtime_error("UI block atlas and index selected from different source mounts/containers");
    if(normalize_asset_name(binding.index_identity->original_name)!=normalize_asset_name(index_name))
        throw std::runtime_error("UI block index does not pair with selected atlas identity");
    binding.index.emplace(DdsData::parse(index_asset->read_bytes(256ull*1024*1024)));
    binding.expanded.emplace(expand_ui_block_atlas(binding.source, *binding.index));
    const auto stable=[&](const std::string &requested,const AssetCandidate &before,
                          std::optional<std::uint64_t> mount,const DdsData &dds) {
        const auto current=resolve(requested);
        const auto *handle=std::get_if<AssetHandle>(&current.asset);
        if(!handle) throw std::runtime_error("UI texture source vanished or became ambiguous during paired load");
        const auto &now=handle->candidate();
        if(current.mount_id!=mount || now.original_name!=before.original_name ||
           now.normalized_name!=before.normalized_name || now.payload_size!=before.payload_size ||
           now.provenance.kind!=before.provenance.kind ||
           now.provenance.source_id!=before.provenance.source_id ||
           now.provenance.source_relative_path!=before.provenance.source_relative_path ||
           now.provenance.archive_relative_path!=before.provenance.archive_relative_path ||
           handle->read_bytes(256ull*1024*1024)!=dds.original_bytes())
            throw std::runtime_error("UI texture selected source changed during paired load");
    };
    stable(key,binding.source_identity,binding.source_mount_id,binding.source);
    stable(index_name,*binding.index_identity,binding.index_mount_id,*binding.index);
    return binding;
}
} // namespace civ5::assets
