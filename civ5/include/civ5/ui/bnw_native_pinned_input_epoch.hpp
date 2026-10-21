#pragma once
#include "civ5/ui/bnw_native_early_input_lease.hpp"
namespace civ5::ui {
namespace detail {class BnwNativeCompletedColdSourceLoan;}
// Same immutable source data and ORIGINAL read-only file handles, captured
// while an authentic EarlyInputLease is current. This capability attests file
// inputs only: it grants no completed registration, UI, HWND, drawable extent,
// activation history, audio MASTER or rendering readiness. The genuine Parent
// must bind it at its successful startup handoff and keep independent window /
// publication currentness checks in all actual consumers.
class BnwNativePinnedInputEpoch final {
    friend class BnwNativeEarlyInputLease;
    friend class BnwNativePackageDiscoveryOwner;
    friend class detail::BnwNativeCompletedColdSourceLoan;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit BnwNativePinnedInputEpoch(std::shared_ptr<const Impl>);
    void require_same_early_input(const BnwNativeEarlyInputLease&)const;
  public:
    ~BnwNativePinnedInputEpoch();
    BnwNativePinnedInputEpoch(const BnwNativePinnedInputEpoch&)=delete;
    BnwNativePinnedInputEpoch& operator=(const BnwNativePinnedInputEpoch&)=delete;
    void require_current()const;
    const saves::SaveHeader& selected_header()const;
    const content::Catalog& catalog()const;
    const runtime::SavedContentProfile& profile()const;
    const std::filesystem::path& save_path()const;
    const std::filesystem::path& game_root()const;
    const std::filesystem::path& canonical_payload_root()const;
};
} // namespace civ5::ui
