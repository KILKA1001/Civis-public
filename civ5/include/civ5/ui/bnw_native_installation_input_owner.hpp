#pragma once
#include "civ5/content/manifest.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_stock_lua_client_extent.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>
namespace civ5::ui {
namespace detail {class BnwNativeFilesystemMappingOwner;class BnwNativeColdFilesystemOwner;class BnwNativeOriginalStatService;
class BnwNativeCategoryArchiveChannel;class BnwNativeCategoryLooseAdmissionSource;
template<class> class BnwNativeCallbackOwnerGuard;}
class BnwNativePackageDiscoveryOwner;
class BnwNativePinnedInstallationEpoch;
struct BnwNativeInstallationSourceObservation {
 std::filesystem::path installation,canonical_assets;
 std::vector<runtime::FileIdentity> pinned_files;
 runtime::BnwStockLuaClientExtent client;
};
// Genuine installation/client input BEFORE selection of a game or save.
// No SaveHeader, saved profile, selected DLC list, or ready receipt is accepted.
// Parent/Boot may consume this only through their closed Root-owned join.
class BnwNativeInstallationInputOwner final {
 public:
 static std::shared_ptr<const BnwNativeInstallationInputOwner> acquire(
  const std::filesystem::path& installation,const std::filesystem::path& canonical_assets,
  std::uintptr_t actual_owned_hwnd,runtime::BnwStockLuaClientExtentProvider);
 ~BnwNativeInstallationInputOwner();
 BnwNativeInstallationInputOwner(const BnwNativeInstallationInputOwner&)=delete;
 BnwNativeInstallationInputOwner&operator=(const BnwNativeInstallationInputOwner&)=delete;
 void require_current()const;
 BnwNativeInstallationSourceObservation observe()const;
 private:
 friend class BnwNativePackageDiscoveryOwner;
 friend class detail::BnwNativeColdFilesystemOwner;
 friend class detail::BnwNativeFilesystemMappingOwner;
 friend class detail::BnwNativeOriginalStatService;
 friend class detail::BnwNativeCategoryArchiveChannel;
 friend class detail::BnwNativeCategoryLooseAdmissionSource;
 friend class BnwNativePinnedInstallationEpoch;
 // Read-only test observation of the actual stack borrower; never authority.
 friend bool bnw_native_retained_primary32_borrower_active_for_test(
  const BnwNativeInstallationInputOwner&)noexcept;
 // Root-only live borrower for ONE actual admission/retained validation.
 // Each closed issuer keeps strict Input PRE/POST before operation return/adoption.
 // Admitted HANDLEs stay alive; unrelated held metadata is sampled at those
 // boundaries. Live client/owner/fence/reentry checks stay inside every current.
 // References only: no extra Input/Parent pin or cached validation success.
 class RetainedPrimary32Validation final {
  friend class BnwNativePackageDiscoveryOwner;
  using ParentFence=detail::BnwNativeCallbackOwnerGuard<BnwNativePackageDiscoveryOwner>;
  using Controller=void(*)(const BnwNativePackageDiscoveryOwner&,const ParentFence&,
                           const BnwNativeInstallationInputOwner&);
  const BnwNativeInstallationInputOwner& input_;
  const BnwNativePackageDiscoveryOwner& parent_;
  const ParentFence& fence_;
  const Controller controller_;
  RetainedPrimary32Validation(const BnwNativeInstallationInputOwner&,
   const BnwNativePackageDiscoveryOwner&,const ParentFence&,Controller);
  ~RetainedPrimary32Validation()noexcept;
  RetainedPrimary32Validation(const RetainedPrimary32Validation&)=delete;
  RetainedPrimary32Validation&operator=(const RetainedPrimary32Validation&)=delete;
 public:
  void require_current()const;
 };
 // Root.current is forbidden during the retained interval, even if caught.
 void refuse_parent_current_during_retained_primary32_validation()const;
 // Parent invokes this ONLY at its actual whole-Skin post-return handoff,
 // after strict initial source effects. It is not public authority to relax
 // the window epoch or manufacture a normal-return receipt.
 std::shared_ptr<const BnwNativePinnedInstallationEpoch>
 capture_pinned_installation_epoch_after_parent_return()const;
 // Cold only, immediately between its real Installation FULL bookends.
 void require_cold_roots_identity_without_callbacks(
  const std::filesystem::path& original_root,const std::filesystem::path& canonical_root)const;
 const std::filesystem::path& game_root()const;
 const std::filesystem::path& canonical_payload_root()const;
 const content::Catalog& catalog()const;
 std::uintptr_t actual_hwnd()const;
 const runtime::BnwStockLuaClientExtentProvider& client_provider()const;
 struct Impl;const std::shared_ptr<Impl> impl_;
 explicit BnwNativeInstallationInputOwner(std::shared_ptr<Impl>);
};
}
