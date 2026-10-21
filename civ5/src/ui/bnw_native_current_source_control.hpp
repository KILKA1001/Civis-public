#pragma once
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "bnw_native_archive_v6_entry_admission.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include <thread>
namespace civ5::ui::detail {
class BnwNativeCurrentDefaultSource final {
  public:
    ~BnwNativeCurrentDefaultSource()=default;
    BnwNativeCurrentDefaultSource(const BnwNativeCurrentDefaultSource&)=delete;
    BnwNativeCurrentDefaultSource& operator=(const BnwNativeCurrentDefaultSource&)=delete;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeGlobalColorRegistryBank;
    using Parent=::civ5::ui::BnwNativePackageDiscoveryOwner;
    mutable BnwNativeCallbackOwnerState state_;
    std::weak_ptr<const BnwNativeCurrentDefaultSource> self_;
    std::weak_ptr<const Parent> parent_;
    std::shared_ptr<const void> epoch_;
    std::shared_ptr<const BnwNativeColdFilesystemOwner> cold_;
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena_;
    std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> cache_;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease_;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input_;
    const assets::ResourceMountTable* table_=nullptr;
    const void* default_state_=nullptr;
    mutable const void* actual_record_storage_=nullptr;
    std::thread::id thread_=std::this_thread::get_id();
    // Intrinsic source byte from the actual fresh assignment producer, not a
    // copied constructor receipt/current argument. No toggle issuer exists.
    const std::uint8_t current_byte_=1;
    const std::string native_label_="Base";
    std::vector<std::weak_ptr<const BnwNativeCurrentDefaultSource>> closed_registry_;
    mutable bool failed_=false;
    BnwNativeCurrentDefaultSource()=default;
    void require_current(const Parent&)const;
    // Body belongs to the genuine Parent completed Common producer. This
    // declaration issues no current namespace/record or completed source loan.
    void require_current_completed(const Parent&,const BnwNativeCompletedColdSourceLoan&,
        const BnwNativeColdFilesystemOwner&)const;
};
// Private node-selection result only. No payload/stream/flag getters/factory.
// Root authenticates actual record+handle nodes before issuing or reusing it.
class BnwNativeCurrentDefaultSelection final {
  public:
    ~BnwNativeCurrentDefaultSelection()=default;
    BnwNativeCurrentDefaultSelection(const BnwNativeCurrentDefaultSelection&)=delete;
    BnwNativeCurrentDefaultSelection& operator=(const BnwNativeCurrentDefaultSelection&)=delete;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    BnwNativeCurrentDefaultSelection()=default;
    std::weak_ptr<const BnwNativeCurrentDefaultSource> source_;
    std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent_;
    std::shared_ptr<const void> epoch_;
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena_;
    std::u16string owned_request_;
    std::uint32_t crc_=0;
    const BnwNativeArchiveRecordDiagnostic* actual_record_=nullptr;
    const assets::AssetHandle* actual_handle_=nullptr;
    std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix_;
    std::shared_ptr<const BnwNativeArchiveCachePublicationOwner> archive_;
    const BnwNativeCurrentDefaultSource* written_out_source_=nullptr;
};
}
