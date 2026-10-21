#pragma once
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include <array>

namespace civ5::ui {
// Non-owning ONE-construction origin, not a currentness verdict or public flag.
class BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin final {
    friend class BnwNativePackageDiscoveryOwner;
    friend struct BnwNativePackageDiscoveryOwner::Impl;
    friend class BnwNativeResourceBootSettingsOwner;
    friend class BnwNativeUserDirectoryOwner;
    BnwNativePackageDiscoveryOwner::Impl* const state_;
    const BnwNativePackageDiscoveryOwner* const parent_;
    const void* const loan_;
    const std::array<const void*,26> identities_;
    const std::weak_ptr<const BnwNativePackageDiscoveryOwner> factory_;
    const std::size_t rows_,packages_,descriptors_;
    mutable std::weak_ptr<const BnwNativeUserDirectoryOwner> directory_;
    mutable const BnwNativeUserDirectoryOwner* directory_identity_=nullptr;
    std::weak_ptr<const BnwNativeResourceBootSettingsOwner> boot_;
    const BnwNativeResourceBootSettingsOwner* boot_identity_=nullptr;
    mutable bool failed_=false;
    InstalledBootConstructionOrigin(const BnwNativePackageDiscoveryOwner&,const void* actual_registered_loan);
    ~InstalledBootConstructionOrigin()noexcept;
    InstalledBootConstructionOrigin(const InstalledBootConstructionOrigin&)=delete;
    InstalledBootConstructionOrigin&operator=(const InstalledBootConstructionOrigin&)=delete;
    InstalledBootConstructionOrigin(InstalledBootConstructionOrigin&&)=delete;
    void require_identity(const BnwNativePackageDiscoveryOwner&)const;
    void require_current(const BnwNativePackageDiscoveryOwner&)const;
    void record_actual_directory(const std::shared_ptr<const BnwNativeUserDirectoryOwner>&)const;
    void record_actual_boot(const std::shared_ptr<const BnwNativeResourceBootSettingsOwner>&);
    [[noreturn]]void refuse_ordinary_current()const;
};
}
