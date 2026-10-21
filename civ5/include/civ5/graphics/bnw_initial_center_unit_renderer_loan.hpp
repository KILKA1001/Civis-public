#pragma once
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include <memory>

namespace civ5::runtime { class BnwUnitMovePending; }
namespace civ5::ui { class BnwClientSceneFrame; }
namespace civ5::validation { struct PreviewPresenter; }
namespace civ5::graphics {
class BnwInitialCenterUnitRendererLoan;

// Actual complete ordered plot-unit visibility stores, issued by the SAME
// renderer. No public mint, caller visibility list, model or full-Init grant.
class BnwInitialCenterUnitVisibilityProof final {
public:
    BnwInitialCenterUnitVisibilityProof(const BnwInitialCenterUnitVisibilityProof&)=delete;
    BnwInitialCenterUnitVisibilityProof& operator=(const BnwInitialCenterUnitVisibilityProof&)=delete;
private:
    friend class BnwInitialCenterUnitRendererLoan;
    friend class runtime::BnwNativeUnitAllocatorCore;
    struct State;
    const std::shared_ptr<const State> state_;
    explicit BnwInitialCenterUnitVisibilityProof(std::shared_ptr<const State>);
    void require_current_for_core(const runtime::BnwNativeUnitAllocatorCore&) const;
    std::shared_ptr<const BnwInitialCenterUnitVisibilityProof> retain_factory_owner() const;
    void require_for_plan(
        const runtime::BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan&) const;
};

// Stack-only initialized-renderer/complete-roster borrower. Existing pending
// false/position or after-present proofs cannot mint this separate endpoint.
class BnwInitialCenterUnitRendererLoan final {
public:
    BnwInitialCenterUnitRendererLoan(const BnwInitialCenterUnitRendererLoan&)=delete;
    BnwInitialCenterUnitRendererLoan& operator=(const BnwInitialCenterUnitRendererLoan&)=delete;
    BnwInitialCenterUnitRendererLoan(BnwInitialCenterUnitRendererLoan&&)=delete;
    BnwInitialCenterUnitRendererLoan& operator=(BnwInitialCenterUnitRendererLoan&&)=delete;
private:
    friend class runtime::BnwNativeUnitAllocatorCore;
    friend class BnwInitialCenterUnitVisibilityProof;
    friend class ::civ5::ui::BnwClientSceneFrame;
    friend struct ::civ5::validation::PreviewPresenter;
    struct State;
    State& state_;
    explicit BnwInitialCenterUnitRendererLoan(State& state):state_(state) {}
    void require_current_for_center(const runtime::BnwNativeUnitAllocatorCore&,
                                    const runtime::BnwUnitMovePending&) const;
    // Complete roster/receiver preflight BEFORE Core center/dirty stores.
    void preflight_plot_unit_visibility(
        const runtime::BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan&) const;
    std::shared_ptr<const BnwInitialCenterUnitVisibilityProof> apply_plot_unit_visibility(
        const runtime::BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan&) const;
    void require_plot_unit_visibility_current(
        const runtime::BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan&,
        const BnwInitialCenterUnitVisibilityProof&) const;
};
} // namespace civ5::graphics
