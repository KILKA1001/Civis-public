#pragma once
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_package_pre_skin_owner.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/ui/detail/bnw_native_resource_bucket_order.hpp"
#include <map>
#include <unordered_map>
#include <algorithm>
#include <type_traits>
#include <utility>
namespace civ5::ui::detail {
class BnwNativeOriginalStatRead;
// Finite successful ASCII port of the actual5F6BB4 pattern literal,
// 81D100 semicolon tokenization and81D1C0 case-insensitive wildcard branch.
// Fixed tokens are "*.xml" and "*.ggxml": leading '*' backtracks until the
// literal suffix consumes the whole filename. Native '. *' empty-tail quirk
// is unreachable for these two nonempty suffixes. This is no owner/issuer.
inline bool native_category_xml_filter_ascii(std::string_view basename) {
    if(basename.empty() || basename.size()>127 ||
       basename.find_first_of("/\\")!=std::string_view::npos ||
       !std::all_of(basename.begin(),basename.end(),[](unsigned char c){return c>=32&&c<127;}))
        throw std::runtime_error("Native category XML matcher requires finite printable ASCII basename/C CRT scope");
    constexpr std::string_view patterns="*.xml;*.ggxml";
    const auto upper=[](unsigned char c){return c>='a'&&c<='z'?static_cast<unsigned char>(c-('a'-'A')):c;};
    std::size_t start=0;
    for(;;) {
        const auto separator=patterns.find(';',start);
        const auto token=patterns.substr(start,separator==std::string_view::npos?patterns.size()-start:separator-start);
        // Exact fixed-star domain of81D1C0; not a general guessed extension API.
        const auto suffix=token.substr(1);
        if(basename.size()>=suffix.size()) {
            const auto at=basename.size()-suffix.size();bool same=true;
            for(std::size_t i=0;i<suffix.size();++i)
                same&=upper(static_cast<unsigned char>(basename[at+i]))==upper(static_cast<unsigned char>(suffix[i]));
            if(same)return true;
        }
        if(separator==std::string_view::npos)return false;
        start=separator+1;
    }
}
constexpr unsigned native_category_matched_filter_mode(unsigned incoming)noexcept {
    return (incoming&0xfffffff2u)|2u; // literal7→2; unrelated mode6 is not this caller.
}

// Internal source algorithm; constructing this index does not authenticate an
// actual package/table epoch or permit advancing startup readiness.
struct UnreadableLoose {
    std::filesystem::path native_path;
    std::string relative;
    std::uint32_t error;
    BnwNativeUnreadableLooseReceipt receipt;
    // Conservative Civis UNKNOWN unchecked-word representation. This genuine
    // failed Original read owns no successful length/time or native words.
    // Legacy known-stat/unavailable and diagnostic aggregate callers leave null.
    std::shared_ptr<const BnwNativeOriginalStatRead> failed_original_stat{};
};
// Default registration is ONE native record set, not a stack of archive mounts.
struct NativeResourceRecordIndex {
    std::unordered_map<std::uint32_t, std::variant<assets::AssetHandle, UnreadableLoose>> records;
    std::map<std::uint32_t, std::string> record_names;
    // Staged projection of the SAME privately retained global FIRST. Pure
    // diagnostic indexes remain local and cannot issue/bind an authentic lineage.
    std::map<std::uint32_t, std::string> first_record_names;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class BnwNativeCategoryNamespaceLineage;
    std::shared_ptr<BnwNativeSharedFirstLineage> source_first_;
    void bind_shared_first(std::shared_ptr<BnwNativeSharedFirstLineage>);
    const std::map<std::uint32_t,std::string>& source_first_names() const;
    // CategorySource's privately constructed Borrow authenticates the SAME
    // namespace, retained parent/lease and original stat/cache sources at each
    // call. Neither public insert nor scalar diagnostics can enter this route.
    // This stages records/FIRST projection; CategorySource must still perform
    // its final shared-FIRST extension and owned publication bookends.
    template<class Borrow> bool insert_mode7(const std::string& name,
        const std::variant<assets::AssetHandle, UnreadableLoose>& incoming, Borrow& owned) {
        record_need(bool(source_first_), "Native mode7 requires genuinely bound shared FIRST");
        const auto current=[&] { owned.require_current(*this,*source_first_); };
        current();
        const auto* loose=std::get_if<assets::AssetHandle>(&incoming);
        record_need(loose && loose->candidate().provenance.kind==assets::SourceKind::loose,
                    "Native mode7 finite operation requires owned readable loose payload");
        const auto key=assets::resource_basename(name);
        const auto hash=source_hash(key);
        record_need(!key.empty() && assets::resource_basename(loose->candidate().original_name)==key,
                    "Native mode7 payload belongs to another incoming basename");
        // The borrow derives the original relative path from its captured
        // incoming handle; no caller stat/path, FILETIME or canonical time.
        const auto stat=owned.original_loose_stat(incoming);current();
        record_need(stat.return_value==0 && stat.size && stat.mtime,
                    "Native mode7 unchecked original stat outputs are unknown");
        record_need(*stat.size>=0 && loose->candidate().payload_size==std::uint64_t(*stat.size),
                    "Native mode7 original signed32 size differs from bounded canonical payload");
        const auto old=records.find(hash);
        bool replace=true;
        if(old!=records.end()) {
            const auto* selected=std::get_if<assets::AssetHandle>(&old->second);
            const bool archived=selected && selected->candidate().provenance.kind==assets::SourceKind::fpk;
            if(archived) {
                const auto cached=owned.cached_archive_mtime(*selected);current();
                record_need(cached.source_cache_present || !cached.original_cached_mtime,
                            "Native absent source cache cannot issue archive time");
                if(!cached.source_cache_present || !cached.original_cached_mtime)replace=true;
                else replace=native_mode7_replaces(true,true,true,std::bit_cast<std::uint64_t>(*stat.mtime),
                    std::bit_cast<std::uint64_t>(*cached.original_cached_mtime));
            }
        }
        // No visible erase or FIRST change precedes the last source check.
        // Allocations and CRC/order/FIRST refusals occur in this staged copy.
        auto staged=*this;
        if(replace) {
            const auto collision_begin=staged.collisions.size();
            record_need(staged.insert(name,incoming,2), "Native mode7 selected insertion failed");
            for(auto n=collision_begin;n<staged.collisions.size();++n)staged.collisions[n].mode=7;
        } else {
            const auto selected_name=record_names.find(hash);
            if(selected_name!=record_names.end() && selected_name->second!=key)
                staged.collisions.push_back({hash,selected_name->second,key,7,false});
        }
        current();
        records.swap(staged.records);record_names.swap(staged.record_names);
        first_record_names.swap(staged.first_record_names);collisions.swap(staged.collisions);
        // NativeResourceBucketOrder owns only value containers; no source callbacks.
        static_assert(std::is_nothrow_swappable_v<NativeResourceBucketOrder>);
        std::swap(bucket_order,staged.bucket_order);
        return replace;
    }

