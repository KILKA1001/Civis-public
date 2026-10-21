#pragma once
namespace civ5::ui::detail {class BnwNativeCommonSourceValidationContext;}
#include "bnw_native_archive_registry_source.hpp"
#include "bnw_native_shared_first_lineage.hpp"
#include "civ5/ui/detail/bnw_native_resource_bucket_order.hpp"
#include "civ5/runtime/crc32_byte.hpp"
#include <map>
#include <optional>
#include <string_view>
#include <utility>
namespace civ5::ui::detail {class BnwNativeCommon13ArchiveChannel;struct BnwNativeCommon13Step;}
namespace civ5::ui::detail {
// Pure source formulas/storage policy only. No diagnostic scalar/map can be
// injected into a genuine private arena/receipt or grant a cache stage.
inline std::uint32_t bnw_native_admission_ascii_crc(std::string_view raw) {
    if(raw.empty() || raw.size()>127)throw std::runtime_error("Native entry requires finite nonempty 128-byte C buffer scope");
    std::uint32_t crc=0xffffffffu;
    for(const char byte:raw) {
        auto c=static_cast<unsigned char>(byte);
        if(c<32 || c>126)throw std::runtime_error("Native entry requires printable ASCII/C CRT lower scope");
        if(c>='A'&&c<='Z')c=static_cast<unsigned char>(c+('a'-'A'));
        crc=runtime::crc32_update_byte(crc,c);
    }
    return crc; //83B4E0 has NO final complement or basename/slash normalization.
}
struct BnwNativeArchiveRecordDiagnostic {
    std::uint32_t crc=0,length=0,offset=0,packed_prefix=0,symbolic_slot=0;
    bool operator==(const BnwNativeArchiveRecordDiagnostic&)const=default;
};
class BnwNativeArchiveEntryKernelDiagnostic final {
    std::map<std::uint32_t,BnwNativeArchiveRecordDiagnostic> records_;
    std::map<std::uint32_t,std::string> first_;
    std::shared_ptr<BnwNativeSharedFirstLineage> source_first_;
    friend class BnwNativeArchiveEntryAdmissionArena;
    void bind_shared_first(std::shared_ptr<BnwNativeSharedFirstLineage> source) {
        if(!source || source_first_ || !records_.empty() || !first_.empty())
            throw std::runtime_error("Native arena shared FIRST binding late/repeated");
        source->require_current(); source_first_=std::move(source);
    }
    std::uint32_t admit_loose(std::string_view raw,std::uint32_t crc,std::uint32_t prefix,std::uint32_t length) {
        if(!source_first_ || prefix>=4096 || raw.empty() || raw.size()>259)
            throw std::runtime_error("Native loose store lacks SAME FIRST/prefix/FindA basename");
        auto old=records_.find(crc);std::uint32_t slot=0;
        if(old!=records_.end()){slot=old->second.symbolic_slot;buckets_.erase_first(crc);records_.erase(old);}
        if(!slot && allocated_>=NativeResourceBucketOrder::source_initial_pool)
            throw std::runtime_error("Native loose initial pool exhausted; native heap/growth not owned");
        source_first_->remember(crc,raw); // AFTER actual replacement decision; SAME83B3F0.
        if(!slot){slot=NativeResourceBucketOrder::source_initial_pool-allocated_;++allocated_;}
        records_.emplace(crc,BnwNativeArchiveRecordDiagnostic{crc,length,0,prefix<<20,slot});
        if(buckets_.insert(crc,slot)!=1)throw std::runtime_error("Native loose duplicate selected node");
        return 1;
    }
    NativeResourceBucketOrder buckets_;
    std::uint32_t allocated_=0;
  public:
    //Genuine private primary/Common callers use mode0; the actual Category
    //entry-store producer uses mode2 under its source/channel/fence. Public
    //diagnostic use alone cannot issue either live owner. Other modes refuse.
    std::uint32_t admit(std::string_view raw,std::uint32_t prefix,std::uint32_t length,
                        std::uint32_t offset,unsigned diagnostic_mode=0) {
        if(prefix>=4096 || (diagnostic_mode!=0&&diagnostic_mode!=2))
            throw std::runtime_error("Native finite prefix/mode branch unsupported");
        const auto crc=bnw_native_admission_ascii_crc(raw);
        auto old=records_.find(crc);
        if(old!=records_.end()&&diagnostic_mode==0)return 0; //83B130 preserves BEFORE FIRST.
        std::uint32_t slot=0;
        if(old!=records_.end()) {slot=old->second.symbolic_slot;buckets_.erase_first(crc);records_.erase(old);}
        //83B3F0 preserves FIRST raw string across replacement/colliding names.
        //Successful storage distinguishes legitimate CRC0 from alloc-failure0.
        if(!slot && allocated_>=NativeResourceBucketOrder::source_initial_pool)
            throw std::runtime_error("Native finite initial record pool exhausted; heap/growth not owned");
        if(source_first_) source_first_->remember(crc,raw);
        else first_.try_emplace(crc,raw); // Pure diagnostic storage only.
        if(!slot) {
            slot=NativeResourceBucketOrder::source_initial_pool-allocated_;
            ++allocated_;
        }
        BnwNativeArchiveRecordDiagnostic record{crc,length,offset,prefix<<20,slot};
        records_.emplace(crc,record);
        if(buckets_.insert(crc,slot)!=1)throw std::runtime_error("Native selected admission retained duplicate namespace node");
        return 1;
    }
    const auto& records()const{return records_;}
    const auto& first_names()const{return source_first_ ? source_first_->names() : first_;}
    const NativeResourceBucketOrder& bucket_order()const{return buckets_;}
};
// Finite native83C9 read-layout equivalence from privately prepared parser
// snapshot. This validates INPUTS, never substitutes declared count for returns.
inline void bnw_native_v6_entry_inputs(const std::vector<assets::FpkEntry>& entries,
                                      std::uint32_t declared,std::uint64_t directory_end,
                                      std::uint64_t bytes) {
    if(declared>1000000 || entries.size()!=declared)throw std::runtime_error("Native finite directory declared/read count differs");
    std::uint64_t cursor=14;
    for(std::size_t n=0;n<entries.size();++n) {
        const auto& e=entries[n];(void)bnw_native_admission_ascii_crc(e.original_name);
        const auto length=e.original_name.size();const auto padding=static_cast<std::uint32_t>((4-(length&3))&3);
        if(e.original_name.find_first_of("/\\:")!=std::string::npos || e.ordinal!=n ||
           e.directory_record_offset!=cursor || e.padding_length!=padding ||
           e.metadata_offset!=cursor+4+length+padding || e.metadata_offset>bytes ||
           bytes-e.metadata_offset<16 || e.payload_offset<=12 || e.payload_offset>UINT32_MAX ||
           e.payload_length>UINT32_MAX || e.payload_offset>=bytes || e.payload_length>=bytes ||
           e.payload_length>bytes-e.payload_offset)
            throw std::runtime_error("FPK parser compatibility layout differs from finite native entry read branch");
        cursor=e.metadata_offset+16;
    }
    if(cursor!=directory_end || cursor>bytes)throw std::runtime_error("Native directory cursor differs from parser snapshot");
}
class BnwNativeCategoryLooseAdmissionSource;
class BnwNativeCategoryLooseAdmissionReceipt;
class BnwNativeCurrentDefaultSource;
class BnwNativeInstalledLateDefaultOperation;
class BnwNativeInstalledLateDefaultArchiveAttempt;
class BnwNativeArchiveV6EntryAdmissionOwner;
class BnwNativeArchiveBasenameCacheOwner;
struct BnwNativeDefaultNamespaceInitialReceiptDiagnostic;
class BnwNativePrimary32RecordSourceBindings;
// ONE fresh shared global FIRST/default namespace, privately constructed from
// genuine SAME cold before headers. Existing diagnostic index cannot bind it.
class BnwNativeArchiveEntryAdmissionArena final {
  public:
    ~BnwNativeArchiveEntryAdmissionArena();
    BnwNativeArchiveEntryAdmissionArena(const BnwNativeArchiveEntryAdmissionArena&)=delete;
    BnwNativeArchiveEntryAdmissionArena& operator=(const BnwNativeArchiveEntryAdmissionArena&)=delete;
    void require_current()const;
    void require_same_input(const BnwNativeColdFilesystemOwner&)const;
  private:
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeArchiveCachePublicationOwner;
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;
    void require_same_input_common_source(const BnwNativeColdFilesystemOwner&,BnwNativeCommonSourceValidationContext&)const;

    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeCategoryLooseAdmissionSource;
    friend class BnwNativeCurrentDefaultSource;
    friend class BnwNativeArchiveV6EntryAdmissionOwner;
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeGlobalColorRegistryBank;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeArchiveCachePublicationOwner;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    void require_same_input_completed(const BnwNativeColdFilesystemOwner&,const BnwNativeCompletedColdSourceLoan&)const;
    void require_same_completed_owner_control(const std::weak_ptr<const BnwNativeArchiveEntryAdmissionArena>&,
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    BnwNativeDefaultNamespaceInitialReceiptDiagnostic initial_namespace_policy_receipt_completed(
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    struct GlobalDefaultRecordBorrow {
        const BnwNativeArchiveRecordDiagnostic* record=nullptr;
        const assets::AssetHandle* handle=nullptr;
        std::shared_ptr<const assets::FpkSource> source;
        const assets::FpkEntry* source_entry=nullptr;
        std::shared_ptr<const BnwNativeCategoryLooseAdmissionReceipt> loose;
    };
    //Private same-current-Default storage borrow, not a DTO/source issuer.
    GlobalDefaultRecordBorrow borrow_current_default_record_for_global_query(
        const BnwNativeCurrentDefaultSource&,std::uint32_t)const;
    GlobalDefaultRecordBorrow borrow_current_default_record_for_global_query_completed(
        const BnwNativeCurrentDefaultSource&,std::uint32_t,const BnwNativeCompletedColdSourceLoan&,
        const BnwNativeColdFilesystemOwner&)const;
    void capture_primary32_record_nodes(BnwNativePrimary32RecordSourceBindings&,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    void require_same_primary32_record_nodes(const BnwNativePrimary32RecordSourceBindings&)const;
    void bind_fresh_current_default_source(const BnwNativeCurrentDefaultSource&,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    void require_same_current_default_source(const BnwNativeCurrentDefaultSource&)const;
    void capture_default_projection_nodes(BnwNativePrimary32RecordSourceBindings&,
        NativeResourceRecordIndex&,const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    void require_same_default_projection_nodes(const BnwNativePrimary32RecordSourceBindings&,
        const NativeResourceRecordIndex&)const;
    // Late source-origin proof only: Common mode0 may extend the real Arena.
    // Original whole capture/publication validator remains unchanged above.
    void require_retained_primary_default_projection_origin_without_callbacks(
        const BnwNativePrimary32RecordSourceBindings&,const NativeResourceRecordIndex&)const;
    void require_retained_common_projection_origin_without_callbacks(
        const BnwNativePrimary32RecordSourceBindings&,const NativeResourceRecordIndex&,
        const std::vector<std::unique_ptr<const BnwNativeCommon13Step>>&,const NativeResourceRecordIndex&)const;
    BnwNativeDefaultNamespaceInitialReceiptDiagnostic initial_namespace_policy_receipt()const;
    std::shared_ptr<const BnwNativeColdFilesystemOwner> claim_initial_archive_basename_cache()const;
    static std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> acquire(
        std::shared_ptr<const BnwNativeColdFilesystemOwner>,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&);
    static std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> acquire_category(
        std::shared_ptr<const BnwNativeCategoryNamespaceChannel>);
    void require_same_category_channel(const BnwNativeCategoryNamespaceChannel&)const;
    std::shared_ptr<const BnwNativeColdFilesystemOwner> claim_category_archive_basename_cache(
        const BnwNativeCategoryNamespaceChannel&)const;
    std::uint32_t store_category_raw_record_without_fence(std::string_view,const assets::AssetHandle&,
        std::shared_ptr<const assets::FpkSource>,const assets::FpkEntry&,
        const BnwNativeArchiveV6StreamHeaderOwner&,const BnwNativeGlobalArchivePrefixRecord&,
        std::uint32_t,std::size_t)const;
    std::uint32_t store_category_loose_record(
        std::shared_ptr<const BnwNativeCategoryLooseAdmissionReceipt>,unsigned)const;
    GlobalDefaultRecordBorrow borrow_current_category_record_for_manager_query(
        const BnwNativeCategoryNamespaceChannel&,std::uint32_t)const;
    GlobalDefaultRecordBorrow borrow_current_category_record_for_manager_query_completed(
        const BnwNativeCategoryNamespaceChannel&,std::uint32_t,const BnwNativeCompletedColdSourceLoan&,
        const BnwNativeColdFilesystemOwner&)const;
    void require_installed_late_archive_record_identity_without_callbacks(
        const BnwNativeInstalledLateDefaultArchiveAttempt&,const BnwNativeArchiveV6StreamHeaderOwner&,
        const BnwNativeGlobalArchivePrefixRecord&,const std::shared_ptr<const assets::FpkSource>&,
        const assets::FpkEntry&,const assets::AssetHandle&,const BnwNativeArchiveRecordDiagnostic&)const;
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit BnwNativeArchiveEntryAdmissionArena(std::unique_ptr<Impl>);
};
// Actual83B2A0 finite loop receipt. Positive return is computed by owned source
// admission effects, not entry count/index insertion/metadata parse success.
// This does not authorize native archive basename-cache publication.
template<class> class BnwNativeCallbackOwnerGuard;
class BnwNativeArchiveV6EntryAdmissionOwner final {
  public:
    ~BnwNativeArchiveV6EntryAdmissionOwner();
    BnwNativeArchiveV6EntryAdmissionOwner(const BnwNativeArchiveV6EntryAdmissionOwner&)=delete;
    BnwNativeArchiveV6EntryAdmissionOwner& operator=(const BnwNativeArchiveV6EntryAdmissionOwner&)=delete;
    void require_current()const;
    void require_same_arena(const BnwNativeArchiveEntryAdmissionArena&)const;
    std::uint32_t admitted_count()const;
    std::uint32_t declared_count()const;
  private:
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;
    std::uint32_t admitted_count_common_source(BnwNativeCommonSourceValidationContext&)const;
    friend class BnwNativePrimary32RecordSnapshotOwner;
    friend class BnwNativePrimary32LoopJournalOwner;
    // Immediate SAME Entry FULL + closed Root/controller check only. The actual
    // receipt count is fixed before publication; no cached validation result.
    std::uint32_t admitted_count_after_validation_without_callbacks()const;

    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeArchiveCachePublicationOwner;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeGlobalArchivePrefixOwner&)const;
    void require_same_arena_completed(const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeCompletedColdSourceLoan&,
        const BnwNativeColdFilesystemOwner&,const BnwNativeGlobalArchivePrefixOwner&)const;
    std::uint32_t admitted_count_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeGlobalArchivePrefixOwner&)const;
    std::uint32_t declared_count_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeGlobalArchivePrefixOwner&)const;
    std::pair<std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner>,
              std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>> bound_archive_cache_sources_completed(
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeGlobalArchivePrefixOwner&)const;
    std::pair<std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner>,
              std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>> bound_archive_cache_sources()const;
    struct DefaultCachePublicationInputs {
        std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header;
        std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix;
        const BnwNativeColdArchiveRequest* request=nullptr;
        std::uint32_t admitted=0;
        std::int64_t mtime=0;
        std::shared_ptr<const assets::FpkSource> canonical;
    };
    DefaultCachePublicationInputs borrow_default_cache_publication_inputs(
        const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeColdArchiveRequest&,
        const std::function<void()>& controller)const;
    class DefaultCachePublicationStagingLoan final {
        friend class BnwNativeArchiveV6EntryAdmissionOwner;
        friend class BnwNativeArchiveBasenameCacheOwner;
        struct State;std::unique_ptr<State> state_;
        DefaultCachePublicationStagingLoan(const BnwNativeArchiveV6EntryAdmissionOwner&,
            const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeColdArchiveRequest&,const std::function<void()>&,
            const BnwNativeCallbackOwnerGuard<BnwNativeArchiveV6EntryAdmissionOwner>&);
        const DefaultCachePublicationInputs& inputs_without_callbacks()const;
        void require_current_without_callbacks()const;
      public:
        ~DefaultCachePublicationStagingLoan();
        DefaultCachePublicationStagingLoan(const DefaultCachePublicationStagingLoan&)=delete;
        DefaultCachePublicationStagingLoan& operator=(const DefaultCachePublicationStagingLoan&)=delete;
    };
    void with_default_cache_publication_stage(const BnwNativeArchiveEntryAdmissionArena&,
        const BnwNativeColdArchiveRequest&,const std::function<void()>&,
        std::function<void(const DefaultCachePublicationStagingLoan&)>)const;
    void require_default_cache_stage_retired_without_callbacks()const;
    static std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> acquire(
        std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena>,
        std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner>,
        std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&);
    static std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> acquire_installed_late_default(
        std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena>,
        std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner>,
        std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>,
        std::shared_ptr<const BnwNativeInstalledLateDefaultArchiveAttempt>,
        const BnwNativeInstalledLateDefaultOperation&,const ::civ5::ui::BnwNativePackageDiscoveryOwner&);
    void require_installed_late_default_origin_without_callbacks(
        const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeInstalledLateDefaultArchiveAttempt&)const;
    static std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> acquire_common(
        std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena>,
        std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner>,
        std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&,
        const BnwNativeCommon13ArchiveChannel&);
    struct CommonAdmittedRecord {
        const BnwNativeArchiveRecordDiagnostic* record=nullptr;
        const assets::AssetHandle* handle=nullptr;
        std::string raw_name;
    };
    std::vector<CommonAdmittedRecord> common_admitted_record_nodes(
        const BnwNativeCommon13ArchiveChannel&)const;

    struct CategoryAdmittedRecord {
        std::uint32_t crc=0,prefix_index=0;
        std::size_t directory_ordinal=0;
        std::shared_ptr<const assets::FpkSource> source;
        const assets::FpkEntry* source_entry=nullptr;
        const assets::AssetHandle* canonical_handle=nullptr;
    };
    static std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> acquire_category(
        std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena>,
        std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner>,
        std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>,
        std::shared_ptr<const BnwNativeCategoryArchiveChannel>);
    std::vector<CategoryAdmittedRecord> category_admitted_record_nodes()const;
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit BnwNativeArchiveV6EntryAdmissionOwner(std::unique_ptr<Impl>);
};
}
