#pragma once
#include "civ5/content/manifest.hpp"
#include <filesystem>
#include <memory>
namespace civ5::ui {
class BnwNativeInstallationInputOwner;
class BnwNativePackageDiscoveryOwner;
// SAME retained installation files, issued privately by Parent AFTER its
// genuine whole-Skin normal return. This never carries a save/header/profile.
// Initial-window validation remains in the pre-handoff source owner.
class BnwNativePinnedInstallationEpoch final {
 public:
 ~BnwNativePinnedInstallationEpoch();
 BnwNativePinnedInstallationEpoch(const BnwNativePinnedInstallationEpoch&)=delete;
 BnwNativePinnedInstallationEpoch&operator=(const BnwNativePinnedInstallationEpoch&)=delete;
 void require_current()const;
 private:
 friend class BnwNativeInstallationInputOwner;
 friend class BnwNativePackageDiscoveryOwner;
 void require_same_installation_input(const BnwNativeInstallationInputOwner&)const;
 const std::filesystem::path& game_root()const;
 const std::filesystem::path& canonical_payload_root()const;
 const content::Catalog& catalog()const;
 struct Impl;const std::shared_ptr<const Impl> impl_;
 explicit BnwNativePinnedInstallationEpoch(std::shared_ptr<const Impl>);
};
}
