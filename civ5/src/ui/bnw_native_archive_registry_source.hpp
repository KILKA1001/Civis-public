#pragma once
#include <functional>
#include <span>
#include <tuple>
#include "bnw_native_cold_filesystem_owner.hpp"
#include "bnw_native_archive_registry_kernel.hpp"
namespace civ5::ui::detail {
class BnwNativeCommonSourceValidationContext;
class BnwNativeCategoryLooseAdmissionSource;
class BnwNativeGlobalArchivePrefixOwner;
class BnwNativeGlobalArchivePrefixRecord;
struct BnwNativeCachedPayloadReadResult {std::uint32_t count=0,error=0;};
// The source entry admission producer is intentionally absent. Header declared
// count/metadata parser results never construct or substitute its capability.
class BnwNativeArchiveV6EntryAdmissionOwner;

// Concrete finite header-only source stream. SAME714 stat observation precedes
// open; root one-way claim must precede any FpkSource entry metadata parse.
class BnwNativeArchiveV6StreamHeaderOwner final {
  public:
    ~BnwNativeArchiveV6StreamHeaderOwner();
    BnwNativeArchiveV6StreamHeaderOwner(const BnwNativeArchiveV6StreamHeaderOwner&)=delete;
    BnwNativeArchiveV6StreamHeaderOwner& operator=(const BnwNativeArchiveV6StreamHeaderOwner&)=delete;
    void require_current() const;
    const BnwNativeColdArchiveRequest& source_request() const;
    const BnwNativeArchiveConstruction& construction() const;
    const BnwNativeOriginalStatService& original_stat_service() const;
    const std::u16string& raw_stream_name() const;
    const BnwArchiveV6HeaderFields& header_fields() const;
    std::uint64_t original_byte_length() const;
    // Mandatory83B2A0/namespace entry-loop producer remains unowned.
    void require_native_entry_admission_for_cache() const;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeGlobalArchivePrefixOwner;
    friend class BnwNativeGlobalArchivePrefixRecord;
    friend class BnwNativeArchiveV6EntryAdmissionOwner;
    static std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> acquire(
        std::shared_ptr<const BnwNativeColdFilesystemOwner>,
        std::shared_ptr<const BnwNativeColdArchiveRequest>,
        std::shared_ptr<const BnwNativeArchiveConstruction>,
        std::shared_ptr<const BnwNativeOriginalStatService>);
    friend class BnwNativeGlobalResourceWarmOwner;
    BnwNativeCachedPayloadReadResult read_retained_original_cached_stream(
        std::uint32_t absolute_offset,std::span<std::uint8_t>)const;
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeArchiveCachePublicationOwner;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeDefaultNamespacePolicyOwner;
    friend class BnwNativeArchiveV6EntryAdmissionOwner;
    friend class BnwNativePrimary32RecordSnapshotOwner;
    // Root/Snapshot call only immediately after FULL Header + Root/leaf fence.
    // These void comparisons issue no source/currentness/read capability.
    void require_default_projection_request_identity_without_callbacks(
        const BnwNativeColdFilesystemOwner&,std::size_t ordinal)const;
    friend class BnwNativePrimary32LoopJournalOwner;
    // Journal calls immediately after this Header FULL current + Root check.
    void require_primary32_source_identities_without_callbacks(
        const BnwNativeColdArchiveRequest&,const BnwNativeArchiveConstruction&)const;
    void require_constructed_payload_identity_without_callbacks(const assets::FpkSource*)const;
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;
    const BnwNativeColdArchiveRequest& source_request_common_source(BnwNativeCommonSourceValidationContext&)const;
    const std::u16string& raw_stream_name_common_source(BnwNativeCommonSourceValidationContext&)const;
    // Private explicit completed source route. No new header/prefix publication.
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    const BnwNativeColdArchiveRequest& source_request_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    const BnwNativeArchiveConstruction& construction_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    const BnwNativeOriginalStatService& original_stat_service_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    const std::u16string& raw_stream_name_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    const BnwArchiveV6HeaderFields& header_fields_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    std::uint64_t original_byte_length_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    BnwNativeCachedPayloadReadResult read_retained_original_cached_stream_completed(
        std::uint32_t,std::span<std::uint8_t>,const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    // Sealed initial/default cache borrower; public getters stay unchanged.
    std::tuple<const BnwNativeColdArchiveRequest*,std::int64_t,std::shared_ptr<const assets::FpkSource>>
        cache_publication_metadata(const BnwNativeColdArchiveRequest&,std::uint32_t admitted,
                                   const std::function<void()>& controller)const;
    void claim_prefix_attempt() const;
    // Pure stored ancestry identity ONLY, never currentness/read permission.
    // Parent must retain genuine terminal Stage/Prefix/publication storage and
    // its completed Observation/Cold loan across the full callback lifetime.
    void require_same_retained_request_identity_without_callbacks(const BnwNativeColdArchiveRequest&,
        const std::weak_ptr<const BnwNativeColdArchiveRequest>& actual_factory_request)const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativeArchiveV6StreamHeaderOwner(std::unique_ptr<Impl>);
};

// A real source prefix registration token. Prefix strings persist independently
// of an archive's later0-admission; this token additionally binds SAMEheader.
class BnwNativeGlobalArchivePrefixRecord final {
  public:
    ~BnwNativeGlobalArchivePrefixRecord();
    BnwNativeGlobalArchivePrefixRecord(const BnwNativeGlobalArchivePrefixRecord&)=delete;
    BnwNativeGlobalArchivePrefixRecord& operator=(const BnwNativeGlobalArchivePrefixRecord&)=delete;
    void require_current() const;
    void require_same_owner(const BnwNativeGlobalArchivePrefixOwner&) const;
    const BnwNativeArchiveV6StreamHeaderOwner& header_owner() const;
    std::uint32_t source_index() const;
    const std::u16string& raw_prefix() const;
  private:
    friend class BnwNativeGlobalArchivePrefixOwner;
    friend class BnwNativeArchiveConstruction;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativePrimary32RecordSnapshotOwner;
    // Used immediately after SAME Prefix FULL validation; immutable link only.
    void require_header_identity_without_callbacks(const BnwNativeArchiveV6StreamHeaderOwner&)const;
    // Common entry: immediately after this SAME Prefix FULL and pure Root checks.
    // Only its immutable Header identity; no ownership/currentness verdict.
    const BnwNativeArchiveV6StreamHeaderOwner& header_owner_after_validation_without_callbacks()const;
    // Snapshot follows SAME Prefix FULL + original closed Root/leaf checks.
    // Sole pre-publication index store; no currentness or source capability.
    std::uint32_t source_index_after_validation_without_callbacks()const;
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeArchiveCachePublicationOwner;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeDefaultNamespacePolicyOwner;
    friend class BnwNativeArchiveV6EntryAdmissionOwner;
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;
    void require_same_owner_common_source(const BnwNativeGlobalArchivePrefixOwner&,BnwNativeCommonSourceValidationContext&)const;
    const BnwNativeArchiveV6StreamHeaderOwner& header_owner_common_source(BnwNativeCommonSourceValidationContext&)const;
    std::uint32_t source_index_common_source(BnwNativeCommonSourceValidationContext&)const;
    const std::u16string& raw_prefix_common_source(BnwNativeCommonSourceValidationContext&)const;
    // Actual registry is a genuine retained parent reference, not weak.lock()
    // storage that could mask its last outside facade owner disappearing.
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeGlobalArchivePrefixOwner&)const;
    const BnwNativeArchiveV6StreamHeaderOwner& header_owner_completed(const BnwNativeCompletedColdSourceLoan&,
        const BnwNativeColdFilesystemOwner&,const BnwNativeGlobalArchivePrefixOwner&)const;
    std::uint32_t source_index_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeGlobalArchivePrefixOwner&)const;
    const std::u16string& raw_prefix_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const BnwNativeGlobalArchivePrefixOwner&)const;
    // Full currentness is outside Construction's fence. These private helpers
    // compare only sealed brands/actual active phases; they never call getters,
    // UI callbacks, current(), or construct scalar authority.
    void require_prepared_identity_for(const BnwNativeArchiveConstruction&,
                                       const BnwNativeArchiveV6StreamHeaderOwner&) const;
    void require_published_identity_for(const BnwNativeArchiveConstruction&) const;
    // Sole claimed-header route to entry metadata; once + real source current
    // validation precede Construction's fence and remain retained through it.
    void prepare_bound_construction_metadata() const;
    BnwArchivePrefixIdentityDiagnostic bound_identity_without_callbacks() const;
    // Typed reference to THIS record's original strong Header anchor only.
    // It neither opens a fence nor grants authority from a supplied pointer.
    const BnwNativeArchiveV6StreamHeaderOwner& retained_header_identity_without_callbacks()const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativeGlobalArchivePrefixRecord(std::unique_ptr<Impl>);
};

