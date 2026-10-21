#include "civ5/ui/bnw_worldview_window_input_loan.hpp"
namespace civ5::ui {class LiveUiHost;class BnwEngineUiTurnOwner;}
namespace civ5::ui {class BnwTechAwardInputDispatch;}
namespace civ5::runtime {class BnwNativeUnitAllocatorCore;class BnwNativeAttritionPrefix;class BnwNativePlayerBusyUnitsReturn;class BnwNativePlayerBusyCitiesReturn;class BnwRetainedTurnCursor;struct BnwRetainedTurnView;class BnwPlayerTurnActiveDirtyIssuer;}
#pragma once
#include "civ5/runtime/bnw_attrition_city_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_unit_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_unit_selected_read.hpp"
#include "civ5/runtime/bnw_attrition_selected_unit_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_selection_plot_read.hpp"
#include "civ5/runtime/bnw_attrition_plot_list_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_damage_dll_wrapper_prefix.hpp"
namespace civ5::ui {class BnwScrollAnimationUpdateLoan;}
namespace civ5::runtime {class BnwLocalClientScalarOwner;}
#include "civ5/runtime/bnw_policy_adoption.hpp"
#include "civ5/runtime/bnw_barbarian_team_has_tech.hpp"

#include "civ5/runtime/bnw_team_tech_lua_gate.hpp"
#include "civ5/runtime/bnw_selected_production_action_loan.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/ui/civis_measured_civilopedia_lua.hpp"
#include "civ5/ui/selected_civilopedia_locale.hpp"
#include "civ5/ui/civis_selected_game_option_owner.hpp"
#include "civ5/runtime/civis_units_query_provider.hpp"
#include "civ5/runtime/bnw_stock_lua_client_extent.hpp"
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/runtime/bnw_owned_gameplay_event_pump_loan.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include <array>
#include <functional>
#include "civ5/saves/civis_save.hpp"

namespace civ5::ui { class SelectedCityProductionServices; struct SelectedTrainInitializationObservation; class SelectedBnwReadOnlyPreview; class SelectedCityTrainCreateSource; class BnwNativePackageDiscoveryOwner; class SelectedBnwPreviewHost; class BnwCityProductionUiAdapter;class BnwUnitMoveDimAdapter; }
namespace civ5::ui { struct CivilopediaMeasuredLayout; class BnwClientSceneFrame; class BnwAlphaProgressionBackend; }
namespace civ5::ui { struct BnwCivilopediaNativeRuntimeFrame; class BnwOwnedCameraGuiScreen; }
namespace civ5::assets { class ResourceMountTable; }
namespace civ5::ui::detail { class BnwNativeCompletedColorQuery; }
namespace civ5::graphics { class BnwOwnedCameraRendererWindowLease; class BnwTrainPresentedFrameProof; }
namespace civ5::ui { class BnwTrainRendererRefreshReceipt; }

