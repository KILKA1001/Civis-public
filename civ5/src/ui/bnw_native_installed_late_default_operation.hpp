#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace civ5::ui {
class BnwNativePackageDiscoveryOwner;
namespace detail {
class BnwNativeGlobalResourceWarmOwner;
class BnwNativeColdFilesystemOwner;
class BnwNativeColdArchiveRequest;
class BnwNativeArchiveConstruction;
class BnwNativeArchiveV6StreamHeaderOwner;
class BnwNativeArchiveEntryAdmissionArena;
class BnwNativeArchiveV6EntryAdmissionOwner;
class BnwNativeArchiveCacheLookupOwner;
class BnwNativeArchiveCachePublicationOwner;
class BnwNativeInstalledLateDefaultOperation;

// No accepting implementation lives here. Only the future real Warm Speech
// operation/ActiveCore loan can construct a derived donor. Paths/ordinals/flags
// supplied by an external caller cannot construct this closed physical role.
class BnwNativeInstalledLateSpeechCoreDonor {
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeInstalledLateDefaultOperation;
    BnwNativeInstalledLateSpeechCoreDonor()=default;
    BnwNativeInstalledLateSpeechCoreDonor(const BnwNativeInstalledLateSpeechCoreDonor&)=delete;
    BnwNativeInstalledLateSpeechCoreDonor& operator=(const BnwNativeInstalledLateSpeechCoreDonor&)=delete;
    // All identity methods are callback/allocation-free and authenticate SAME
    // physical operation/table/loan/Core occurrence, not a copied observation.
    virtual void require_operation_identity_without_callbacks(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const=0;
    virtual void require_active_core_identity_without_callbacks(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const=0;
    virtual void require_active_core_current(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const=0;
    virtual const void* active_core_identity_without_callbacks(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const=0;
    // Actual physical directory-call occurrence, minted by the closed Core
    // invocation. The small immutable token owns neither Parent nor Core.
    // A later genuine invocation may reuse the same Core/address/path.
    virtual const std::shared_ptr<const void>& active_directory_occurrence_brand_without_callbacks(
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const=0;
    virtual const std::string& emitted_62ec20_path_without_callbacks(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const=0;
    // Called only at the actual outside E3a close, after all real physical POST,
    // Bank/whole Loaded/Header/new continuation and callback targets retire.
    virtual void require_speech_return_without_callbacks(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const=0;
  protected:
    virtual ~BnwNativeInstalledLateSpeechCoreDonor()=default;
};

class BnwNativeInstalledLateDefaultArchiveAttempt;
// Root alone mints this noncopyable synchronous namespace writer from the actual
// closed ActiveCore donor. It has no public ctor/current token/allow switch.
class BnwNativeInstalledLateDefaultOperation final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeColdFilesystemOwner;
    friend class BnwNativeColdArchiveRequest;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeArchiveV6EntryAdmissionOwner;
    friend class BnwNativeArchiveCacheLookupOwner;
    friend class BnwNativeArchiveCachePublicationOwner;
    friend class BnwNativeInstalledLateDefaultArchiveAttempt;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    BnwNativeInstalledLateDefaultOperation(const ::civ5::ui::BnwNativePackageDiscoveryOwner&,
        const BnwNativeInstalledLateSpeechCoreDonor&);
    ~BnwNativeInstalledLateDefaultOperation();
    BnwNativeInstalledLateDefaultOperation(const BnwNativeInstalledLateDefaultOperation&)=delete;
    BnwNativeInstalledLateDefaultOperation& operator=(const BnwNativeInstalledLateDefaultOperation&)=delete;
    void require_active_core_current(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    void require_fence_without_callbacks(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    const std::string& emitted_speech_path(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    unsigned normal_default_loose_mode(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    const BnwNativeColdFilesystemOwner& cold_without_callbacks(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    const std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner>& parent_without_callbacks()const;
    const std::shared_ptr<const void>& source_brand_without_callbacks()const;
    const std::shared_ptr<const void>& core_brand_without_callbacks()const;
    const void* core_identity_without_callbacks()const;
    std::size_t next_archive_ordinal_without_callbacks()const;
};

// Source owns every actual attempt through Header/Entry/Cache receipt lifetime.
// No retained field points to the departed stack Operation or CoreDonor.
class BnwNativeInstalledLateDefaultArchiveAttempt final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeColdFilesystemOwner;
    friend class BnwNativeColdArchiveRequest;
    friend class BnwNativeArchiveConstruction;
    friend class BnwNativeArchiveV6StreamHeaderOwner;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeArchiveV6EntryAdmissionOwner;
    friend class BnwNativeArchiveCacheLookupOwner;
    friend class BnwNativeArchiveCachePublicationOwner;
    const std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent_;
    const std::weak_ptr<const BnwNativeColdFilesystemOwner> cold_;
    const std::shared_ptr<const void> source_brand_,core_brand_,request_brand_;
    const void* const core_identity_;
    const std::size_t ordinal_;
    const std::string relative_ansi_;
    const std::u16string raw_stream_;
    const std::filesystem::path relative_,original_,canonical_;
    // Cold owns physical path derivation from its actual immutable roots. The
    // only caller is Root's private actual top-FindA producer, using its emitted
    // native basename and this genuine operation; no external filename entry.
    BnwNativeInstalledLateDefaultArchiveAttempt(const BnwNativeColdFilesystemOwner&,
        const BnwNativeInstalledLateDefaultOperation&,
        const ::civ5::ui::BnwNativePackageDiscoveryOwner& actual_existing_facade,
        const std::string& actual_find_basename);
    BnwNativeInstalledLateDefaultArchiveAttempt(const BnwNativeInstalledLateDefaultArchiveAttempt&)=delete;
    BnwNativeInstalledLateDefaultArchiveAttempt& operator=(const BnwNativeInstalledLateDefaultArchiveAttempt&)=delete;
    void require_active_request()const;
    void require_current()const;
    void require_same_input(const BnwNativeColdFilesystemOwner&)const;
    std::size_t source_ordinal()const;
    const std::string& raw_relative_ansi_request()const;
    const std::u16string& raw_utf16_stream_name_candidate()const;
    const std::filesystem::path& original_relative_io_token()const;
    const std::filesystem::path& original_absolute_io_candidate()const;
    const std::filesystem::path& canonical_payload_io_candidate()const;
    const std::shared_ptr<const void>& privatebrand_without_callbacks()const;
  public:
    ~BnwNativeInstalledLateDefaultArchiveAttempt()=default;
};
}
}
