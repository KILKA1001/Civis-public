#pragma once
#include "civ5/ui/bnw_city_production_backend.hpp"
#include "civ5/runtime/bnw_selected_production_action_loan.hpp"
#include "civ5/runtime/bnw_city_production_turn.hpp"
#include <array>
#include <memory>
namespace civ5::ui { class BnwTrainRendererRefreshReceipt; class SelectedCityTrainCreateSource; class SelectedBnwReadOnlyPreview; class BnwClientSceneFrame; struct SelectedTrainInitializationObservation; }
namespace civ5::runtime { class BnwNativeUnitAllocatorCore; class BnwUnitMovePending; }
namespace civ5::runtime {
class BnwSelectedStockLuaSourceOwner;
class BnwNativeUnitAllocatorCore;
std::shared_ptr<const BnwCityProductionTurnPending> advance_selected_city_train_production(
    std::shared_ptr<BnwSelectedStockLuaSourceOwner>,std::shared_ptr<const ui::SelectedBnwPreviewHost>,
    std::int32_t owner,std::int32_t city,std::shared_ptr<const BnwCityProductionTurnPending> pending);
// Genuine SourceOwner implementation is an explicit integration dependency.
// It retains the SAME active VM/Host and advances only the exact registered
// process cursor, then validates its native delta before the private revision
// bookend. Direct calls to LiveSession outside that owner stale the VM loan.
std::shared_ptr<const BnwCityProductionTurnPending> advance_selected_city_process_production(
    std::shared_ptr<BnwSelectedStockLuaSourceOwner>,
    std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int32_t owner,std::int32_t city,
    std::shared_ptr<const BnwCityProductionTurnPending> pending);
// Genuine selected SourceOwner performs the private popOrder prefix, stages
// the actual Help flag, validates its world/VM revision delta, then returns.
// Its implementation is a supplier dependency, NOT a caller success callback.
ui::SelectedTrainInitializationObservation initialize_selected_completed_train_unit(
 std::shared_ptr<BnwSelectedStockLuaSourceOwner>,const ui::SelectedBnwReadOnlyPreview&,
 const std::shared_ptr<const ui::SelectedCityTrainCreateSource>&,
 const std::shared_ptr<const BnwNativeUnitAllocatorCore>&,std::shared_ptr<ui::BnwClientSceneFrame>);
SelectedProductionResult advance_selected_completed_train_tail(
    std::shared_ptr<BnwSelectedStockLuaSourceOwner>,
    const std::shared_ptr<const ui::SelectedCityTrainCreateSource>&);
void execute_selected_completed_train_prefix(
    std::shared_ptr<BnwSelectedStockLuaSourceOwner>,
    const std::shared_ptr<ui::SelectedCityTrainCreateSource>&);
// Reads the actual owner-issued Help queue/current source world; no supplied
// boolean, delivery receipt or GPU completion. Called before CreateUnit grant.
void require_selected_train_help_dirty(const BnwSelectedStockLuaSourceOwner&,
    const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
    std::int32_t owner,std::int32_t city,std::uint64_t occurrence,
    std::uint64_t prefix_revision);
}
namespace civ5::ui {
class SelectedBnwReadOnlyPreview;
class SelectedCityTrainCreateSource;

// Native observations only. This is not a can-order grant or a staged UI receipt.
struct SelectedMaintainPreflight {
    std::int32_t owner=-1, city=-1, process=-1, prerequisite=-1;
    bool native_eligible=false;
    std::array<std::int32_t,6> process_yields{}, before{}, after{};
    std::string blocker;
};
// Native observations only; hooks must run at their actual SDK phase.
struct SelectedConstructPreflight {
    std::int32_t owner=-1,city=-1,building=-1,building_class=-1;
    bool player_native_eligible=false,city_native_eligible=false;
    std::int32_t making_before=0,making_after=0,team_making=0,city_count=0;
    std::vector<std::int32_t> technologies;
    std::vector<std::int64_t> making_values;
    std::string blocker;
};
// Observations only; the stock issuer alone consumes the private action.
struct SelectedTrainPreflight {
    std::int32_t owner=-1,city=-1,unit=-1,unit_class=-1,unit_ai=-1;
    std::int32_t making_before=0,making_after=0,team_making=0;
    bool player_native_eligible=false,city_native_eligible=false;
    std::vector<std::int64_t> making_values,no_train_values,player_seal,city_seal;
    std::vector<std::int32_t> technologies;
    std::string blocker;
};
// The legacy generic services remain closed: their transaction swaps worlds.
// Only the new command methods/backend use the in-place stock loan.
// Owns the real parent handed off by SelectedBnwReadOnlyPreview, not a supplied
// session/rules pair. No VM is created here. The actual stock owner issues
// the narrow command loan and executes its own hooks/native UI publication.
// The old epoch diagnostic is independent of this per-command capability.
struct SelectedTrainInitializationObservation {
    // Actual retained objects, not readiness/ID authority. Retain Core through
    // subsequent real native phases; Source owns only its weak provenance.
    std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore> core;
    std::shared_ptr<const runtime::BnwUnitMovePending> pending;
    runtime::SelectedProductionResult result;
    // Real closed normal-return renderer handoff, never minted from result.
    std::shared_ptr<const BnwTrainRendererRefreshReceipt> renderer_refresh;
};
class SelectedCityProductionServices final {
  public:
    static std::shared_ptr<SelectedCityProductionServices> from_selected_preview(
        const SelectedBnwReadOnlyPreview&,
        std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> stock_owner={});
    ~SelectedCityProductionServices();
    SelectedCityProductionServices(const SelectedCityProductionServices&)=delete;
    SelectedCityProductionServices& operator=(const SelectedCityProductionServices&)=delete;
    SelectedMaintainPreflight native_maintain_preflight(
        std::int64_t owner,std::int64_t city,std::int64_t process)const;
    runtime::BnwProductionOrderServices production_services()const;
    BnwProductionPreview production_preview()const;
    // Actual command lane: supplier-issued SAME stock loan, no clone/swap.
    runtime::SelectedProductionResult preview_maintain_command(
        std::int64_t owner,std::int64_t city,std::int64_t process)const;
    runtime::SelectedProductionResult execute_maintain_command(
        std::int64_t owner,std::int64_t city,std::int64_t process)const;
    std::unique_ptr<BnwCityProductionUiBackend> make_backend(
        content::InfoLocalizer localize={})const;
    SelectedConstructPreflight native_construct_preflight(
        std::int64_t owner,std::int64_t city,std::int64_t building)const;
    runtime::SelectedProductionResult preview_construct_command(
        std::int64_t owner,std::int64_t city,std::int64_t building)const;
    runtime::SelectedProductionResult execute_construct_command(
        std::int64_t owner,std::int64_t city,std::int64_t building)const;
    // Uses the existing typed controller and existing source-owned backend.
    // Caller retains its genuine LiveUiHost/Scene frame and supplies that
    // frame snapshot; this does not fabricate a GUI action loan or mouse hit.
    UiCommandResult open_production_popup(UiScreenController&,const UiFrameSnapshot&,std::int64_t actual_city)const;
    UiCommandResult dispatch_city_choice(UiScreenController&,
        const UiGameplayCommand&,const UiFrameSnapshot&)const;
    // Source-owned imported MAINTAIN component only: actual doProcess prefix,
    // overflow0, feature0. Not a city-turn or doProduction predecessor receipt.
    // nullptr starts the genuine cursor; otherwise SAME registry identity is
    // enforced by the supplier/LiveSession. Each invocation advances once.
    std::shared_ptr<const runtime::BnwCityProductionTurnPending> advance_process_component(
        std::int64_t owner,std::int64_t city,
        std::shared_ptr<const runtime::BnwCityProductionTurnPending> pending={})const;
    // Bounded imported popOrder(0,true,true) prefix, not doProduction/turn.
    // Completion cost alone creates no allocator authority: actual prefix
    // stores + genuine Help flag must precede the opaque CreateUnit source.
    std::shared_ptr<const SelectedCityTrainCreateSource> advance_completed_train_component(
        std::int64_t owner,std::int64_t city)const;
    SelectedTrainPreflight native_train_preflight(std::int64_t owner,std::int64_t city,std::int64_t unit)const;
    runtime::SelectedProductionResult preview_train_command(std::int64_t owner,std::int64_t city,std::int64_t unit)const;
    runtime::SelectedProductionResult execute_train_command(std::int64_t owner,std::int64_t city,std::int64_t unit)const;
    std::shared_ptr<const runtime::BnwCityProductionTurnPending> advance_train_component(
        std::int64_t owner,std::int64_t city,
        std::shared_ptr<const runtime::BnwCityProductionTurnPending> pending={})const;
    runtime::SelectedProductionResult advance_completed_train_tail(const SelectedCityTrainCreateSource&)const;
    SelectedTrainInitializationObservation begin_completed_train_unit_initialization(
        const SelectedBnwReadOnlyPreview&,const SelectedCityTrainCreateSource&,std::shared_ptr<BnwClientSceneFrame>)const;
    std::string unavailable_reason()const;
  private:
    friend class SelectedCityTrainCreateSource;
    friend class runtime::BnwSelectedStockLuaSourceOwner;
    static std::shared_ptr<SelectedCityProductionServices> from_actual_source_owner(
        std::shared_ptr<const SelectedBnwPreviewHost>,runtime::LiveSession&,
        std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner>);
    struct Impl; std::shared_ptr<Impl> impl_;
    explicit SelectedCityProductionServices(std::shared_ptr<Impl>)noexcept;
    std::unique_ptr<SelectedTrainPreparedAction> prepare_train(std::int64_t owner,std::int64_t city,std::int64_t unit)const;
    runtime::SelectedProductionResult run_train(std::int64_t owner,std::int64_t city,std::int64_t unit,bool execute)const;
    std::unique_ptr<SelectedMaintainPreparedAction> prepare_maintain(
        std::int64_t owner,std::int64_t city,std::int64_t process)const;
    std::unique_ptr<SelectedConstructPreparedAction> prepare_construct(
        std::int64_t owner,std::int64_t city,std::int64_t building)const;
    runtime::SelectedProductionResult run_construct(
        std::int64_t owner,std::int64_t city,std::int64_t building,bool execute)const;
    runtime::SelectedProductionResult run_maintain(
        std::int64_t owner,std::int64_t city,std::int64_t process,bool execute)const;
};
}

