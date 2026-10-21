#pragma once
#include "civ5/assets/ui_texture_binding.hpp"
#include "civ5/graphics/detail/notification_native_ui_index_supplier.hpp"
#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::graphics::detail {
// Only the actual renderer factory may call this source selector. This is not
// a public resource/clock/geometry capability. No atlas expansion discards mips.
class NotificationRawUiTextureSource final {
    friend class ::civ5::ui::UiScreenGlRenderer;
    assets::UiTextureBinding binding_;
    std::string requested_,index_requested_;
    std::uint32_t block_size_=0;
    std::optional<NotificationNativeUiIndexSupplier> index_supplier_;
    NotificationRawUiTextureSource(assets::UiTextureBinding b,std::string key,
        std::string index_key,std::uint32_t block,
        std::optional<NotificationNativeUiIndexSupplier> supplier)
      :binding_(std::move(b)),requested_(std::move(key)),index_requested_(std::move(index_key)),
       block_size_(block),index_supplier_(std::move(supplier)){}
    static std::uint32_t block_size(const assets::DdsData& d){
        if(assets::ui_texture_storage_profile(d)!=assets::UiTextureStorageProfile::indexed_interface)return 0;
        const auto& b=d.original_bytes();
        if(b.size()<121)throw std::runtime_error("Native UI BC profile truncated");
        unsigned result=0;
        for(std::size_t i=118;i<121;++i){const auto c=b[i];unsigned digit;
            if(c>='0'&&c<='9')digit=c-'0';else if(c>='a'&&c<='f')digit=c-'a'+10;
            else if(c>='A'&&c<='F')digit=c-'A'+10;
            else throw std::runtime_error("Native UI BC three-hex factor unavailable");
            result=result*16+digit;
        }
        if(!result)throw std::runtime_error("Native UI indexed factor zero unsupported");
        return result;
    }
    template<class Boundary>
    static NotificationRawUiTextureSource load(const assets::ResourceMountTable& resources,
        const std::string& key,const Boundary& boundary){
        if(key.empty())throw std::runtime_error("Authored masked resource key absent");
        const auto resolve=[&](const std::string& requested){
            boundary();auto result=resources.resolve(assets::AssetKey(requested));boundary();
            const auto basename=assets::resource_basename(requested);
            if(std::holds_alternative<std::monostate>(result.asset)&&basename!=requested){
                boundary();result=resources.resolve(assets::AssetKey(basename));boundary();
            }
            return result;
        };
        const auto selected=resolve(key);
        const auto* found=std::get_if<assets::AssetHandle>(&selected.asset);
        if(!found)throw std::runtime_error("Authored masked source unavailable or ambiguous: "+key);
        // Handles and immutable candidate values are independent of table and
        // renderer lifetime. FIRST boundary after read precedes parsing/access.
        const auto asset=*found;boundary();auto bytes=asset.read_bytes(256ull*1024*1024);boundary();
        assets::UiTextureBinding binding{asset.candidate(),selected.mount_id,
            assets::DdsData::parse(std::move(bytes)),{},{},{},{}};
        const auto& header=binding.source.original_bytes();
        const auto depth=std::uint32_t(header[24])|(std::uint32_t(header[25])<<8)|
            (std::uint32_t(header[26])<<16)|(std::uint32_t(header[27])<<24);
        if(binding.source.face_count()!=1||depth>1)throw std::runtime_error("Masked source not authored 2D");
        const auto block=block_size(binding.source);std::string index_key;
        std::optional<NotificationNativeUiIndexSupplier> supplier;
        if(block){
            const auto original=binding.source_identity.original_name;
            if(original.size()<4||assets::normalize_asset_name(original.substr(original.size()-4))!=".dds")
                throw std::runtime_error("Masked indexed source not DDS");
            index_key=original.substr(0,original.size()-4)+"-index.dds";
            const auto selected_index=resolve(index_key);
            const auto* hit=std::get_if<assets::AssetHandle>(&selected_index.asset);
            if(!hit)throw std::runtime_error("Masked paired index unavailable or ambiguous: "+index_key);
            const auto index_asset=*hit;
            binding.index_identity=index_asset.candidate();binding.index_mount_id=selected_index.mount_id;
            const auto& a=binding.source_identity.provenance;const auto& b=binding.index_identity->provenance;
            if(binding.source_mount_id!=binding.index_mount_id||a.kind!=b.kind||a.source_id!=b.source_id||
               a.archive_relative_path!=b.archive_relative_path||
               assets::normalize_asset_name(binding.index_identity->original_name)!=assets::normalize_asset_name(index_key))
                throw std::runtime_error("Masked atlas/index actual winner containers differ");
            boundary();auto index_bytes=index_asset.read_bytes(256ull*1024*1024);boundary();
            binding.index.emplace(assets::DdsData::parse(std::move(index_bytes)));
            if(block_size(*binding.index)!=block||binding.source.width()%block||binding.source.height()%block)
                throw std::runtime_error("Masked atlas/index actual tile profiles differ");
            supplier.emplace(NotificationNativeUiIndexSupplier::prepare(*binding.index));
        }
        const auto stable=[&](const std::string& requested,const assets::AssetCandidate& before,
            std::optional<std::uint64_t> mount,const assets::DdsData& dds){
            const auto selected_now=resolve(requested);const auto* hit=std::get_if<assets::AssetHandle>(&selected_now.asset);
            if(!hit)throw std::runtime_error("Masked selected source vanished/ambiguous");
            const auto handle=*hit;const auto& now=handle.candidate();
            if(selected_now.mount_id!=mount||now.original_name!=before.original_name||
               now.normalized_name!=before.normalized_name||now.payload_size!=before.payload_size||
               now.provenance.kind!=before.provenance.kind||now.provenance.source_id!=before.provenance.source_id||
               now.provenance.source_relative_path!=before.provenance.source_relative_path||
               now.provenance.archive_relative_path!=before.provenance.archive_relative_path)
                throw std::runtime_error("Masked selected source identity changed");
            boundary();const auto current_bytes=handle.read_bytes(256ull*1024*1024);boundary();
            if(current_bytes!=dds.original_bytes())throw std::runtime_error("Masked selected source bytes changed");
        };
        stable(key,binding.source_identity,binding.source_mount_id,binding.source);
        if(block)stable(index_key,*binding.index_identity,binding.index_mount_id,*binding.index);
        boundary();return NotificationRawUiTextureSource(std::move(binding),key,std::move(index_key),block,std::move(supplier));
    }
};
}
