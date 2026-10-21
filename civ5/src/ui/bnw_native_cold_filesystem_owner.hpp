#pragma once
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "bnw_native_completed_cold_source_loan.hpp"
#include <functional>
#include <array>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace civ5::ui { class BnwNativePackageDiscoveryOwner; class BnwNativePackageCategorySourceOwner; class BnwNativeInstallationInputOwner; }
namespace civ5::ui::detail {class BnwNativeCommon13ArchiveChannel;class BnwNativeInstalledLateDefaultArchiveAttempt;}
namespace civ5::ui::detail {
// Pure source constants/formulas. A string returned here is never a lease,
// source epoch, stream, prefix/cache entry or authenticated archive request.
// Pinned source B843A0[32], A0392C; audit055c5075fe executes all32 callers.
inline constexpr std::array<std::string_view,32> bnw_primary_archive_descriptors = {
    "AmericanWashingtonTextures",
    "ArabiaHarunAl-RashidTextures",
    "AztecMontezumaTextures",
    "BuildingTextures",
    "ChineseWuTextures",
    "CityTextures",
    "DecalTextures",
    "EffectTextures",
    "EgyptianRamessesTextures",
    "EnglandElizabethTextures",
    "FeatureTextures",
    "FrenchNapoleonTextures",
    "GermanBismarckTextures",
    "GreeceAlexanderTextures",
    "HutsBarbarianFortTextures",
    "ImprovementTextures",
    "IndianGandhiTextures",
    "IroquoisHiawathaTextures",
    "JapaneseOdaNobunugaTextures",
    "LeadersTextures",
    "MiscTextures",
    "OttomanSuliemanTextures",
    "PersiaDariusTextures",
    "RomanCaesarTextures",
    "RussianCatherineTextures",
    "SiamRamkhamaengTextures",
    "SonghaiAskiaTextures",
    "StrategicViewTextures",
    "TerrainTextures",
    "UITextures",
    "UnitsTextures",
    "WonderTextures"
};
inline std::string bnw_primary_archive_raw_ansi_literal(std::size_t ordinal) {
    if(ordinal>=bnw_primary_archive_descriptors.size())
        throw std::runtime_error("Native primary archive ordinal outside source32 family");
    return std::string("Resource\\DX9\\")+
           std::string(bnw_primary_archive_descriptors[ordinal])+".fpk";
}
inline std::u16string bnw_primary_archive_ascii_stream_token(std::string_view raw) {
    std::u16string out;out.reserve(raw.size());
    for(const char byte:raw) {
        const auto value=static_cast<unsigned char>(byte);
        if(value==0 || value>=128)
            throw std::runtime_error("Cold source stream token lacks non-ASCII CP_ACP proof");
        out.push_back(value=='/'?u'\\':static_cast<char16_t>(value));
    }
    return out;
}

class BnwNativeCommonSourceValidationContext;
class BnwNativeGlobalResourceWarmOwner;
class BnwNativeCategoryArchiveChannel;
class BnwNativeCategoryNamespaceChannel;
class BnwNativeColdFilesystemOwner;
class BnwNativeGlobalArchivePrefixOwner;
class BnwNativeArchiveV6StreamHeaderOwner;
class BnwNativeArchiveEntryAdmissionArena;
// Immutable source-issued name preparation ONLY. No stream/header/registration
// receipt. Getters refuse after the issuing cold epoch dies or its lease stales.
class BnwNativeColdArchiveRequest final {
  public:
    ~BnwNativeColdArchiveRequest();
    BnwNativeColdArchiveRequest(const BnwNativeColdArchiveRequest&)=delete;
    BnwNativeColdArchiveRequest& operator=(const BnwNativeColdArchiveRequest&)=delete;
    void require_current() const;
    void require_same_owner(const BnwNativeColdFilesystemOwner&) const;
    std::size_t source_ordinal() const;
    const std::string& raw_relative_ansi_request() const;
    const std::u16string& raw_utf16_stream_name_candidate() const;
    const std::filesystem::path& original_relative_io_token() const;
    const std::filesystem::path& original_absolute_io_candidate() const;
    const std::filesystem::path& canonical_payload_io_candidate() const;
    // Candidate name is NOT a registered prefix; both mandatory transitions
    // remain unowned by this preparation. No fabricated index/cache defaults.
    void require_native_prefix_registration() const;
    void require_native_archive_cache_admission() const;
  private:
    friend class BnwNativeColdFilesystemOwner;
    friend class BnwNativeArchiveV6EntryAdmissionOwner;
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeArchiveV6StreamHeaderOwner;
    friend class BnwNativeArchiveConstruction;
    friend class BnwNativeGlobalArchivePrefixOwner;
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;
    void require_same_owner_common_source(const BnwNativeColdFilesystemOwner&,BnwNativeCommonSourceValidationContext&)const;
    const std::string& raw_relative_ansi_request_common_source(BnwNativeCommonSourceValidationContext&)const;
    const std::u16string& raw_utf16_stream_name_candidate_common_source(BnwNativeCommonSourceValidationContext&)const;
    const std::filesystem::path& original_relative_io_token_common_source(BnwNativeCommonSourceValidationContext&)const;
    const std::filesystem::path& original_absolute_io_candidate_common_source(BnwNativeCommonSourceValidationContext&)const;
    const std::filesystem::path& canonical_payload_io_candidate_common_source(BnwNativeCommonSourceValidationContext&)const;
    class CompletedDescriptor final {
        friend class BnwNativeColdArchiveRequest;
        const BnwNativeColdArchiveRequest& request_;
        const BnwNativeCompletedColdSourceLoan& loan_;
        CompletedDescriptor(const BnwNativeColdArchiveRequest& request,const BnwNativeCompletedColdSourceLoan& loan)
            :request_(request),loan_(loan){}
      public:
        CompletedDescriptor(const CompletedDescriptor&)=delete;
        CompletedDescriptor&operator=(const CompletedDescriptor&)=delete;
        CompletedDescriptor(CompletedDescriptor&&)=delete;
        CompletedDescriptor&operator=(CompletedDescriptor&&)=delete;
        void require_current()const;
        std::size_t source_ordinal()const;
        const std::string& raw_relative_ansi_request()const;
        const std::u16string& raw_utf16_stream_name_candidate()const;
        const std::filesystem::path& original_relative_io_token()const;
        const std::filesystem::path& original_absolute_io_candidate()const;
        const std::filesystem::path& canonical_payload_io_candidate()const;
    };
    void with_current_completed_descriptor(const BnwNativeCompletedColdSourceLoan&,
        const std::function<void(const CompletedDescriptor&)>&)const;
    void require_same_category_channel(const BnwNativeCategoryArchiveChannel&) const;
    void require_same_installed_late_default_attempt(const BnwNativeInstalledLateDefaultArchiveAttempt&)const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativeColdArchiveRequest(std::unique_ptr<Impl>);
};

// Private fresh stock source epoch, not recovered original process state.
// Owns initially-null callback memory; does not mutate ambient OS CWD or expose
// a setter. Physical source base is logical routing from retained original EXE.
class BnwNativeColdFilesystemOwner final {
  public:
    ~BnwNativeColdFilesystemOwner();
    BnwNativeColdFilesystemOwner(const BnwNativeColdFilesystemOwner&)=delete;
    BnwNativeColdFilesystemOwner& operator=(const BnwNativeColdFilesystemOwner&)=delete;
    void require_current() const;
    void require_same_input(const BnwNativeEarlyInputLease&) const;
    void require_fresh_direct_original_branch() const;
    const std::shared_ptr<const BnwNativeEarlyInputLease>& early_input_lease() const;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend struct BnwNativeFirstArchiveColdRegression;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class BnwNativeGlobalArchivePrefixOwner;
    friend class BnwNativeArchiveV6StreamHeaderOwner;
    friend class BnwNativeArchiveConstruction;
    friend class BnwNativeCurrentDefaultSource;
    friend class BnwNativePrimary32RecordSnapshotOwner;
    // Owned fresh epoch: ONE registry before ANY archive header operation.
    // Header operations may repeat for different constructions after the claim.
    void claim_initial_prefix_registry() const;
    friend class BnwNativeArchiveEntryAdmissionArena;
    void claim_initial_entry_admission_arena() const;
    void require_before_any_archive_request_and_header_without_callbacks() const;
    void require_and_mark_archive_header_phase() const;
    friend class BnwNativeColdArchiveRequest;
    friend class BnwNativeInstalledLateDefaultArchiveAttempt;
    friend class BnwNativeCompletedColdSourceLoan;
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeDefaultNamespacePolicyOwner;
    // Policy calls immediately after SAME Cold FULL + its persistent-owner check.
    void require_initial_policy_lease_identity_without_callbacks(const BnwNativeEarlyInputLease&)const;
    // Explicit same-owner control check; no weak lock adds a hidden Cold pin.
    void require_same_completed_owner_control(const std::weak_ptr<const BnwNativeColdFilesystemOwner>&,
        const BnwNativeCompletedColdSourceLoan&)const;
    void with_same_completed_source(const BnwNativeCompletedParentObservation::Loan&,
        const BnwNativeCompletedColdSourceLoan::Source&,
        const std::function<void(const BnwNativeCompletedColdSourceLoan&)>&)const;
    void require_current_completed_request(const BnwNativeColdArchiveRequest&,
        const BnwNativeCompletedColdSourceLoan&)const;
    static std::shared_ptr<const BnwNativeColdFilesystemOwner> acquire(
        std::shared_ptr<const BnwNativeEarlyInputLease>);
    static std::shared_ptr<const BnwNativeColdFilesystemOwner> acquire(
        std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner>);
    void require_same_input(const ::civ5::ui::BnwNativeInstallationInputOwner&)const;
    const std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner>& installation_input_for_source()const;
    std::shared_ptr<const BnwNativeColdArchiveRequest> request_from_common(
        std::shared_ptr<const BnwNativeCommon13ArchiveChannel>)const;
    std::shared_ptr<const BnwNativeColdArchiveRequest> request_from_installed_late_default(
        std::shared_ptr<const BnwNativeInstalledLateDefaultArchiveAttempt>)const;
    // Root's source loop calls this PRIVATE producer at its actual ordinal.
    // Issuance never advances or certifies default-manager/native loop progress.
    std::shared_ptr<const BnwNativeColdArchiveRequest> primary_request_at_source_ordinal(
        std::size_t) const;
    void require_current_request(const BnwNativeColdArchiveRequest&) const;
    // Header's immediate Default identity comparison only; no source verdict.
    void require_request_ordinal_identity_without_callbacks(
        const BnwNativeColdArchiveRequest&,std::size_t ordinal)const;
    void require_common_source_current(BnwNativeCommonSourceValidationContext&)const;
    void require_same_input_common_source(const BnwNativeEarlyInputLease&,BnwNativeCommonSourceValidationContext&)const;
    void require_fresh_direct_original_branch_common_source(BnwNativeCommonSourceValidationContext&)const;
    const std::shared_ptr<const BnwNativeEarlyInputLease>& early_input_lease_common_source(BnwNativeCommonSourceValidationContext&)const;
    void require_current_request_common_source(const BnwNativeColdArchiveRequest&,BnwNativeCommonSourceValidationContext&)const;
    // Only a genuine retained category attempt can issue a late request.
    std::shared_ptr<const BnwNativeColdArchiveRequest> request_from_category(
        std::shared_ptr<const BnwNativeCategoryArchiveChannel>) const;
    std::shared_ptr<const BnwNativeColdArchiveRequest> request_from_owned_category(
        std::shared_ptr<const BnwNativeCategoryArchiveChannel>,const BnwNativeGlobalResourceWarmOwner&,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativeColdFilesystemOwner(std::unique_ptr<Impl>);
};
}
