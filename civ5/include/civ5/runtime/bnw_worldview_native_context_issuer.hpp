#pragma once
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include "civ5/ui/bnw_worldview_control_effects.hpp"
#include <array>
namespace civ5::runtime {
// Actual native XML context construction is separate from executing its Lua.
// This borrower neither claims VM loading completion nor issues presentation.
class BnwWorldViewNativeContextIssuer final {
 public:
    BnwWorldViewNativeContextIssuer(const BnwWorldViewNativeContextIssuer&)=delete;
    BnwWorldViewNativeContextIssuer& operator=(const BnwWorldViewNativeContextIssuer&)=delete;
    void require_current()const;
    const BnwWorldViewSourceOperationLoan& operation()const;
 private:
    friend class BnwSelectedStockLuaBootstrap;
    friend class BnwWorldViewScriptUiAdapter;
    // REQUIRED genuine Root implementation, deliberately not a linker stub.
    // Only authentic initialized canonical state may mint the private Loan.
    enum class LayoutControlCommand {calculate_size,reprocess_anchoring,set_size_y,calculate_internal_size};
    static void dispatch_layout_control_owned(ui::BnwWorldViewControlTree&,
        const ui::BnwWorldViewControlTree::Control&,const BnwWorldViewSourceOperationLoan&,
        LayoutControlCommand,double actual_scalar);
    friend class ::civ5::ui::UiScreenGlRenderer;
    static void advance_scroll_updates_owned(const BnwWorldViewSourceOperationLoan&,
        const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>&,const ui::BnwScrollAnimationUpdateLoan&);
    const std::shared_ptr<BnwSelectedWorldViewContextOwner> owner_;
    BnwWorldViewSourceOperationLoan loan_;
    const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5> trees_;
    BnwWorldViewNativeContextIssuer(std::shared_ptr<BnwSelectedWorldViewContextOwner>,
        const BnwStockLuaAttemptLease&,const BnwSelectedStockLuaBootstrap&,const LuaScriptHost&,
        const ui::BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
      const ui::BnwWorldViewGridToWorldLoan&,
        std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>& actual_native_slots);
    // Existing slots are borrowed only inside the actual Bootstrap scope.
    const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>* const lifecycle_slots_=nullptr;
    BnwWorldViewNativeContextIssuer(std::shared_ptr<BnwSelectedWorldViewContextOwner>,
        const BnwStockLuaAttemptLease&,const BnwSelectedStockLuaBootstrap&,const LuaScriptHost&,
        const ui::BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan&,
        const BnwUnitFlagHealthInstanceLoan&,
        const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>& actual_native_slots);
    void require_lifecycle_slot_owned()const;
    ~BnwWorldViewNativeContextIssuer();
    std::shared_ptr<const ScriptUiChildSource> source(std::string_view)const;
    std::shared_ptr<ui::BnwWorldViewControlTree> tree(std::string_view)const;
    static std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5> construct_contexts(
        const std::shared_ptr<BnwSelectedWorldViewContextOwner>&,
        const BnwWorldViewSourceOperationLoan&,
        std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>&);
};
}
