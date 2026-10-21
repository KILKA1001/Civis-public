#include "civ5/ui/bnw_worldview_window_input_loan.hpp"
namespace civ5::ui {class BnwEngineUiTurnOwner;}
namespace civ5::runtime {class BnwOwnedGameplayEventPumpLoan;}
namespace civ5::ui {class BnwTechAwardInputDispatch;}
#pragma once
#include "civ5/runtime/bnw_worldview_native_context_issuer.hpp"
#include "civ5/graphics/bnw_unit_damage_command_owner.hpp"
#include <array>
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/bnw_stock_lua_client_extent.hpp"
#include "civ5/ui/bnw_civilopedia_scrollbar_owner.hpp"
#include <exception>
namespace civ5::ui { class BnwOwnedCameraGuiScreen; struct BnwRightTreeFrame; struct BnwCivilopediaNativeRuntimeFrame; }
namespace civ5::ui {class SelectedCityTrainCreateSource;}
namespace civ5::runtime {
class BnwCivisOwnedStockSourceBundle;
class BnwTreasurySameVmIncomeLoan;
class BnwNativeUnitAllocatorCore;
enum class BnwResearchChoiceEvent;
enum class BnwSelectedProductionLuaEvent;
// Historical bounded rectangle observations cannot grant native setter parity.
enum class BnwStockScrollOperationKind {set_size,set_offset};
struct BnwStockScrollOperation {
    std::string control;
    BnwStockScrollOperationKind kind=BnwStockScrollOperationKind::set_size;
    std::int64_t x=0,y=0;
};
struct BnwStockLuaBootstrapProgress {
    bool selected_sources_verified=false, first_prefix_executed=false;
    bool loading_complete=false, gameplay_callbacks_ready=false;
    std::uint64_t prefix_executions=0, manager_bindings=0, invalidation_listeners=0;
    bool load_wb_scenario=false;
    std::uint64_t native_screen_queries=0,native_scrollbar_queries=0,native_panel_queries=0,native_scrollbar_resizes=0,client_extent_revision=0;
    std::int64_t screen_width=0,screen_height=0,scrollbar_width=0,scrollbar_height=0;
    std::int64_t panel_width=0,panel_height=0,resized_scrollbar_width=0,resized_scrollbar_height=0;
    std::uint64_t native_left_panel_queries=0,native_sibling_mutations=0;
    std::uint64_t initial_content_creations=0;
    std::uint64_t staged_right_controls=0,staged_right_literal_metrics=0;
    std::string initial_native_dependency;
    std::int64_t left_panel_width=0,left_panel_height=0;
    std::vector<BnwStockScrollOperation> sibling_operations;
    std::string client_extent_owner;
    std::string context, pending_api, parent_sha256, declaration_sha256, xml_sha256, blocker;
    std::vector<std::string> includes_executed;
};
// Persistent staged stock VM. The exact prefix runs once; the remaining parent
// source stays on a suspended coroutine retaining its original local variables.
// Unsupported UI services pause BEFORE their statement, not after a Lua error.
// Neither staged registry nor partial IncludeFileList can authorize callbacks.
// This bounded owner has no epoch export/publish or complete_loading path.
class BnwSelectedStockLuaBootstrap {
 public:
    BnwSelectedStockLuaBootstrap(std::filesystem::path shared_asset_root,
        const content::Catalog &,const SavedContentProfile &,
        const SessionCheckpointIdentity &,const LiveSessionState &,
        const content::RulesDatabase &);
    BnwSelectedStockLuaBootstrap(std::filesystem::path shared_asset_root,
        const content::Catalog &,const SavedContentProfile &,
        const SessionCheckpointIdentity &,const LiveSessionState &,
        const content::RulesDatabase &,ScriptMapProvider);
    ~BnwSelectedStockLuaBootstrap();
    BnwSelectedStockLuaBootstrap(const BnwSelectedStockLuaBootstrap &)=delete;
    BnwSelectedStockLuaBootstrap &operator=(const BnwSelectedStockLuaBootstrap &)=delete;
    bool attach_client_extent(BnwStockLuaClientExtentProvider);
    const BnwStockLuaBootstrapProgress &advance();
    const BnwStockLuaBootstrapProgress &progress()const;
    // Retained created rectangles for rendering; pointer is owned by this bootstrap.
    const ui::BnwScrollBarLayoutFrame *scrollbar_frame()const;
    // Retained partial initialization evidence on the same VM/shell epoch;
    // not a complete native subtree or a permission to execute its setter.
    const ui::BnwRightTreeFrame *right_tree_frame()const;
 private:
    friend class BnwTreasurySameVmIncomeLoan;
    friend class BnwSelectedStockLuaSourceOwner;
    friend class BnwWorldViewSourceOperationLoan;
    void require_modding_origin_owned(const BnwWorldViewSourceOperationLoan&,const LuaScriptHost&)const;
    std::shared_ptr<ScriptQuery> modding_activated_entrypoint_query_owned(
        const BnwWorldViewSourceOperationLoan&,const LuaScriptHost&,std::string_view)const;
    void require_modding_activated_entrypoint_query_owned(
        const BnwWorldViewSourceOperationLoan&,const LuaScriptHost&,const std::shared_ptr<ScriptQuery>&)const;
    void bind_worldview_sources_before_context_owned(const BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<ui::CivisInputPolicyOwner> ensure_context_input_policy_before_load_owned(
        const BnwWorldViewSourceOperationLoan&)const;
    friend class ::civ5::ui::UiScreenGlRenderer;
    std::pair<const void*,std::uint64_t> require_worldview_vm_origin_without_callbacks(
        const BnwSelectedWorldViewContextOwner&,const LuaScriptHost&,
        const BnwStockLuaAttemptLease&,const BnwWorldViewSourceOperationLoan&)const;
    enum class WorldViewCallbackPurpose {render,initial_lua};
    void with_worldview_context_owned(const ui::BnwOwnedCameraGuiScreen&,const ui::BnwWorldViewGridToWorldLoan&,
        const std::function<void(const BnwWorldViewSourceOperationLoan&,
            const ui::BnwOwnedCameraGuiScreen::Loan&,
            const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
            const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>&)>&,
        WorldViewCallbackPurpose=WorldViewCallbackPurpose::render);
    void activate_worldview_context_owned(const ui::BnwOwnedCameraGuiScreen&,const ui::BnwWorldViewGridToWorldLoan&);
    friend class ui::BnwEngineUiTurnOwner;
    void drain_gameplay_events_owned(const BnwOwnedGameplayEventPumpLoan&,const BnwStockLuaAttemptLease&);
    bool try_dispatch_unitflag_health_head_owned(
        const std::weak_ptr<graphics::BnwUnitDamageCommandOwner>&,
        const BnwOwnedGameplayEventPumpLoan&,const BnwStockLuaAttemptLease&,
        const BnwSelectedStockLuaSourceOwner&);
    void dispatch_unitflag_health_head_owned(
        const std::weak_ptr<graphics::BnwUnitDamageCommandOwner>&,
        const BnwOwnedGameplayEventPumpLoan&,const BnwStockLuaAttemptLease&,
        const BnwSelectedStockLuaSourceOwner&);
    // Production-only: the SourceOwner passes its SAME retained actual lease.
    BnwSelectedStockLuaBootstrap(std::filesystem::path,const content::Catalog&,
        const SavedContentProfile&,const SessionCheckpointIdentity&,
        std::shared_ptr<const LiveSessionState>,const content::RulesDatabase&,
        ScriptMapProvider,BnwStockLuaAttemptLease,
        std::shared_ptr<content::RulesDatabase> saved_database);
    BnwSelectedStockLuaBootstrap(std::shared_ptr<const BnwCivisOwnedStockSourceBundle>,
        const SessionCheckpointIdentity&,std::shared_ptr<const LiveSessionState>,
        const content::RulesDatabase&,ScriptMapProvider,BnwStockLuaAttemptLease,
        std::shared_ptr<content::RulesDatabase> saved_database);
    struct OwnedOutcome {
        BnwStockLuaOperationOutcome outcome;
        const BnwStockLuaBootstrapProgress* progress;
        std::exception_ptr error;
    };
    // Root declaration proposal only. GRAPHICS owns the actual retained-VM
    // UnitFlagManager receiver implementation; no stub definition is supplied.
    void dispatch_same_unit_dim_owned(const BnwUnitMovePending&,const BnwStockLuaAttemptLease&);
    OwnedOutcome advance_owned();
    void require_prepared_owned()const;
    void require_healthy_owned()const;
    // Callback-free actual host/failure stamp; intentionally does not borrow
    // the operation loan (avoids currentness recursion).
    void require_vm_current_stamp()const;
    std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> retain_research_choice_gui_owned()const;
    // Pure storage retention for closed commit/provenance checks. Never calls
    // stale GUI currentness during post-dispatch refresh.
    std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> retain_research_choice_gui_storage_without_callbacks()const;
    std::optional<bool> test_policy_adoption_owned(std::int64_t,std::int64_t,const BnwStockLuaAttemptLease&);
    ScriptTestResult call_policy_adoption_hook_owned(std::int64_t,std::int64_t,const BnwStockLuaAttemptLease&);
    std::optional<bool> test_research_choice_owned(BnwResearchChoiceEvent,
        std::int64_t player,std::int64_t technology,const BnwStockLuaAttemptLease&);
    std::optional<bool> test_production_owned(BnwSelectedProductionLuaEvent,
        std::int32_t player,std::int32_t city,std::int32_t item,const BnwStockLuaAttemptLease&);
    void dispatch_initial_happiness_founder_owned(const BnwNativeUnitAllocatorCore&,
        const ui::SelectedCityTrainCreateSource&,const BnwStockLuaAttemptLease&);
    void dispatch_initial_unit_setxy_owned(const BnwNativeUnitAllocatorCore&,
        const ui::SelectedCityTrainCreateSource&,const BnwStockLuaAttemptLease&);
    void call_city_trained_owned(std::int32_t,std::int32_t,std::int32_t,const BnwStockLuaAttemptLease&);
    void invoke_team_tech_owned(std::shared_ptr<const class BnwOwnedTeamSetHasTechLuaDispatch>,
        std::int64_t team,std::int64_t technology,std::int64_t player,bool announce,
        const BnwStockLuaAttemptLease&);
    std::shared_ptr<const class BnwMajorTechTransportConsumption> dispatch_major_tech_award_owned(
        std::shared_ptr<const class BnwMajorTechTransportRequest>,const BnwStockLuaAttemptLease&);
    void dispatch_tech_award_input_owned(std::shared_ptr<const ui::BnwTechAwardInputDispatch>,const BnwStockLuaAttemptLease&);
    friend class BnwRetainedTurnCursor;
    void bind_engine_ui_localization_owned(ui::BnwEngineUiTurnOwner&,const BnwStockLuaAttemptLease&);
    ScriptIntegerResult accumulate_engine_founder_owned(std::int32_t,const BnwStockLuaAttemptLease&);
    ScriptIntegerResult accumulate_team_happiness_founder_owned(
        std::int64_t player,const BnwStockLuaAttemptLease&);
    void require_owned_physical_window()const;
    std::shared_ptr<const ui::detail::BnwNativeCompletedColorQuery> retain_completed_color_query(
        std::shared_ptr<const ui::detail::BnwNativeCompletedColorQuery>);
    void bind_completed_color_query(std::shared_ptr<const ui::detail::BnwNativeCompletedColorQuery>,
        std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner>);
    BnwStockLuaOperationRecord retained_operation_record()const;
    // Production-only entry: SourceOwner retains the genuine VM and inputs
    // across callbacks and guards the whole operation before/after effects.
    bool attach_native_client(std::uintptr_t actual_hwnd,BnwStockLuaClientExtentProvider);
    void bind_gui_screen(std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>);
    void attach_renderer_window_owner(
        std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>,
        BnwStockLuaClientExtentProvider);
    std::optional<ui::BnwCivilopediaNativeRuntimeFrame> native_runtime_frame()const;
    std::weak_ptr<const void> native_window_storage()const;
#ifdef _WIN32
    //Native capture notification closes pre-input hover reads without Lua.
    void note_worldview_native_input_entry_owned();
    bool dispatch_worldview_input_owned(const BnwWorldViewSourceOperationLoan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,const ui::BnwWorldViewWindowInputLoan&);
    void dispatch_worldview_shutdown_owned(const BnwWorldViewSourceOperationLoan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,const ui::BnwWorldViewWindowInputLoan&);
#endif
    bool drain_worldview_unit_created_owned(const BnwWorldViewSourceOperationLoan&,
        const std::weak_ptr<graphics::BnwUnitSetupGraphicalQueue>&);
    void retire_native_client()noexcept;
    struct Impl; std::unique_ptr<Impl> impl_;
};
}