// ONE root-retained shared registry across default/primary/Common/category
// source stages. No per-archive recreation/relabeling of a copied namespace.
class BnwNativeGlobalArchivePrefixOwner final {
  public:
    ~BnwNativeGlobalArchivePrefixOwner();
    BnwNativeGlobalArchivePrefixOwner(const BnwNativeGlobalArchivePrefixOwner&)=delete;
    BnwNativeGlobalArchivePrefixOwner& operator=(const BnwNativeGlobalArchivePrefixOwner&)=delete;
    void require_current() const;
    void require_same_input(const BnwNativeColdFilesystemOwner&) const;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeCategoryLooseAdmissionSource;
    friend class BnwNativeGlobalArchivePrefixRecord;
    friend class BnwNativeCategoryLooseAdmissionSource;
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeArchiveCachePublicationOwner;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeDefaultNamespacePolicyOwner;
    friend class BnwNativeArchiveV6EntryAdmissionOwner;
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;
    void require_same_input_common_source(const BnwNativeColdFilesystemOwner&,BnwNativeCommonSourceValidationContext&)const;
    void require_record_common_source(const BnwNativeGlobalArchivePrefixRecord&,bool,BnwNativeCommonSourceValidationContext&)const;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    void require_same_input_completed(const BnwNativeColdFilesystemOwner&,const BnwNativeCompletedColdSourceLoan&)const;
    void require_record_completed(const BnwNativeGlobalArchivePrefixRecord&,const BnwNativeCompletedColdSourceLoan&,
        const BnwNativeColdFilesystemOwner&)const;
    std::u16string read_registered_loose_prefix_completed(std::uint32_t,const BnwNativeCompletedColdSourceLoan&,
        const BnwNativeColdFilesystemOwner&)const;
    static std::shared_ptr<const BnwNativeGlobalArchivePrefixOwner> acquire(
        std::shared_ptr<const BnwNativeColdFilesystemOwner>);
    std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> register_header_prefix(
        std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner>) const;
    std::uint32_t register_skin_source_prefix(std::u16string,std::function<void()>)const;
    std::uint32_t register_loose_source_prefix(std::u16string,std::function<void()>)const;
    std::u16string read_registered_loose_prefix(std::uint32_t)const;
    void require_record(const BnwNativeGlobalArchivePrefixRecord&,bool published) const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativeGlobalArchivePrefixOwner(std::unique_ptr<Impl>);
};
}
