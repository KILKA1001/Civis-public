#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/ui/detail/bnw_native_resource_record_index.hpp"
#include <thread>
namespace civ5::assets::detail {
// PRIVATE OWN-port DATA, never a Native namespace/FIRST/App or Lua-list issuer.
// Derived only from the actual closed capture table and its owned sources.
class CivisPortNamespaceCustody final {
    struct Source final {
        std::shared_ptr<const AssetSource> owner;
        std::vector<AssetHandle> loose_admissions;
        std::vector<FpkEntry> archive_admissions;
        std::unique_ptr<ui::detail::NativeResourceRecordIndex> loose_bucket_projection;
        bool loose_projection_supported=false;
        explicit Source(std::shared_ptr<const AssetSource> value):owner(std::move(value)){}
    };
    const std::thread::id creator_=std::this_thread::get_id();
    const std::unique_ptr<const ResourceMountTable> table_;
    std::vector<Source> defaults_,registered_;
    template<class Bookend> static Source capture_source(std::shared_ptr<const AssetSource> source,Bookend& current){
        if(!source)throw std::runtime_error("Port namespace missing actual source custody");
        Source out(std::move(source));
        if(const auto loose=std::dynamic_pointer_cast<const LooseResourceIndex>(out.owner)){
            out.loose_admissions=loose->port_admission_order();current();
            auto index=std::make_unique<ui::detail::NativeResourceRecordIndex>();current();
            // Actual loose add_file replaces earlier basename. For the finite
            // ASCII/no-different-name-CRC domain this agrees with native mode2.
            // The index is diagnostic: no authenticated shared FIRST is bound.
            bool supported=true;
            std::map<std::uint32_t,std::string> crc_names;
            for(const auto& admission:out.loose_admissions){
                const auto name=admission.candidate().original_name;
                const auto key=resource_basename(name);current();
                if(key.empty()||!std::all_of(key.begin(),key.end(),[](unsigned char c){return c>=32&&c<127;})){
                    supported=false;break;
                }
                const auto crc=ui::detail::NativeResourceRecordIndex::source_hash(key);
                const auto found=crc_names.find(crc);
                if(found!=crc_names.end()&&found->second!=key){supported=false;break;}
                crc_names.try_emplace(crc,key);current();
                index->insert(name,admission,2);current();
            }
            if(supported){out.loose_bucket_projection=std::move(index);out.loose_projection_supported=true;}
        }else if(const auto archive=std::dynamic_pointer_cast<const FpkSource>(out.owner)){
            out.archive_admissions=archive->entries();current();
            // Actual archive entry order is retained. FPK ambiguity/replacement
            // modes cannot be converted to native83B130 without its real actor.
        } // Unknown source stays retained; its native projection is unavailable.
        current();return out;
    }
public:
    template<class Bookend> explicit CivisPortNamespaceCustody(ResourceMountTable table,Bookend& current)
        :table_(std::make_unique<const ResourceMountTable>(std::move(table))){
        current();
        defaults_.reserve(table_->port_default_sources().size());current();
        for(const auto& source:table_->port_default_sources()){
            const auto retained=source;auto row=capture_source(retained,current);current();
            defaults_.push_back(std::move(row));current();
        }
        registered_.reserve(table_->mounts().size());current();
        for(const auto& mount:table_->mounts()){
            const auto retained=mount.port_source_owner;auto row=capture_source(retained,current);current();
            registered_.push_back(std::move(row));current();
        }
        // Empty provenance stays unavailable without changing Bank publication.
    }
    template<class Bookend> void require_same_table(const ResourceMountTable& actual,Bookend& current)const{
        current();
        if(creator_!=std::this_thread::get_id()||actual.port_default_enabled()!=table_->port_default_enabled()||
            actual.mounts().size()!=registered_.size()||actual.port_default_sources().size()!=defaults_.size())
            throw std::runtime_error("Port namespace captured table identity changed");
        const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        for(std::size_t n=0;n<defaults_.size();++n)
            if(!same(actual.port_default_sources()[n],defaults_[n].owner))throw std::runtime_error("Port default source/control changed");
        for(std::size_t n=0;n<registered_.size();++n){
            const auto& a=actual.mounts()[n];const auto& b=table_->mounts()[n];
            if(a.id!=b.id||a.group!=b.group||a.priority!=b.priority||a.enabled!=b.enabled||a.preferred!=b.preferred||
                !same(a.port_source_owner,registered_[n].owner))throw std::runtime_error("Port registered sequence/control changed");
        }
        current();
    }
    // Replay the supported port's reverse registration/default bucket DATA.
    // Not wired to IncludeCollection. Every raw admission remains retained;
    // order is NEVER reconstructed from keys/maps or sorted catalog names.
    template<class Bookend,class Visit> void visit_supported_loose_bucket_data(Bookend& current,Visit&& visit)const{
        current();
        const auto emit=[&](const Source& s){
            if(!s.loose_projection_supported||!s.loose_bucket_projection)
                throw std::runtime_error("Native archive/CRC/FIRST namespace bridge required before collector");
            auto order=s.loose_bucket_projection->bucket_order.ordered_nodes();current();
            for(const auto node:order){
                const auto& index=*s.loose_bucket_projection;
                const auto record=index.records.find(node.key);
                const auto name=index.first_record_names.find(node.key);
                if(record==index.records.end()||name==index.first_record_names.end())
                    throw std::runtime_error("Port loose bucket physical record missing");
                visit(node,std::get<AssetHandle>(record->second),name->second);current();
            }
        };
        if(creator_!=std::this_thread::get_id())throw std::runtime_error("Port namespace foreign creator");
        if(!table_->port_default_enabled())return; // Captured real port byte, NOT Native +89 receipt.
        for(std::size_t n=registered_.size();n>0;--n)if(table_->mounts()[n-1].enabled)emit(registered_[n-1]);
        for(const auto& s:defaults_)emit(s);
        // There is deliberately no App container and no successful names return.
    }
};
}
