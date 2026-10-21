#pragma once
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"

namespace civ5::ui {
// Native823E90/constructor6BDE0B directory identity, separate from any INI
// parser and material startup. Missing directories remain missing: no writes.
class BnwNativeUserDirectoryOwner {
  public:
    static std::shared_ptr<const BnwNativeUserDirectoryOwner> create(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner>);
    ~BnwNativeUserDirectoryOwner();
    BnwNativeUserDirectoryOwner(const BnwNativeUserDirectoryOwner&) = delete;
    BnwNativeUserDirectoryOwner& operator=(const BnwNativeUserDirectoryOwner&) = delete;
    BnwNativeUserDirectoryOwner(BnwNativeUserDirectoryOwner&&) = delete;
    BnwNativeUserDirectoryOwner& operator=(BnwNativeUserDirectoryOwner&&) = delete;
    const std::filesystem::path& current() const;
    void require_same_resources(const BnwNativePackageDiscoveryOwner&) const;
  private:
    friend class BnwNativeResourceBootSettingsOwner;
    static std::shared_ptr<const BnwNativeUserDirectoryOwner> create_from_installation_boot_source(const BnwNativePackageDiscoveryOwner&);
    static std::shared_ptr<const BnwNativeUserDirectoryOwner> create_from_installation_boot_source(
        const BnwNativePackageDiscoveryOwner&,const BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin&);
    const std::filesystem::path& current_for_installation_boot(
        const BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin&)const;
    friend class BnwNativePackageDiscoveryOwner;
    friend class BnwAudioUserSettingsInput;
    friend class BnwAudioLiveKnobOwner;
    std::shared_ptr<const BnwNativeUserDirectoryOwner> retain_factory_owner(
        const std::shared_ptr<const BnwNativeUserDirectoryOwner>&) const;
    void require_parent_loan_creator_thread() const;
    void require_parent_loan_identity_without_callbacks(
        const BnwNativePackageDiscoveryOwner& actual,
        const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& actual_factory,
        const assets::ResourceMountTable& actual_table) const;
    void require_parent_loan_source_without_callbacks(
        const BnwNativePackageDiscoveryOwner& actual,
        const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& actual_factory,
        const assets::ResourceMountTable& actual_table) const;
    void validate_source() const;
    const std::filesystem::path& source_directory_without_parent_callbacks()const;
    struct Impl;
    struct SelfPin;
    struct Operation;
    std::shared_ptr<Impl> impl_;
    explicit BnwNativeUserDirectoryOwner(std::shared_ptr<Impl>);
};
}
