#pragma once
namespace civ5::ui::detail {class BnwNativeCommonSourceValidationContext;}
#include "bnw_native_archive_construction.hpp"
#include <map>
#include <functional>
#include <optional>
namespace civ5::ui { class BnwNativePackageCategorySourceOwner; }
namespace civ5::ui::detail {
class BnwNativeColdFilesystemOwner;
class BnwNativeCategoryNamespaceChannel;
class BnwNativeCategoryArchiveChannel;
class BnwNativeGlobalResourceWarmOwner;
class BnwNativeArchiveEntryAdmissionArena;
class BnwNativeArchiveV6EntryAdmissionOwner;
class BnwNativeColdArchiveRequest;
class BnwNativeArchiveBasenameCacheOwner;
class BnwNativeArchiveCachePublicationOwner;
// Pure source map policy, also used by the real private owner. There is no
// route from diagnostic values/maps into an issued native cache owner.
inline bool archive_cache_positive_loader_diagnostic(std::uint32_t result) noexcept {return result!=0;}
template<class Value> class BnwArchiveBasenameCacheStoragePolicy final {
    std::map<std::string,Value,std::less<>> records_;
  public:
    const Value* lookup(std::string_view lower_basename)const noexcept {
        const auto i=records_.find(lower_basename);return i==records_.end()?nullptr:&i->second;
    }
    bool insert_unique(std::string key,Value value) {
        return records_.try_emplace(std::move(key),std::move(value)).second;
    }
    std::size_t size()const noexcept{return records_.size();}
    void swap(BnwArchiveBasenameCacheStoragePolicy& other)noexcept {records_.swap(other.records_);}
};
// A completed lookup BEFORE constructor/header. Hit forbids this caller's
// construction and publication; miss is not a positive loader result.
class BnwNativeArchiveCacheLookupOwner final {
  public:
    ~BnwNativeArchiveCacheLookupOwner();
    BnwNativeArchiveCacheLookupOwner(const BnwNativeArchiveCacheLookupOwner&)=delete;
    void require_current()const;
    bool hit()const;
  private:
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    bool hit_owned_category(const BnwNativeArchiveBasenameCacheOwner&,
        const std::shared_ptr<const BnwNativeColdArchiveRequest>&,
        const std::shared_ptr<const BnwNativeCategoryArchiveChannel>&,
        const BnwNativeGlobalResourceWarmOwner&,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeArchiveBasenameCacheOwner&)const;
    bool hit_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeArchiveBasenameCacheOwner&)const;
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit BnwNativeArchiveCacheLookupOwner(std::unique_ptr<Impl>);
};
// ONE namespace+38 cache, claimed from SAME fresh Arena before ANY Header.
class BnwNativeArchiveBasenameCacheOwner final {
  public:
    ~BnwNativeArchiveBasenameCacheOwner();
    BnwNativeArchiveBasenameCacheOwner(const BnwNativeArchiveBasenameCacheOwner&)=delete;
    void require_current()const;
    void require_same_arena(const BnwNativeArchiveEntryAdmissionArena&)const;
  private:
    friend class BnwNativeArchiveCachePublicationOwner;
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;

    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeCurrentDefaultSource;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class BnwNativeCategoryLooseAdmissionSource;
    friend class BnwNativeGlobalResourceWarmOwner;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeArchiveEntryAdmissionArena&)const;
    void require_same_arena_completed(const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeCompletedColdSourceLoan&,
        const BnwNativeColdFilesystemOwner&)const;
    void require_same_completed_owner_control(const std::weak_ptr<const BnwNativeArchiveBasenameCacheOwner>&,
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,const BnwNativeArchiveEntryAdmissionArena&)const;
    // Pure stored-slot identity only; the genuine Parent opening source retains
    // this Cache and Publication throughout the surrounding completed loan.
    void require_same_cached_publication_identity_without_callbacks(const BnwNativeArchiveCachePublicationOwner&)const;
    void require_current_lookup_completed(const BnwNativeArchiveCacheLookupOwner&,const BnwNativeCompletedColdSourceLoan&,
        const BnwNativeColdFilesystemOwner&,const BnwNativeArchiveEntryAdmissionArena&)const;
    static std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> acquire_category(
        std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena>,
        std::shared_ptr<const BnwNativeCategoryNamespaceChannel>);
    std::shared_ptr<const BnwNativeArchiveCacheLookupOwner> lookup_category_before_constructor(
        std::shared_ptr<const BnwNativeColdArchiveRequest>,
        std::shared_ptr<const BnwNativeCategoryArchiveChannel>)const;
    std::shared_ptr<const BnwNativeArchiveCacheLookupOwner> lookup_owned_category_before_constructor(
        std::shared_ptr<const BnwNativeColdArchiveRequest>,
        std::shared_ptr<const BnwNativeCategoryArchiveChannel>,const BnwNativeGlobalResourceWarmOwner&,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    std::shared_ptr<const BnwNativeArchiveCachePublicationOwner> publish_category_after_entries(
        std::shared_ptr<const BnwNativeArchiveCacheLookupOwner>,
        std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner>,
        std::shared_ptr<const BnwNativeCategoryArchiveChannel>)const;
    friend class BnwNativeArchiveCacheLookupOwner;
    friend class BnwNativeArchiveCachePublicationOwner;
    static std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> acquire(
        std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena>,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&);
    std::shared_ptr<const BnwNativeArchiveCacheLookupOwner> lookup_before_constructor(
        std::shared_ptr<const BnwNativeColdArchiveRequest>,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    std::shared_ptr<const BnwNativeArchiveCachePublicationOwner> publish_after_entries(
        std::shared_ptr<const BnwNativeArchiveCacheLookupOwner>,
        std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner>,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit BnwNativeArchiveBasenameCacheOwner(std::unique_ptr<Impl>);
};
// Positive admitted loop -> cache store. Neither default VFS publication nor
// full startup is certified. Cache retains this value; it has only weak backedge.
class BnwNativeArchiveCachePublicationOwner final {
  public:
    ~BnwNativeArchiveCachePublicationOwner();
    BnwNativeArchiveCachePublicationOwner(const BnwNativeArchiveCachePublicationOwner&)=delete;
    void require_current()const;
    std::int64_t checked_original_mtime()const;
    bool checked_preference280()const;
    std::shared_ptr<const assets::FpkSource> canonical_payload_source()const;
  private:
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;

    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativePrimary32RecordSnapshotOwner;
    // Root/Snapshot follow SAME Publication FULL current + Root/leaf fence. Void
    // identity comparisons only, with no escaping observation or payload.
    void require_default_projection_payload_identity_without_callbacks(
        const BnwNativeArchiveV6StreamHeaderOwner&)const;
    friend class BnwNativePrimary32LoopJournalOwner;
    // SAME Publication FULL current + Journal/Root check immediately precede.
    void require_primary32_construction_identity_without_callbacks(
        const BnwNativeArchiveConstruction&)const;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeGlobalArchivePrefixOwner&,const BnwNativeArchiveBasenameCacheOwner&)const;
    std::int64_t checked_original_mtime_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeGlobalArchivePrefixOwner&,const BnwNativeArchiveBasenameCacheOwner&)const;
    bool checked_preference280_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeArchiveEntryAdmissionArena&,const BnwNativeGlobalArchivePrefixOwner&,const BnwNativeArchiveBasenameCacheOwner&)const;
    std::shared_ptr<const assets::FpkSource> canonical_payload_source_completed(const BnwNativeCompletedColdSourceLoan&,
        const BnwNativeColdFilesystemOwner&,const BnwNativeArchiveEntryAdmissionArena&,
        const BnwNativeGlobalArchivePrefixOwner&,const BnwNativeArchiveBasenameCacheOwner&)const;
    void disable_preference280_from_category(const BnwNativeCategoryNamespaceChannel&)const;
    //Parent-only phase borrower; no new public field/currentness API.
    class Primary32ReadLoan;
    void require_primary32_construction_fields(
        const BnwNativeArchiveConstruction&,const std::function<void()>&)const;
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit BnwNativeArchiveCachePublicationOwner(std::unique_ptr<Impl>);
};
}
