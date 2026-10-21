#pragma once
#include "bnw_native_original_stat_service.hpp"
#include "civ5/assets/asset_source.hpp"
#include <algorithm>
#include <functional>
#include <utility>
#include <stdexcept>
#include <string_view>
namespace civ5::ui { class BnwNativePackageDiscoveryOwner; class BnwNativePackageCategorySourceOwner; }
namespace civ5::ui::detail {
class BnwNativeCommonSourceValidationContext;
class BnwNativeColdFilesystemOwner;
class BnwNativeColdArchiveRequest;
class BnwNativeArchiveV6StreamHeaderOwner;
class BnwNativeCompletedColdSourceLoan;
class BnwNativeGlobalArchivePrefixOwner;
class BnwNativeGlobalArchivePrefixRecord;
class BnwNativeArchiveCachePublicationOwner;
class BnwNativeCategoryNamespaceChannel;
// Pure source formula only. Scalars never authenticate a constructed archive.
inline std::int64_t archive_checked_stat_mtime(const BnwNativeOriginalStatFields& stat) {
    if(stat.return_value!=0)return 0;
    if(!stat.mtime)throw std::runtime_error("Checked archive stat successful time unknown");
    return *stat.mtime;
}
//612020/81DA10: LAST backslash, otherwise FIRST colon. Forward slash remains
//a byte in the key; generic resource_basename/filename normalization is wrong.
inline std::string archive_cache_basename_ascii(std::string_view raw) {
    if(!std::all_of(raw.begin(),raw.end(),[](unsigned char c){return c<128&&c!=0;}))
        throw std::runtime_error("Native archive cache key requires ASCII source scope");
    const auto backslash=raw.rfind('\\');
    const auto delimiter=backslash==std::string_view::npos?raw.find(':'):backslash;
    auto out=std::string(delimiter==std::string_view::npos?raw:raw.substr(delimiter+1));
    for(auto& c:out)if(c>='A'&&c<='Z')c=static_cast<char>(c+('a'-'A'));
    return out;
}
// Closed source metadata preparation for root's actual default archive caller.
// No public factory, caller bind, prefix-index token or cache-ready certificate.
// Root must retain constructor-stage ordering and mapped stream/prefix ownership.
class BnwNativeArchiveConstruction final {
  public:
    ~BnwNativeArchiveConstruction();
    BnwNativeArchiveConstruction(const BnwNativeArchiveConstruction&) = delete;
    BnwNativeArchiveConstruction& operator=(const BnwNativeArchiveConstruction&) = delete;
    void require_current() const;
    void require_same_input(const BnwNativeEarlyInputLease&,
                            const BnwNativeOriginalStatService&) const;
    const BnwNativeOriginalStatRead& stat_before_open() const;
    std::int64_t checked_original_mtime() const;
    bool checked_preference280() const;
    const std::filesystem::path& direct_original_path_candidate() const;
    const std::string& direct_archive_basename_candidate() const;
    const std::shared_ptr<const assets::FpkSource>& canonical_payload_source() const;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeArchiveV6StreamHeaderOwner;
    // Header reads immediately after SAME Construction FULL and its pure parent check.
    const std::filesystem::path& direct_original_path_candidate_after_validation_without_callbacks()const noexcept;
    static std::shared_ptr<const BnwNativeArchiveConstruction> begin(
        std::shared_ptr<const BnwNativeColdFilesystemOwner>,
        std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner>,
        std::shared_ptr<const BnwNativeOriginalStatService>,
        std::shared_ptr<const BnwNativeColdArchiveRequest>);
    void require_same_input(const ::civ5::ui::BnwNativeInstallationInputOwner&,
                            const BnwNativeOriginalStatService&)const;

    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeArchiveCachePublicationOwner;
    // Private immutable payload identity; no fresh-source/read authority.
    void require_canonical_payload_identity_without_callbacks(const assets::FpkSource*)const;
    void require_primary32_cached_fields_identity_without_callbacks(
        std::int64_t,const assets::FpkSource*)const;
    void disable_preference280_from_cached_category(const BnwNativeArchiveV6StreamHeaderOwner&,
                                                   const BnwNativeCategoryNamespaceChannel&)const;
    friend class BnwNativeGlobalArchivePrefixOwner;
    friend class BnwNativeGlobalArchivePrefixRecord;
    // Explicit completed observation of the SAME immutable constructed source.
    // These operations publish/retry no archive and borrow no retired Skin loan.
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;
    void require_same_input_common_source(const BnwNativeEarlyInputLease&,const BnwNativeOriginalStatService&,
                                          BnwNativeCommonSourceValidationContext&)const;
    const std::filesystem::path& direct_original_path_candidate_common_source(BnwNativeCommonSourceValidationContext&)const;
    const BnwNativeOriginalStatRead& stat_before_open_common_source(BnwNativeCommonSourceValidationContext&)const;
    std::int64_t checked_original_mtime_common_source(BnwNativeCommonSourceValidationContext&)const;
    const std::shared_ptr<const assets::FpkSource>& canonical_payload_source_common_source(BnwNativeCommonSourceValidationContext&)const;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&)const;
    void require_same_input_completed(const BnwNativeEarlyInputLease&,
        const BnwNativeOriginalStatService&,const BnwNativeCompletedColdSourceLoan&)const;
    const BnwNativeOriginalStatRead& stat_before_open_completed(const BnwNativeCompletedColdSourceLoan&)const;
    std::int64_t checked_original_mtime_completed(const BnwNativeCompletedColdSourceLoan&)const;
    bool checked_preference280_completed(const BnwNativeCompletedColdSourceLoan&)const;
    const std::filesystem::path& direct_original_path_candidate_completed(const BnwNativeCompletedColdSourceLoan&)const;
    const std::string& direct_archive_basename_candidate_completed(const BnwNativeCompletedColdSourceLoan&)const;
    const std::shared_ptr<const assets::FpkSource>& canonical_payload_source_completed(const BnwNativeCompletedColdSourceLoan&)const;
    // Default publication-only scalar snapshot. Controller is the actual
    // Cache/Parent pure fence, borrowed only for this operation; no public use.
    std::pair<std::int64_t,std::shared_ptr<const assets::FpkSource>>
        cache_publication_metadata(const std::function<void()>& controller)const;
    // One-way claim precedes header opens; a failed read cannot retry this epoch.
    void claim_header_before_entry_metadata() const;
    // Validate exact PREPARED brands before the shared prefix store; completion
    // performs no allocation/validation and retains the committed token weakly.
    void require_shared_prefix_commit_available(const BnwNativeArchiveV6StreamHeaderOwner&,
                                                const BnwNativeGlobalArchivePrefixRecord&) const;
    void record_shared_prefix_before_entry_metadata(const BnwNativeArchiveV6StreamHeaderOwner&,
        const std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>&) const noexcept;
    static std::shared_ptr<const BnwNativeArchiveConstruction> begin(
        std::shared_ptr<const BnwNativeEarlyInputLease>,
        std::shared_ptr<const BnwNativeOriginalStatService>,
        const std::filesystem::path& source_relative);
    // Called ONLY by the root owner access route before resource registration.
    // Does not execute native mapped-stream callback or issue a prefix token.
    void prepare_original_and_canonical_metadata() const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativeArchiveConstruction(std::unique_ptr<Impl>);
};
}