namespace civ5::validation {template<class LandRenderer>struct CameraPreviewLoop;}
namespace civ5::ui {class UiScreenGlRenderer;class Win32ViewportTransaction;}
namespace civ5::ui {class BnwWorldViewControlTree;class BnwWorldViewGridToWorldLoan;}
namespace civ5::ui {class CivisResourceBankUiSnapshot;class CivisResourceBankFontColorSnapshot;}
namespace civ5::runtime {
class BnwCivisOwnedStockSourceBundle;
class BnwWorldViewSourceOperationLoan;
class BnwNativeUnitAllocatorCore;
class BnwMajorTechTransportRequest;class BnwMajorTechTransportConsumption;
class BnwLivePlayerGoldPending;
struct BnwStockLuaBootstrapProgress;
class BnwResearchChoiceCommit;
struct BnwResearchChoiceCommand;
// Actual core stores may have returned normally before a later renderer/source
// bookend fails. Keep that opaque receipt; never rewrite its native phase.
class BnwResearchChoiceSourceFailure final:public std::runtime_error {
  public:
    const std::shared_ptr<const BnwResearchChoiceCommit>& receipt()const noexcept {return receipt_;}
  private:
    friend class BnwSelectedStockLuaSourceOwner;
    explicit BnwResearchChoiceSourceFailure(std::shared_ptr<const BnwResearchChoiceCommit> actual,std::string reason)
        :std::runtime_error(std::move(reason)),receipt_(std::move(actual)) {}
    std::shared_ptr<const BnwResearchChoiceCommit> receipt_;
};
// Retained issued native receipt on an actual source failure. Issuance alone
// is not a completed policy effect, client refresh or permission to retry.
class BnwPolicyAdoptionSourceFailure final:public std::runtime_error {
  public:
    const std::shared_ptr<const BnwPolicyAdoptionPending>& receipt()const noexcept {return receipt_;}
  private:
    friend class BnwSelectedStockLuaSourceOwner;
    explicit BnwPolicyAdoptionSourceFailure(std::shared_ptr<const BnwPolicyAdoptionPending> actual,std::string reason)
        :std::runtime_error(std::move(reason)),receipt_(std::move(actual)) {}
    std::shared_ptr<const BnwPolicyAdoptionPending> receipt_;
};
class BnwCityProductionTurnPending;class BnwUnitMovePending;
struct BnwUnitMovePrefix;
struct BnwCityProductionTurnProgress;

struct BnwGameplayLuaForkResult {
    std::unique_ptr<BnwTeamTechLuaEpoch> epoch;
    std::string blocker;
};
struct BnwGameplayLuaPublishResult {
    bool committed=false, uncertain=false;
    std::string blocker;
};

// The session owner owns one gameplay Lua VM epoch alongside its live world.
// fork_at must clone the current selected context registry, bind all gameplay
// getters to the forked world and escrow script/world/UI/save-data writes.
// publish must install VM, world and buffered effects as one transaction. A
// publication error after an external effect is uncertain and cannot retry.
class BnwGameplayLuaEpochOwner {
  public:
    virtual ~BnwGameplayLuaEpochOwner()=default;
    virtual BnwGameplayLuaForkResult fork_at(const LiveSessionState &)=0;
    virtual BnwGameplayLuaPublishResult publish(
        std::unique_ptr<BnwTeamTechLuaEpoch>,LiveSessionState &)=0;
};

struct BnwSelectedStockLuaEvidence {
    bool selected_profile=false, first_include_executed=false;
    std::uint64_t declared_in_game_contexts=0;
    std::string first_context_id, declaration_sha256;
    std::string first_source_sha256, first_include_sha256;
    std::string blocker;
};

struct BnwCivilopediaParentProbe {
    bool selected_source_verified=false;
    bool full_parent_loaded=false;
    std::size_t manager_bindings=0;
    std::size_t invalidation_listeners=0;
    std::size_t civilopedia_listeners=0;
    std::size_t layout_operations_completed=0;
    // Diagnostic observations in the disposable parent VM. They do not make
    // that failed/incomplete VM an active UI or gameplay context.
    std::size_t category_callback_controls=0;
    std::size_t registered_category_callbacks=0;
    std::size_t top_level_callback_controls=0,registered_control_callbacks=0;
    std::size_t registered_text_callbacks=0,registered_context_handlers=0;
    std::size_t tooltip_type_registrations=0,tooltip_control_tables=0;
    bool tooltip_render_receipt_verified=false;
    std::vector<ui::CivilopediaLocaleObservation> resolved_locale_labels;
    std::int32_t viewport_width=0,viewport_height=0;
    std::int32_t scrollbar_width=0,scrollbar_height=0;
    std::int32_t scroll_panel_height=0,down_button_offset_y=0;
    bool scroll_subtree_verified=false;
    std::size_t scroll_nodes=0,content_roots=0,explicit_hidden=0;
    std::vector<std::string> observed_include_order;
    std::string first_missing_api;
    std::string blocker;
};
enum class BnwCivilopediaInputCapability { captured_civis_input };
enum class BnwCivilopediaTooltipCapability { source_declared_state };
enum class BnwCivilopediaLocaleTextCapability { civis_windows_nls };
enum class BnwCivilopediaTableCountCapability { civis_raw_immediate_keys };
enum class BnwCivilopediaGameOptionCapability { civis_selected_no_religion };
enum class BnwCivilopediaUnitsQueryCapability { civis_first_units_statement };
// Separate source-declared statement policy; old Units capability is unchanged.
enum class BnwCivilopediaBuildingsQueryCapability { civis_units_and_buildings_statements };
// Only the new overload returns this additional diagnostic observation;
// existing parent-probe ABI and older service frontiers remain unchanged.
struct BnwCivilopediaGameOptionProbe {
    BnwCivilopediaParentProbe parent;
    std::optional<ui::CivisSelectedGameOptionResolution> option;
};

struct BnwCivilopediaUnitsQueryProbe {
    BnwCivilopediaParentProbe parent;
    std::optional<ui::CivisSelectedGameOptionResolution> option;
    ScriptCivisUnitsQueryObservations query;
};

// First source-backed stock owner stage. It pins the selected Gandhi DLC
// profile and executes the first authored CivilopediaScreen include from the
// actual installation. It can also own the persistent partial stock bootstrap;
// this still cannot fork/publish a complete selected gameplay epoch.

// The genuine issuer may throw after irreversible paired/native/Lua effects.
// Only this SourceOwner can construct this diagnostic receipt-bearing failure;
// it grants no replay, rollback, VM readiness, technology or turn completion.
class BnwMajorTeamSourceFailure final:public std::runtime_error {
 public:
 const std::shared_ptr<const BnwMajorTeamHasTechPending>& receipt()const noexcept{return receipt_;}
 private:
 friend class BnwSelectedStockLuaSourceOwner;
 BnwMajorTeamSourceFailure(std::string reason,std::shared_ptr<const BnwMajorTeamHasTechPending> receipt)
  :std::runtime_error(std::move(reason)),receipt_(std::move(receipt)){}
 std::shared_ptr<const BnwMajorTeamHasTechPending> receipt_;
};

class BnwSelectedStockLuaSourceOwner final:public BnwGameplayLuaEpochOwner {
  public:
    BnwSelectedStockLuaSourceOwner(std::filesystem::path game_root,
        content::Catalog catalog,SavedContentProfile selected,
        SessionCheckpointIdentity identity);
    ~BnwSelectedStockLuaSourceOwner() override;
    // Genuine selected construction path. Legacy constructors retain their
    // existing behavior but cannot issue alpha action/renderer bindings.
    static std::shared_ptr<BnwSelectedStockLuaSourceOwner> create_selected(
        std::filesystem::path,content::Catalog,SavedContentProfile,
        SessionCheckpointIdentity);
    static std::shared_ptr<BnwSelectedStockLuaSourceOwner> create_selected_from_civis_resources(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,
        std::shared_ptr<const ui::CivisResourceBankUiSnapshot>,
        std::shared_ptr<const ui::CivisResourceBankFontColorSnapshot>);
    BnwStockLuaBootstrapProgress start_stock_bootstrap_from_civis_resources();
    BnwSelectedStockLuaSourceOwner(const BnwSelectedStockLuaSourceOwner&)=delete;
    BnwSelectedStockLuaSourceOwner& operator=(const BnwSelectedStockLuaSourceOwner&)=delete;
    // Own the genuine selected suspended stock VM, never a disposable probe.
    // Requires this actual host's explicitly sealed immutable preview DB.
    // Include bnw_selected_stock_lua_bootstrap.hpp to consume returned progress.
    BnwStockLuaBootstrapProgress start_stock_bootstrap(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,
        std::filesystem::path shared_asset_root);
    bool attach_stock_client_extent(BnwStockLuaClientExtentProvider);
    bool attach_stock_native_client(std::uintptr_t actual_hwnd,BnwStockLuaClientExtentProvider);
    // INITIAL physical renderer source only. The real producer stack Loan
    // authenticates the shared control; this cannot open gameplay callbacks.
    void attach_stock_gui_screen(std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>);
    void attach_stock_renderer_window_owner(
        std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>,
        BnwStockLuaClientExtentProvider);
    // Genuine completed ordinary Color source only; actual Parent is derived
    // from the privately adopted Host, never supplied by the caller.
    void attach_stock_completed_color_query(std::shared_ptr<const ui::detail::BnwNativeCompletedColorQuery>);
    std::optional<ui::BnwCivilopediaNativeRuntimeFrame> stock_native_runtime_frame()const;
    // Lifetime observation only. Weak erased storage cannot execute/admit UI.
    // It does not retain the session, VM, physical Window or native registration.
    std::weak_ptr<const void> stock_native_window_storage()const;
    BnwStockLuaBootstrapProgress advance_stock_bootstrap();
    // Borrowed until this owner is destroyed; refuses retired/stale binding.
    const BnwStockLuaBootstrapProgress* stock_bootstrap_progress() const;
    // Foreign-thread revoke touches only the stable constructor-owned atomic
    // control. Native teardown is deferred until owner-thread retirement or
    // safe final release; it never reads mutable VM publication cross-thread.
    void retire_stock_bootstrap() noexcept;
    // Owner-thread retained uncertainty journal; never a currentness/Ready grant.
    BnwStockLuaOperationEvidence stock_operation_evidence()const;
    // Exact ID6 DB domain only. Does NOT export modified world or VM/once history.
    // Requires healthy SAME attempted production VM and quiescent SQL connections.
    saves::CivisSaveSection snapshot_stock_save_data();
    // Real SAME-VM tests and opaque stores, followed by genuine Civis UI dirty
    // publications. Full Original selected UI/loading parity is not asserted.
    // Genuine native prefix only; incomplete suffix never becomes UI performed.
    std::shared_ptr<const BnwPolicyAdoptionPending> run_same_policy_adoption_prefix(const BnwPolicyAdoptionCommand&,std::shared_ptr<const ui::LiveUiHost> actual_ui={});
    // Creator-only retained history, including denied/uncertain issued receipts.
    // This observation never renews a failed VM or claims refreshed UI.
    std::shared_ptr<const BnwPolicyAdoptionPending> pending_same_policy_adoption()const;
    std::shared_ptr<const BnwResearchChoiceCommit> run_same_research_choice(
        const BnwResearchChoiceCommand&);
    std::shared_ptr<ui::BnwAlphaProgressionBackend> create_alpha_progression_backend(
        std::shared_ptr<ui::BnwClientSceneFrame>);
    // Pure actual pending-receipt observation; expired association refuses.
    // No VM/provider resumes while a post-dispatch render is pending.
    std::shared_ptr<const BnwResearchChoiceCommit> pending_same_research_choice()const;
    // The backend calls this only after real post-dispatch paint/present and
    // issued Scene/UI finality checks, with the compositor-issued fresh owner.
    void complete_same_research_choice_after_refresh(
        std::shared_ptr<const BnwResearchChoiceCommit>,
        std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>);
    const BnwSelectedStockLuaEvidence &evidence() const noexcept {return evidence_;}
    // Runs the installed parent chunk in a disposable, selected-world VM.
    // A missing native UI service stops execution at its actual Lua call.
    // No failed VM or its listener registry is published to gameplay.
    BnwCivilopediaParentProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected) const;
    BnwCivilopediaParentProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected,
        const assets::ResourceMountTable &selected_ui_resources,
        const ui::CivilopediaMeasuredLayout &measured) const;
    BnwCivilopediaParentProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected,
        const assets::ResourceMountTable &selected_ui_resources,
        const ui::CivilopediaMeasuredLayout &measured,
        ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements) const;
    BnwCivilopediaParentProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected,
        const assets::ResourceMountTable &selected_ui_resources,
        const ui::CivilopediaMeasuredLayout &measured,
        ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
        std::int64_t civis_left_click_event) const;
    BnwCivilopediaParentProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected,
        const assets::ResourceMountTable &selected_ui_resources,
        const ui::CivilopediaMeasuredLayout &measured,
        ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
        std::int64_t civis_left_click_event,
        std::shared_ptr<ui::SelectedCivilopediaLocale> locale) const;
    BnwCivilopediaParentProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected,
        const assets::ResourceMountTable &selected_ui_resources,
        const ui::CivilopediaMeasuredLayout &measured,
        ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
        std::int64_t civis_left_click_event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,
        BnwCivilopediaInputCapability input) const;
    BnwCivilopediaParentProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &selected_ui_resources,
        const ui::CivilopediaMeasuredLayout &measured,
        ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
        std::int64_t civis_left_click_event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,
        BnwCivilopediaTooltipCapability tooltip,const assets::ResourceMountTable *texture_resources=nullptr) const;
    BnwCivilopediaParentProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &selected_ui_resources,
        const ui::CivilopediaMeasuredLayout &measured,
        ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
        std::int64_t civis_left_click_event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,
        BnwCivilopediaLocaleTextCapability text_operations,const assets::ResourceMountTable *texture_resources=nullptr) const;
    BnwCivilopediaParentProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &selected_ui_resources,
        const ui::CivilopediaMeasuredLayout &measured,
        ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
        std::int64_t civis_left_click_event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,
        BnwCivilopediaTableCountCapability count,const assets::ResourceMountTable *texture_resources=nullptr) const;
    BnwCivilopediaGameOptionProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &selected_ui_resources,
        const ui::CivilopediaMeasuredLayout &measured,
        ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
        std::int64_t civis_left_click_event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,
        BnwCivilopediaGameOptionCapability option,const assets::ResourceMountTable *texture_resources=nullptr) const;
    BnwCivilopediaUnitsQueryProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &selected_ui_resources,
        const ui::CivilopediaMeasuredLayout &measured,
        ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
        std::int64_t civis_left_click_event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,
        BnwCivilopediaUnitsQueryCapability query,const assets::ResourceMountTable *texture_resources=nullptr) const;
    BnwCivilopediaUnitsQueryProbe probe_parent_chunk(
        const ui::SelectedBnwPreviewHost &selected,const assets::ResourceMountTable &selected_ui_resources,
        const ui::CivilopediaMeasuredLayout &measured,
        ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements,
        std::int64_t civis_left_click_event,std::shared_ptr<ui::SelectedCivilopediaLocale> locale,
        BnwCivilopediaBuildingsQueryCapability query,const assets::ResourceMountTable *texture_resources=nullptr) const;
    void run_same_tech_award_input(std::shared_ptr<const ui::BnwTechAwardInputDispatch>);
    // Actual SAME-VM query; a detached expected world is refused.
    std::optional<std::int64_t> query_treasury_founder_benefits(std::int64_t actor,const LiveSessionState& expected_world);
    // Explicit original-import Gold component, never a scheduler grant.
    std::shared_ptr<const BnwLivePlayerGoldPending> begin_imported_player_gold_component(std::int64_t actor,std::shared_ptr<const ui::LiveUiHost> actual_ui={},std::shared_ptr<const BnwNativeUnitAllocatorCore> actual_deficit_core={});
    BnwRetainedTurnView run_engine_ui_turn(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_tail_to_next_required_owner(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_turn_clock_prefix(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_turn_ui_dirty_prefix(std::shared_ptr<BnwRetainedTurnCursor>,std::shared_ptr<const ui::LiveUiHost>);
    BnwRetainedTurnView run_game_turn_sent_auto_prefix(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_turn_end_timer_prefix(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_turn_elapsed_prefix(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_turn_activation_prefix(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_turn_shuffle_prefix(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_turn_player_active_prefix(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_turn_player_dirty_prefix(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_turn_player_end_turn_prefix(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_turn_player_busy_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_game_turn_player_busy_city_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_game_turn_player_end_after_busy_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_processed_auto_moves_after_busy(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_player_attrition_mountain_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_player_attrition_damage_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&,const std::shared_ptr<const ui::LiveUiHost>&);
    BnwRetainedTurnView run_player_attrition_garrison_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_player_attrition_city_strength_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_player_attrition_city_info_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_player_attrition_unit_info_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_player_attrition_unit_selected_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_player_attrition_selected_unit_info_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_player_attrition_selection_plot_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_player_attrition_plot_list_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
    BnwRetainedTurnView run_player_attrition_dll_wrapper_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&,const std::shared_ptr<ui::BnwClientSceneFrame>&);
    BnwRetainedTurnView run_player_attrition_first_prefix(std::shared_ptr<BnwRetainedTurnCursor>,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);

    BnwRetainedTurnView resume_game_turn_player_after_dirty_delivery(std::shared_ptr<BnwRetainedTurnCursor>);
    BnwRetainedTurnView run_game_turn_gameplay_prefix(std::shared_ptr<BnwRetainedTurnCursor>,std::shared_ptr<const ui::LiveUiHost>);
    // Genuine existing SAME-VM Team:SetHasTech C path. Missing mandatory
    // suffix throws receipt-bearing failure, never void-success/replay.
    std::shared_ptr<const BnwMajorTeamHasTechPending> run_same_team_tech(
        std::int64_t team,std::int64_t technology,std::int64_t player,bool announce);
    // Active operation only; the opaque request is issued by the actual UI dispatcher.
    std::shared_ptr<const BnwMajorTechTransportConsumption> dispatch_major_tech_award_actual(
        std::shared_ptr<const BnwMajorTechTransportRequest>);
    // Own factory pins genuine canonical selected Host/VM/session; no caller
    // state or owner DTO can mint the live production backend binding.
    std::shared_ptr<ui::SelectedCityProductionServices> create_selected_city_production_services();
    // Creator/canonical-control/pending metadata ONLY. No DB, session, VM
    // providers or new serial. Call before every production-service borrow.
    std::shared_ptr<BnwSelectedStockLuaSourceOwner>
        retain_production_entry_owner_without_callbacks()const;
    std::shared_ptr<const ui::BnwTrainRendererRefreshReceipt> pending_train_renderer_refresh()const;
    // Definition requires the genuine graphics after-present proof supplier.
    void complete_train_renderer_refresh_after_present(
        std::shared_ptr<const ui::BnwTrainRendererRefreshReceipt>,
        const graphics::BnwTrainPresentedFrameProof&);
    BnwGameplayLuaForkResult fork_at(const LiveSessionState &) override;
    BnwGameplayLuaPublishResult publish(
        std::unique_ptr<BnwTeamTechLuaEpoch>,LiveSessionState &) override;
  private:
    friend class BnwLivePlayerGoldPending;
    friend class BnwLiveGameContext;
    void require_live_gold_deficit_random_operands_without_callbacks(const BnwLivePlayerGoldPending&,
        const BnwNativeUnitAllocatorCore&)const;
    std::array<std::int32_t,2> require_live_gold_deficit_scan_return(const BnwLivePlayerGoldPending&,const BnwNativeUnitAllocatorCore&)const;
    friend class BnwRetainedTurnCursor;
    friend class BnwAttritionCityInfoDirtyLoan;
    friend class BnwAttritionUnitInfoDirtyLoan;
    friend class BnwAttritionUnitSelectedRead;
    friend class BnwAttritionSelectedUnitInfoDirtyLoan;
    friend class BnwAttritionSelectionPlotRead;
    friend class BnwAttritionPlotListDirtyLoan;
    friend class BnwAttritionDamageDllWrapperPrefix;
    friend class BnwPlayerTurnActiveDirtyIssuer;
    std::shared_ptr<const BnwPlayerTurnActiveDirtyIssuer> issue_player_turn_dirty_owned(const std::shared_ptr<BnwRetainedTurnCursor>&,const BnwStockLuaAttemptLease&)const;
    void require_player_turn_dirty_source_without_callbacks(const BnwPlayerTurnActiveDirtyIssuer&,const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease&,bool)const;
    void require_player_turn_dirty_idle_source_without_callbacks(const BnwPlayerTurnActiveDirtyIssuer&,const BnwRetainedTurnCursor&)const;
    void adopt_actual_game_turn_clock_return(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease&)const;
    void adopt_actual_game_turn_sent_auto_return(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease&)const;
    void adopt_actual_game_turn_gameplay_return(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease&)const;
    void adopt_actual_game_turn_elapsed_return(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease&)const;
    void adopt_actual_game_turn_activation_return(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease&)const;
    void adopt_actual_game_turn_shuffle_return(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease&)const;
    void adopt_actual_game_turn_player_active_return(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease&)const;
    void adopt_actual_game_turn_player_end_turn_return(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease&)const;
    void require_player_end_turn_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease*,bool,std::uint64_t)const;
    void issue_player_busy_units_return_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,
     const BnwStockLuaAttemptLease&,std::shared_ptr<const BnwNativePlayerBusyUnitsReturn>&)const;
    void seal_player_busy_units_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwNativePlayerBusyUnitsReturn&,
     const BnwStockLuaAttemptLease*,bool)const;
    void require_player_busy_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,
     const BnwStockLuaAttemptLease*,bool)const;


    void require_player_busy_city_reentry_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&)const;
    void require_player_busy_units_parent_for_city_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void issue_player_busy_cities_return_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,
     std::shared_ptr<const BnwNativePlayerBusyUnitsReturn>,std::shared_ptr<const BnwNativePlayerBusyCitiesReturn>&)const;
    void seal_player_busy_cities_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwNativePlayerBusyCitiesReturn&,const BnwStockLuaAttemptLease*,bool)const;
    void require_player_busy_city_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_player_end_after_busy_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,
     const BnwStockLuaAttemptLease*,bool)const;
    void adopt_actual_player_end_after_busy_return(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&)const;
    void require_player_end_after_busy_reentry_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&)const;
    void require_processed_auto_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_processed_auto_parent_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_processed_auto_reentry_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&)const;
    void require_attrition_first_reentry_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&)const;
    void require_attrition_mountain_parent_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_attrition_mountain_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void issue_attrition_mountain_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,const BnwNativeAttritionPrefix&)const;
    void seal_attrition_mountain_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwNativeAttritionPrefix&,const BnwStockLuaAttemptLease&,bool)const;
    void adopt_actual_attrition_mountain_return(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&)const;
    void require_attrition_damage_parent_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void issue_attrition_damage_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,const BnwNativeAttritionPrefix&,const std::shared_ptr<const ui::LiveUiHost>&,const std::weak_ptr<ui::BnwEngineUiTurnOwner>&)const;
    void seal_attrition_damage_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwNativeAttritionPrefix&,const BnwStockLuaAttemptLease&,bool)const;
    void adopt_actual_attrition_damage_return(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&)const;
    void require_attrition_damage_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_attrition_garrison_parent_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void issue_attrition_garrison_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,const BnwNativeAttritionPrefix&)const;
    void seal_attrition_garrison_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwNativeAttritionPrefix&,const BnwStockLuaAttemptLease&,bool)const;
    void adopt_actual_attrition_garrison_return(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&)const;
    void require_attrition_garrison_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_attrition_city_strength_parent_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void issue_attrition_city_strength_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,const BnwNativeAttritionPrefix&)const;
    void seal_attrition_city_strength_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwNativeAttritionPrefix&,const BnwStockLuaAttemptLease&,bool)const;
    void adopt_actual_attrition_city_strength_return(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&)const;
    void require_attrition_city_strength_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_attrition_city_info_parent_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    std::shared_ptr<const BnwAttritionCityInfoDirtyLoan> issue_attrition_city_info_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,const BnwNativeAttritionPrefix&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
    void require_attrition_city_info_operation_without_callbacks(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease*,std::uint64_t,std::uint64_t,bool)const;
    void adopt_attrition_city_info_without_callbacks(std::uint64_t)const noexcept;
    void require_attrition_city_info_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_attrition_unit_info_parent_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    std::shared_ptr<const BnwAttritionUnitInfoDirtyLoan> issue_attrition_unit_info_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,const BnwNativeAttritionPrefix&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
    void require_attrition_unit_info_operation_without_callbacks(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease*,std::uint64_t,std::uint64_t,bool)const;
    void adopt_attrition_unit_info_without_callbacks(std::uint64_t)const noexcept;
    void require_attrition_unit_info_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_attrition_unit_selected_parent_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    std::shared_ptr<const BnwAttritionUnitSelectedRead> issue_attrition_unit_selected_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,const BnwNativeAttritionPrefix&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
    void require_attrition_unit_selected_operation_without_callbacks(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease*,std::uint64_t,std::uint64_t,bool)const;
    void adopt_attrition_unit_selected_without_callbacks(std::uint64_t)const noexcept;
    void require_attrition_unit_selected_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_attrition_selected_unit_info_parent_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    std::shared_ptr<const BnwAttritionSelectedUnitInfoDirtyLoan> issue_attrition_selected_unit_info_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,const BnwNativeAttritionPrefix&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
    void require_attrition_selected_unit_info_operation_without_callbacks(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease*,std::uint64_t,std::uint64_t,bool)const;
    void adopt_attrition_selected_unit_info_without_callbacks(std::uint64_t)const noexcept;
    void require_attrition_selected_unit_info_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_attrition_selection_plot_parent_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    std::shared_ptr<const BnwAttritionSelectionPlotRead> issue_attrition_selection_plot_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,const BnwNativeAttritionPrefix&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
    void require_attrition_selection_plot_operation_without_callbacks(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease*,std::uint64_t,std::uint64_t,bool)const;
    void adopt_attrition_selection_plot_without_callbacks(std::uint64_t)const noexcept;
    void require_attrition_selection_plot_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_attrition_plot_list_parent_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void require_attrition_dll_wrapper_parent_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    std::shared_ptr<const BnwAttritionDamageDllWrapperPrefix> issue_attrition_dll_wrapper_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,const BnwNativeAttritionPrefix&)const;

    void require_attrition_dll_wrapper_operation_without_callbacks(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease*,std::uint64_t,std::uint64_t,bool)const;
    void adopt_attrition_dll_wrapper_without_callbacks(std::uint64_t)const noexcept;
    void require_attrition_dll_wrapper_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    std::shared_ptr<const BnwAttritionPlotListDirtyLoan> issue_attrition_plot_list_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,const BnwNativeAttritionPrefix&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
    void require_attrition_plot_list_operation_without_callbacks(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease*,std::uint64_t,std::uint64_t,bool)const;
    void adopt_attrition_plot_list_without_callbacks(std::uint64_t)const noexcept;
    void require_attrition_plot_list_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void issue_attrition_first_owned(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&,
     std::shared_ptr<const BnwNativeAttritionPrefix>&)const;
    void seal_attrition_first_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwNativeAttritionPrefix&,const BnwStockLuaAttemptLease&,bool)const;
    void require_attrition_first_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void adopt_actual_attrition_first_return(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&)const;
    void require_attrition_first_parent_source_without_callbacks(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease*,bool)const;
    void adopt_actual_processed_auto_return(const BnwRetainedTurnCursor&,const BnwNativeUnitAllocatorCore&,const BnwStockLuaAttemptLease&)const;
    friend class BnwPolicyAdoptionPending;
    friend class BnwOwnedGameplayEventPumpLoan;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class ::civ5::ui::Win32ViewportTransaction;
    void with_worldview_renderer_context_owned(const ui::BnwOwnedCameraGuiScreen&,const ui::BnwWorldViewGridToWorldLoan&,
      const std::function<void(const BnwWorldViewSourceOperationLoan&,
        const ui::BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>&)>&);