  public:
    std::vector<BnwNativeResourceCollisionReceipt> collisions;
    //Captures admission/replacement order in this SAME staged index. A future
    //private actual namespace bridge must derive enable/globalFIRST/epoch from
    //PackageImpl; the pure order container alone cannot authorize enumeration.
    NativeResourceBucketOrder bucket_order;
    static void record_need(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
    static std::uint32_t source_hash(const std::string& name) {
        const auto key = assets::resource_basename(name);
        record_need(std::all_of(key.begin(), key.end(), [](unsigned char c) { return c < 128; }),
                    "Native non-ASCII record CRC requires CP_ACP/CRT source owner");
        return content::saved_info_type_hash(key);
    }
    bool insert(const std::string& name, std::variant<assets::AssetHandle, UnreadableLoose> asset, unsigned mode) {
        record_need(mode == 0 || mode == 1 || mode == 2 || mode == 4,
             "Native default insertion mode requires a source-owned handler");
        const auto key = assets::resource_basename(name);
        const auto hash = source_hash(key);
        const auto staged_first = first_record_names.find(hash);
        const std::string* first_name = staged_first==first_record_names.end() ? nullptr : &staged_first->second;
        if(source_first_) {
            const auto& inherited=source_first_names();
            const auto actual=inherited.find(hash);
            if(actual!=inherited.end()) {
                record_need(!first_name || *first_name==actual->second,"Staged native FIRST conflicts with shared source");
                first_name=&actual->second;
            }
        }
        const auto slash = name.find_last_of("/\\");
        const auto raw_basename = slash == std::string::npos ? name : name.substr(slash + 1);
        // Native83B2D0/849F00 compares the CRC DWORD only. A different
        // basename with the same CRC is the SAME lookup record. Mode handling
        // runs before83B3F0 global debug-name caching, not a string comparator.
        const auto old = records.find(hash);
        const auto old_name = record_names.find(hash);
        const bool collision = old_name != record_names.end() && old_name->second != key;
        if (old != records.end()) {
            const auto* handle = std::get_if<assets::AssetHandle>(&old->second);
            const bool archived = handle && handle->candidate().provenance.kind == assets::SourceKind::fpk;
            // Source83B130: 0 preserves existing;1 replaces only loose;
            // 2 replaces either;4 replaces only archive. Mode6 timestamps are
            // outside this initial startup sequence and cannot be guessed.
            const bool replace = mode == 2 || (mode == 1 && !archived) || (mode == 4 && archived);
            const auto* incoming_handle = std::get_if<assets::AssetHandle>(&asset);
            const bool incoming_archive = incoming_handle &&
                incoming_handle->candidate().provenance.kind == assets::SourceKind::fpk;
            //83B3F0 keeps the first GLOBAL basename for a CRC.839FE0 uses it
            // for loose filename reconstruction. A new different-name loose
            // payload cannot use its incoming handle as native read authority.
            // Comparing the current selected record name is insufficient:
            // archiveA -> archiveB -> looseB must still reconstruct nameA.
            const bool differs_from_first = first_name &&
                assets::resource_basename(*first_name) != key;
            record_need(!differs_from_first || !replace || incoming_archive,
                "Native different-name CRC loose replacement needs global filename cache/read owner");
            if (collision) collisions.push_back({hash, old_name->second, key, mode, replace});
            if (!replace) return false;
        }
        // With a shared FIRST, another namespace may own this CRC's first
        // basename even when this namespace has no old record. Native loose
        // reads reconstruct that FIRST name, so an incoming different-name
        // handle cannot authenticate the payload. Refuse before mutation.
        const auto* incoming_handle = std::get_if<assets::AssetHandle>(&asset);
        const bool incoming_archive = incoming_handle &&
            incoming_handle->candidate().provenance.kind == assets::SourceKind::fpk;
        record_need(!first_name || assets::resource_basename(*first_name)==key || incoming_archive,
                    "Native cross-namespace CRC loose admission needs authentic global FIRST read owner");
        //83B195 removes the selected hash record before insertion; update
        // record payload and enumerated name together in this staged copy.
        if (old != records.end()) {
            record_need(bucket_order.live_nodes() == records.size(),
                        "Native selected CRC/order state differs before replacement");
            bucket_order.erase_first(hash);
        }
        record_need(bucket_order.insert(hash, hash) == 1,
                    "Native selected record insertion unexpectedly retained duplicate CRC");
        first_record_names.try_emplace(hash, first_name ? *first_name : raw_basename);
        records.insert_or_assign(hash, std::move(asset));
        record_names.insert_or_assign(hash, key);
        return true;
    }
    assets::ResourceMountHit lookup(const assets::AssetKey& key) const;
    std::vector<std::string> keys(std::string_view prefix) const {
        std::vector<std::string> result;
        const auto normalized = assets::normalize_asset_name(std::string(prefix));
        for (const auto& [name, asset] : records) {
            const auto* handle = std::get_if<assets::AssetHandle>(&asset);
            const auto original_name = handle ? handle->candidate().original_name : std::get<UnreadableLoose>(asset).relative;
            const auto selected = assets::normalize_asset_name(original_name);
            if (normalized.empty() || selected == normalized || selected.starts_with(normalized + '/'))
                result.push_back(original_name);
        }
        std::sort(result.begin(), result.end());
        return result;
    }
};
}