namespace civ5::ui {
// Authority is private actual popOrder lineage. Public observations are not a
// completion flag or Unit DTO. The actual SourceOwner alone performs stores.
class SelectedCityTrainCreateSource final {
 public:
    ~SelectedCityTrainCreateSource();
    SelectedCityTrainCreateSource(const SelectedCityTrainCreateSource&)=delete;
    SelectedCityTrainCreateSource& operator=(const SelectedCityTrainCreateSource&)=delete;
 private:
    friend class SelectedCityProductionServices;
    friend class runtime::BnwSelectedStockLuaSourceOwner;
    friend class runtime::BnwNativeUnitAllocatorCore;
    struct Operands {std::int32_t owner=-1,city=-1,unit=-1,unit_ai=-1,x=-1,y=-1;};
    struct Impl;std::shared_ptr<Impl> impl_;
    std::weak_ptr<const SelectedCityTrainCreateSource> factory_self_;
    explicit SelectedCityTrainCreateSource(std::shared_ptr<Impl>)noexcept;
    std::shared_ptr<const SelectedCityTrainCreateSource> retain_factory_owner()const;
    void require_for_allocator(const SelectedBnwPreviewHost&,
        const runtime::LiveSession&,std::int32_t actor)const;
    Operands allocator_operands()const;
    void bind_allocator_once(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_unit_id_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_garrison_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_sight_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_plot_area_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_city_strength_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_happiness_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_happiness_suffix_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_happiness_return(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_name_game_counter_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_game_class_counter_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_team_class_counter_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_player_class_counter_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void accept_actual_initial_military_support_prefix(const runtime::BnwNativeUnitAllocatorCore&)const;
    void require_tail_source_owner(const runtime::BnwSelectedStockLuaSourceOwner&)const;
    // Genuine SourceOwner cannot dereference this opaque Impl in another TU.
    // Its private borrowed prefix is issued only from the actual prepared
    // source, not supplied State/IDs/ready. Caller must retain canonical Source.
    struct BorrowedPrefix {
        std::shared_ptr<const SelectedBnwPreviewHost> host;
        const runtime::LiveSession* session=nullptr;
        std::int32_t owner=-1,city=-1;
        std::uint64_t revision=0,help_occurrence=0;
    };
    struct BorrowedTail {
        std::shared_ptr<const SelectedBnwPreviewHost> host;
        std::int32_t owner=-1,city=-1,unit_id=-1;
        std::uint64_t revision=0;
    };
    BorrowedTail borrow_completed_create_unit_return()const;
    struct BorrowedInitialization {
        std::shared_ptr<const SelectedBnwPreviewHost> host;
        std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore> core;
        std::int32_t owner=-1,city=-1;std::uint64_t revision=0;
    };
    BorrowedInitialization borrow_bound_initialization(const std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore>&)const;
    bool city_trained_hook_pending()const;
    void begin_city_trained_hook()const;
    void accept_city_trained_hook()const;
    runtime::SelectedProductionResult apply_completion_native_stage(runtime::LiveSessionState&,bool actual_city_selected,std::uint64_t actual_generation)const;
    BorrowedPrefix borrow_issuer_prefix()const;
    void require_current_native()const;
    void apply_pop_order_prefix(runtime::LiveSessionState&)const;
    void accept_actual_help_dirty(const runtime::BnwSelectedStockLuaSourceOwner&)const;
};
}