#ifdef _WIN32
    struct WorldViewInputQueue;
    std::shared_ptr<WorldViewInputQueue> worldview_input_queue_;
    void capture_worldview_native_input_owned(std::shared_ptr<const ui::BnwWorldViewWindowInputLoan>);
    void consume_worldview_native_input_owned(const BnwWorldViewSourceOperationLoan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,bool closing);
    void require_worldview_input_origin_without_callbacks()const;
    void shutdown_worldview_renderer_context_owned(const ui::BnwOwnedCameraGuiScreen&,const ui::BnwWorldViewGridToWorldLoan&);
#endif
    void with_worldview_native_dispatch_owned(const ui::BnwOwnedCameraGuiScreen&,const ui::BnwWorldViewGridToWorldLoan&,
      const std::function<void(const BnwWorldViewSourceOperationLoan&,
        const ui::BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>&)>*);
    template<class LandRenderer>friend struct ::civ5::validation::CameraPreviewLoop;
    BnwOwnedGameplayEventPumpLoan::SourceLineage require_gameplay_pump_source_without_callbacks()const;
    void pump_gameplay_events_owned(const BnwOwnedGameplayEventPumpLoan&);
    mutable std::atomic<std::size_t> gameplay_pump_facade_pins_{0};
    void require_policy_adopt_hook_return(const BnwPolicyAdoptionPending&)const;
    void adopt_engine_cache_revision(const BnwRetainedTurnCursor&,const BnwStockLuaAttemptLease&)const;
    friend class ui::BnwUnitMoveDimAdapter;
    friend class BnwSelectedProductionActionLoan;
    friend class BnwStockLuaAttemptLease;
    friend std::unique_ptr<BnwSelectedProductionActionLoan> borrow_selected_city_production(
        std::shared_ptr<BnwSelectedStockLuaSourceOwner>,std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int32_t,std::int32_t);
    friend void execute_selected_completed_train_prefix(std::shared_ptr<BnwSelectedStockLuaSourceOwner>,const std::shared_ptr<ui::SelectedCityTrainCreateSource>&);
    friend void require_selected_train_help_dirty(const BnwSelectedStockLuaSourceOwner&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::int32_t,std::int32_t,std::uint64_t,std::uint64_t);
    friend std::shared_ptr<const BnwCityProductionTurnPending> advance_selected_city_process_production(std::shared_ptr<BnwSelectedStockLuaSourceOwner>,std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int32_t,std::int32_t,std::shared_ptr<const BnwCityProductionTurnPending>);
    std::unique_ptr<BnwSelectedProductionActionLoan> borrow_city_production(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int32_t,std::int32_t);
    void require_production_binding(const ui::SelectedBnwPreviewHost&,const LiveSession&,std::int32_t,std::int32_t)const;
    SelectedProductionResult run_production_maintain(ui::SelectedMaintainPreparedAction&,bool);
    SelectedProductionResult run_production_construct(ui::SelectedConstructPreparedAction&,bool);
    SelectedProductionResult run_production_train(ui::SelectedTrainPreparedAction&,bool);
    std::shared_ptr<const BnwCityProductionTurnPending> run_train_component(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int32_t,std::int32_t,
        std::shared_ptr<const BnwCityProductionTurnPending>);
    friend std::shared_ptr<const BnwCityProductionTurnPending> advance_selected_city_train_production(
        std::shared_ptr<BnwSelectedStockLuaSourceOwner>,std::shared_ptr<const ui::SelectedBnwPreviewHost>,
        std::int32_t,std::int32_t,std::shared_ptr<const BnwCityProductionTurnPending>);
    friend ui::SelectedTrainInitializationObservation initialize_selected_completed_train_unit(std::shared_ptr<BnwSelectedStockLuaSourceOwner>,const ui::SelectedBnwReadOnlyPreview&,const std::shared_ptr<const ui::SelectedCityTrainCreateSource>&,const std::shared_ptr<const BnwNativeUnitAllocatorCore>&,std::shared_ptr<ui::BnwClientSceneFrame>);
    ui::SelectedTrainInitializationObservation run_completed_train_initialization(const ui::SelectedBnwReadOnlyPreview&,const std::shared_ptr<const ui::SelectedCityTrainCreateSource>&,const std::shared_ptr<const BnwNativeUnitAllocatorCore>&,std::shared_ptr<ui::BnwClientSceneFrame>);
    friend SelectedProductionResult advance_selected_completed_train_tail(std::shared_ptr<BnwSelectedStockLuaSourceOwner>,const std::shared_ptr<const ui::SelectedCityTrainCreateSource>&);
    SelectedProductionResult run_completed_train_tail(const std::shared_ptr<const ui::SelectedCityTrainCreateSource>&);
    void run_completed_train_prefix(const std::shared_ptr<ui::SelectedCityTrainCreateSource>&);
    void require_actual_train_help(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::int32_t,std::int32_t,std::uint64_t,std::uint64_t)const;
    std::shared_ptr<const BnwCityProductionTurnPending> run_process_component(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int32_t,std::int32_t,std::shared_ptr<const BnwCityProductionTurnPending>);
    // GRAPHICS-owned adapter only: genuine registered Pending, never an ack.
    BnwUnitMovePrefix run_same_unit_dim(std::shared_ptr<const BnwUnitMovePending>);
    explicit BnwSelectedStockLuaSourceOwner(std::shared_ptr<const BnwCivisOwnedStockSourceBundle>);
    std::shared_ptr<const BnwCivisOwnedStockSourceBundle> civis_sources_;
    std::filesystem::path game_root_;
    content::Catalog catalog_;
    SavedContentProfile selected_;
    SessionCheckpointIdentity identity_;
    BnwSelectedStockLuaEvidence evidence_;
    // Only the actual future source-owned production dispatcher may call this
    // member. No public lease rebaseline, callback, BANNER return or epoch grant.
    BnwCityProductionTurnProgress run_same_production_city_screen(
        ui::BnwCityProductionUiAdapter&,
        std::shared_ptr<const BnwCityProductionTurnPending>);
    std::weak_ptr<BnwSelectedStockLuaSourceOwner> action_self_;
    std::shared_ptr<BnwSelectedStockLuaSourceOwner> retain_action_factory_owner()const;
    struct StockThreadLease;
    std::shared_ptr<StockThreadLease> stock_thread_;
    friend class ui::BnwTrainRendererRefreshReceipt;
    void require_pending_train_renderer_refresh(const ui::BnwTrainRendererRefreshReceipt&)const;
    void require_train_presented_scope_without_callbacks(const ui::BnwTrainRendererRefreshReceipt&)const;
    void finish_train_presented_scope_after_target(const ui::BnwTrainRendererRefreshReceipt&);
    void quarantine_train_presented_scope_without_callbacks()noexcept;
    void run_sp_client_update_owned(const BnwWorldViewSourceOperationLoan&,
        const ui::BnwScrollAnimationUpdateLoan&);
    std::weak_ptr<BnwLocalClientScalarOwner> canonical_local_sp_client_origin_owned(
        const BnwWorldViewSourceOperationLoan&)const;
    struct StockBootstrap;
    std::shared_ptr<StockBootstrap> bootstrap_;
    BnwCivilopediaParentProbe probe_parent_chunk_impl(
        const ui::SelectedBnwPreviewHost &selected,
        const assets::ResourceMountTable *selected_ui_resources,
        const ui::CivilopediaMeasuredLayout *measured,
        ui::CivisCivilopediaScrollMeasurementProvider renderer_measurements = {},
        std::optional<std::int64_t> civis_left_click_event = {},
        std::shared_ptr<ui::SelectedCivilopediaLocale> locale = {},bool captured_input = false,
        bool tooltip_state = false,const assets::ResourceMountTable *texture_resources = nullptr,
        bool locale_text_operations = false,bool raw_table_count = false,bool selected_game_option=false,
        std::optional<ui::CivisSelectedGameOptionResolution> *option_observation=nullptr,
        bool first_units_query=false,ScriptCivisUnitsQueryObservations *query_observation=nullptr,
        bool buildings_query=false) const;
};

} // namespace civ5::runtime
