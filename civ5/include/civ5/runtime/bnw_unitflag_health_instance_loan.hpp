#pragma once
#include "civ5/ui/bnw_worldview_control_effects.hpp"
#include <memory>
#include <thread>
#include <utility>
#include <cstdint>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
struct LiveSessionState;
class BnwSelectedWorldViewContextOwner;
class LuaScriptHost;
class BnwSelectedStockLuaBootstrap;
class BnwSelectedUnitFlagContextOwner;
class BnwWorldViewSourceOperationLoan;
class BnwStockLuaAttemptLease;
class ScriptUnitReadReference;
enum class BnwUnitFlagHealthCommand { hide,fg_color,percent,offset_x };
// This reference is issued by the actual VM only after the original full-ID
// UnitFlag constructor and its protected caller return. No public ID/DTO
// publisher, named-control list, saved flag or listener count can construct it.
// Lifetime-only lending does not authorize geometry, input, layout or a frame.
class BnwUnitFlagHealthInstanceLoan final {
 public:
    BnwUnitFlagHealthInstanceLoan(const BnwUnitFlagHealthInstanceLoan&)=delete;
    BnwUnitFlagHealthInstanceLoan& operator=(const BnwUnitFlagHealthInstanceLoan&)=delete;
    BnwUnitFlagHealthInstanceLoan(BnwUnitFlagHealthInstanceLoan&&)=delete;
    BnwUnitFlagHealthInstanceLoan& operator=(BnwUnitFlagHealthInstanceLoan&&)=delete;
 private:
    friend class LuaScriptHost;
    friend class BnwSelectedStockLuaBootstrap;
    friend class BnwWorldViewSourceOperationLoan;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct OriginalReturnedInstance;
    struct State;
    class LivingUnitReference;
    static std::pair<std::int32_t,std::int32_t> owner_full_id_from_storage_without_callbacks(const std::shared_ptr<State>&);
    static std::shared_ptr<const ScriptUnitReadReference> retain_living_unit_reference_from_storage_without_callbacks(const std::shared_ptr<State>&);
    const std::thread::id creator_=std::this_thread::get_id();
    const std::shared_ptr<State> state_;
    BnwUnitFlagHealthInstanceLoan(const BnwStockLuaAttemptLease&,const LuaScriptHost&,
        const BnwSelectedUnitFlagContextOwner&,const OriginalReturnedInstance&);
    ~BnwUnitFlagHealthInstanceLoan();
    // Saved storage is authenticated before capture. State retains its own
    // atomic facade-alive carrier: destructor retires it, and static bridges
    // test creator/alive FIRST before any saved borrower or callback. No bridge
    // may dereference this destroyed Loan or a prior stack/current-source token.
    std::shared_ptr<State> retain_storage_without_callbacks()const;
    static void require_origin_from_storage_without_callbacks(const std::shared_ptr<State>&);
    static void require_current_from_storage_owned(const std::shared_ptr<State>&);
    // Source ctor alone binds after its actual lend_operation_scope returns.
    // Stockserial belongs to the fresh counted Stock identity; metadata serial
    // belongs to the ONE actual SourceOwner active slot. Never use the not-yet-
    // entered VM NativeHostScope or dereference an old Source stack pointer.
    static void bind_source_scope_from_storage_without_callbacks(
        const std::shared_ptr<State>&,const BnwWorldViewSourceOperationLoan&,
        std::uint64_t metadata_scope_serial,std::uint64_t stock_operation_serial);
    static void require_source_origin_from_storage_without_callbacks(
        const std::shared_ptr<State>&,const LuaScriptHost&,
        const BnwSelectedWorldViewContextOwner&,
        const std::weak_ptr<const ui::SelectedBnwPreviewHost>&,
        const std::weak_ptr<const LiveSessionState>&,std::uint64_t);
    static void require_control_from_storage_without_callbacks(
        const std::shared_ptr<State>&,const ui::BnwWorldViewControlTree::Control&,
        BnwUnitFlagHealthCommand);
    // PRE-MINT: authenticate actual fresh Stock ScopeIdentity, VM lifetime and
    // retained Original constructor occurrence; do not fetch a nonexistent
    // current_operation_loan or substitute old Tree require_current.
    void require_origin_without_callbacks()const;
    // POST-MINT: SAME actual metadata active Source loan/current serial plus
    // current full-ID native unit and all original membership seals.
    void require_current()const;
    // Exact current canonical Control membership, no rectangle/data grants.
    // hide: BG/Bar/BarButton/NormalButton/NormalSelect/HealthBarSelect;
    // fg_color+percent: HealthBar only; offset_x: genuine optional CargoPullDown.
    void require_control_without_callbacks(const ui::BnwWorldViewControlTree::Control&,
        BnwUnitFlagHealthCommand)const;
};
}

