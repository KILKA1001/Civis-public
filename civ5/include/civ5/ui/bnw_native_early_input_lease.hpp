#pragma once
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/runtime/bnw_stock_lua_client_extent.hpp"
#include <memory>

namespace civ5::ui {
namespace detail {class BnwNativeCommonSourceValidationContext; class BnwNativeColdFilesystemOwner; class BnwNativeOriginalStatService; class BnwNativeArchiveConstruction; class BnwNativeArchiveV6StreamHeaderOwner; class BnwNativeCommon13ArchiveChannel; class BnwNativeDefaultNamespacePolicyOwner;}
class BnwNativePackageDiscoveryOwner;
class BnwNativePinnedInputEpoch;
// Owns actual input and client before SQL/Color creation. This capability does
// not certify package registration, art compilation, adoption or game readiness.
class BnwNativeEarlyInputLease final {
  public:
    static std::shared_ptr<const BnwNativeEarlyInputLease> acquire(
        const std::filesystem::path& selected_save,
        const std::filesystem::path& original_game_root,
        const std::filesystem::path& canonical_payload_root,
        std::uintptr_t actual_owned_hwnd,
        runtime::BnwStockLuaClientExtentProvider);
    ~BnwNativeEarlyInputLease();
    BnwNativeEarlyInputLease(const BnwNativeEarlyInputLease&) = delete;
    BnwNativeEarlyInputLease& operator=(const BnwNativeEarlyInputLease&) = delete;
    // Pure factory control/lifetime authentication before callbackful use.
    std::shared_ptr<const BnwNativeEarlyInputLease> retain_owner() const;
    void require_current() const;
    const saves::SaveHeader& selected_header() const;
    const content::Catalog& catalog() const;
    const runtime::SavedContentProfile& profile() const;
    const std::filesystem::path& save_path() const;
    const std::filesystem::path& game_root() const;
    const std::filesystem::path& canonical_payload_root() const;
    std::uintptr_t actual_hwnd() const;
    const runtime::BnwStockLuaClientExtentProvider& client_provider() const;
    const runtime::BnwStockLuaClientExtent& client_extent() const;
  private:
    friend class BnwNativePackageDiscoveryOwner;
    friend class BnwNativePinnedInputEpoch;
    // Only genuine Parent may bind this file-only input after full handoff.
    std::shared_ptr<const BnwNativePinnedInputEpoch> capture_pinned_input_epoch()const;
    friend class detail::BnwNativeColdFilesystemOwner;
    // Cold keeps FULL PRE/POST; this void comparison exposes no source/currentness.
    void require_cold_roots_profile_identity_without_callbacks(
        const std::filesystem::path&,const std::filesystem::path&)const;
    friend class detail::BnwNativeOriginalStatService;
    friend class detail::BnwNativeArchiveConstruction;
    friend class detail::BnwNativeArchiveV6StreamHeaderOwner;
    friend class detail::BnwNativeCommon13ArchiveChannel;
    friend class detail::BnwNativeDefaultNamespacePolicyOwner;
    void require_common_source_current(detail::BnwNativeCommonSourceValidationContext&)const;
    const std::filesystem::path& game_root_common_source(detail::BnwNativeCommonSourceValidationContext&)const;
    const std::filesystem::path& canonical_payload_root_common_source(detail::BnwNativeCommonSourceValidationContext&)const;
    const runtime::SavedContentProfile& profile_common_source(detail::BnwNativeCommonSourceValidationContext&)const;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit BnwNativeEarlyInputLease(std::shared_ptr<Impl>);
};
}
