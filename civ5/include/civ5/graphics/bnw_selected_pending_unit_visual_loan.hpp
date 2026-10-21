#pragma once
#include <functional>
#include <memory>

namespace civ5::runtime {
class BnwNativeUnitAllocatorCore;
class BnwUnitMovePending;
class BnwStockLuaAttemptLease;
class BnwSelectedStockLuaSourceOwner;
}
namespace civ5::ui { class BnwClientSceneFrame; class BnwOwnedCameraGuiScreen; }
namespace civ5::graphics {
class BnwPendingUnitVisualOwner;

// Issued only by the actual selected Scene inside its synchronous borrower.
// This loan never attests a post-store frame, GUI refresh or presentation.
class BnwSelectedPendingUnitVisualLoan final {
  public:
    BnwSelectedPendingUnitVisualLoan(const BnwSelectedPendingUnitVisualLoan&)=delete;
    BnwSelectedPendingUnitVisualLoan& operator=(const BnwSelectedPendingUnitVisualLoan&)=delete;
    BnwSelectedPendingUnitVisualLoan(BnwSelectedPendingUnitVisualLoan&&)=delete;
    BnwSelectedPendingUnitVisualLoan& operator=(BnwSelectedPendingUnitVisualLoan&&)=delete;
    void require_pre_store_current() const;
    // Genuine Core20 opening after constructor storage, before native SetID.
    void bind_pre_assignment_core(const runtime::BnwNativeUnitAllocatorCore&) const;
    const std::shared_ptr<BnwPendingUnitVisualOwner>& canonical_receiver() const;
    // Accepts only Core17's actual stored hidden-slot proof and exact earned
    // Pending5 garrison successor. Never checks stale pre-store GUI/frame.
    void require_hidden_slot_proof(const runtime::BnwNativeUnitAllocatorCore&,
                                   const runtime::BnwUnitMovePending&) const;
  private:
    friend class ::civ5::ui::BnwClientSceneFrame;
    struct State;
    explicit BnwSelectedPendingUnitVisualLoan(State& state):state_(state) {}
    State& state_;
};
} // namespace civ5::graphics
