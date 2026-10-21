#pragma once
#include <cstdint>
#include <array>
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/runtime/bnw_two_layer_unit_pathfinder.hpp"
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/bnw_unit_mission_continuation_checkpoint.hpp"
#include "civ5/runtime/bnw_imported_damage_archive.hpp"
#include "civ5/runtime/bnw_attrition_damage_child.hpp"
#include "civ5/runtime/bnw_attrition_garrison_child.hpp"
#include "civ5/runtime/bnw_attrition_city_strength_child.hpp"
#include "civ5/runtime/bnw_attrition_city_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_unit_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_unit_selected_read.hpp"
#include "civ5/runtime/bnw_attrition_selected_unit_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_selection_plot_read.hpp"
#include "civ5/runtime/bnw_attrition_plot_list_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_damage_dll_wrapper_prefix.hpp"
#include <map>
#include <set>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace civ5::graphics {class BnwPendingUnitVisualOwner;class BnwPendingUnitHiddenSlotProof;class BnwPendingUnitPositionProof;class BnwSelectedPendingUnitVisualLoan;class BnwInitialCenterUnitRendererLoan;class BnwInitialCenterUnitVisibilityProof;class BnwInitialUnitShouldDimRendererLoan;class BnwInitialUnitShouldDimEnqueueProof;class BnwInitialUnitSetupGraphicalRendererLoan;class BnwInitialUnitSetupGraphicalEnqueueProof;}
namespace civ5::saves {struct SavedUnitObservation;struct SavedSession;}
namespace civ5::ui {class BnwEngineUiTurnOwner;class SelectedBnwReadOnlyPreview;class SelectedCityTrainCreateSource;class SelectedBnwPreviewHost;class BnwClientSelectionControl;class BnwClientSceneFrame;struct BnwClientUnitDirtySnapshot;enum class BnwClientUnitDirtyKind:std::int32_t;}
namespace civ5::runtime {
class BnwPendingCreatedUnitRead;class BnwUnitCombatWarnedRestorePlan;struct BnwPathfinderCacheControlOwner;
class BnwSelectedStockLuaBootstrap;class BnwSelectedStockLuaSourceOwner;
class BnwNativeUnitPathMovementCost;class BnwTwoLayerUnitPathFinder;struct BnwPlotUnitReadServices;class LiveSession;struct LiveSessionState;struct BnwCityDirtyEvent;class BnwUnitMovePending;enum class BnwUnitMoveBoundary;class BnwInitialUnitSightOwner;class BnwMinorCityStrengthScratch;
class BnwNativeUnitAllocatorUnavailable:public std::runtime_error{public:using std::runtime_error::runtime_error;};
struct BnwNativeUnitSlotReservation {
 std::int32_t player=-1,slot=-1,reserved_id=-1;
 std::int32_t slots=0,last=-1,head=-1,free_count=0,current_id=0;
 std::vector<std::int64_t> next_free;
 std::map<std::string,std::int32_t,std::less<>> constructor_reset_scalars;
 // Actual private storage is allocated and its native Reset scalar prefix
 // executed. CvUnit ctor remainder precedes SetID and serial advance.
 // This is NOT a world unit, completed Add/initUnit, placement or turn receipt.
};
struct BnwNativeUnitConstructorStorageSnapshot {
 std::int32_t slot=-1;
 std::map<std::string,std::int32_t,std::less<>> scalars;
 std::map<std::string,std::vector<std::int32_t>,std::less<>> indexed;
 std::map<std::string,std::string,std::less<>> strings;
 std::vector<std::int32_t> promotion_info_ids;
 std::vector<std::uint8_t> promotion_bits;
 std::int32_t religion=-1,religion_strength=0,religion_spreads=0;
 std::uint32_t last_path_destination=UINT32_MAX;
 // Diagnostic copy of real private owned storage. No completion/ID authority.
};
struct BnwNativeUnitArchiveObservation {
 // Actual SAME-node bindings in native constructor order; read-only copy.
 std::vector<std::string> binding_names,binding_members;
 std::vector<std::string> dirty_members,vector_clear_members;
 std::map<std::string,std::int32_t,std::less<>> scalar_values;
 std::weak_ptr<const void> registered_lifetime;
};
// One genuinely registered imported scalar only, never a whole-unit archive.
struct BnwNativeImportedMovesArchiveObservation {
 std::int32_t owner=-1,full_id=-1,value=0,loaded_value=0;
 std::size_t native_archive_index=0;
 std::string binding_name,binding_member;
 bool dirty=false,call_stack_tracking=true;
 std::uint64_t changes=0;
 std::weak_ptr<const void>registered_lifetime;
};
struct BnwNativeImportedMissionRuntimeObservation {
 std::int32_t owner=-1,full_id=-1,mission_timer=0,combat_city_owner=-1,combat_city_id=-1;
 std::size_t visual_locations=0;bool busy=false;
 std::weak_ptr<const void>registered_lifetime;
 // Partial actual OriginalRead plain storage, never full imported Read or a
 // timer/visual/mission store grant and never a substitute for a producer.
};
struct BnwNativeImportedMissionObservation {
 struct Mission {std::int32_t type=-1,data1=0,data2=0,flags=0,push_turn=0;bool operator==(const Mission&)const=default;};
 std::vector<Mission>queue;
 std::int32_t owner=-1,full_id=-1,unit_flags=0,activity=-1;
 // Ordered decoded native CLinkList Read data only; not PushMission or a
 // ContinueMission/UnitPathTo/phase/GUI permission.
};
struct BnwNativeContinueMissionEntryObservation {
 std::int32_t owner=-1,full_id=-1;
 std::uint64_t path_clear_operation=0;
 // Actual first ClearPathCache effect ONLY. Saved evaluating Flags|1 and
 // archive/checkpoint/body/exit remain mandatory, never an entered mission.
};
struct BnwNativeContinueMissionPhaseObservation {
 std::int32_t owner=-1,full_id=-1,flags=0,loaded_flags=0;
 BnwUnitMissionContinuationPhase phase=BnwUnitMissionContinuationPhase::cache_cleared_before_evaluating_flags;
 std::uint64_t path_clear_operation=0,archive_changes=0;
 bool archive_dirty=false;std::weak_ptr<const void>registered_lifetime;
 // Actual phase copy only. Head/body/exit/SetCombatWarned are not completed.
};
struct BnwNativeContinueMissionEmptyHeadReturnObservation {
 std::int32_t owner=-1,full_id=-1,flags=0;
 std::uint64_t path_clear_operation=0,archive_changes=0,game_store_ordinal=0;
 bool archive_dirty=false;
 // Diagnostic copy of real NULL-head/equal-AWAKE SDK return only. It grants
 // no nonempty mission, changed waking, saved restore, or full turn readiness.
};
enum class BnwNativeUnitPathToBoundary {returned_no_move,destination_admission,unit_move,rejected_path_visualization,destination_capture_admission,failed_unit_move_visualization,movement_store_graphical_owner};
struct BnwNativeUnitPathToObservation {
 BnwNativeUnitPathToBoundary boundary=BnwNativeUnitPathToBoundary::returned_no_move;
 std::optional<std::int32_t>native_return;
 std::optional<std::uint64_t>first_plot;
 std::optional<std::int32_t>move_cost,moves_before,moves_requested;
 // COPY of earned body prefix; no completion/store/queue authority. A
 // mandatory boundary has no native_return, even if native would later0.
};
struct BnwNativeUnitIdAssignment {
 std::int32_t player=-1,slot=-1,full_id=-1,serial_before=0,serial_after=0;
 // Actual typed allocation prefix only; no playable-unit/world publication.
};
struct BnwNativeTraitFreeUnitSelection {
 std::int32_t player=-1,unit_class=-1,unit=-1,unit_ai=-1;
 std::int64_t technology=-1;
};
class BnwUnitFlagLivingHealthRead;class BnwNativeAttritionPrefix;class BnwNativeUnitAllocatorCore;class BnwRetainedTurnCursor;class BnwBuildingHappinessRules;struct SessionCheckpointIdentity;
// Only a real Core-issued native predicate call can construct this return.
// Public lifetime is harmless: no public result/issuer or continuation factory.
enum class BnwAttritionPrefixBoundary {minor_open_borders_owner,localized_damage_owner,mountain_domain_owner,active_deferred_fog_owner,active_count_owner};
struct BnwAttritionPrefixProgress {
 bool issued=false,executed=false,rng_stored=false,outer_returned=false;
 std::optional<std::int32_t>unit,slot;std::optional<std::uint64_t>roll;
 std::uint64_t before_revision=0,after_revision=0;
 BnwAttritionPrefixBoundary boundary=BnwAttritionPrefixBoundary::mountain_domain_owner;
};
// One authentic retained CvPlayer::DoUnitAttrition entry. It holds the first
// BeginIter/GetAt result and exact reached territory branches. No DTO can
// mint its RNG effect, claim a finished unit, or continue later effects.
struct BnwAttritionMountainProgress {
 bool issued=false,executed=false,damage_required=false;
 std::optional<std::int32_t>unit,domain,plot_type,all_terrain_count;
 std::uint64_t revision=0;
};
class BnwNativeAttritionPrefix final {
 friend class BnwUnitFlagLivingHealthRead;
 public:~BnwNativeAttritionPrefix();
 BnwNativeAttritionPrefix(const BnwNativeAttritionPrefix&)=delete;
 BnwNativeAttritionPrefix&operator=(const BnwNativeAttritionPrefix&)=delete;
 private:
 friend class BnwNativeUnitAllocatorCore;friend class BnwRetainedTurnCursor;friend class BnwSelectedStockLuaSourceOwner;
 friend class BnwAttritionCityInfoDirtyLoan;
 friend class BnwAttritionUnitInfoDirtyLoan;
 friend class BnwAttritionUnitSelectedRead;
 friend class BnwAttritionSelectedUnitInfoDirtyLoan;
 friend class BnwAttritionSelectionPlotRead;
 friend class BnwAttritionPlotListDirtyLoan;
 friend class BnwAttritionDamageDllWrapperPrefix;
 struct Impl;std::shared_ptr<Impl>impl_;explicit BnwNativeAttritionPrefix(std::shared_ptr<Impl>);
 void require_current_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
  const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool)const;
 void execute_native_without_callbacks(LiveSessionState&)const;
 BnwAttritionPrefixProgress progress_without_callbacks()const;
 void prepare_mountain_owned(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
  const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
 void require_mountain_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
  const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool)const;
 void execute_mountain_without_callbacks(const LiveSessionState&)const;
 BnwAttritionMountainProgress mountain_progress_without_callbacks()const;
 void prepare_damage_owned(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
  const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const std::weak_ptr<ui::BnwEngineUiTurnOwner>&)const;
 void require_damage_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
  const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool)const;
 void execute_damage_without_callbacks(LiveSessionState&)const;
 BnwAttritionDamageProgress damage_progress_without_callbacks()const;
 void fail_damage_without_callbacks()const noexcept;
 void prepare_garrison_owned(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
 void require_garrison_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool)const;
 void execute_garrison_without_callbacks(const LiveSessionState&)const;
 BnwAttritionGarrisonProgress garrison_progress_without_callbacks()const;
 void fail_garrison_without_callbacks()const noexcept;
 void prepare_city_strength_owned(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
 void require_city_strength_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool)const;
 void execute_city_strength_without_callbacks(LiveSessionState&)const;
 std::uint64_t damage_descendant_revision_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 BnwAttritionCityStrengthProgress city_strength_progress_without_callbacks()const;
 void fail_city_strength_without_callbacks()const noexcept;
 std::shared_ptr<const BnwAttritionCityInfoDirtyLoan> prepare_city_info_owned(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const BnwRetainedTurnCursor&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
 void require_unit_info_parent_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
 std::shared_ptr<const BnwAttritionUnitInfoDirtyLoan> prepare_unit_info_owned(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const BnwRetainedTurnCursor&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
 void require_unit_selected_parent_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
 std::shared_ptr<const BnwAttritionUnitSelectedRead> prepare_unit_selected_owned(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const BnwRetainedTurnCursor&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
 void require_selected_unit_info_parent_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
 std::shared_ptr<const BnwAttritionSelectedUnitInfoDirtyLoan> prepare_selected_unit_info_owned(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const BnwRetainedTurnCursor&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
 void require_selection_plot_parent_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
 std::pair<std::shared_ptr<ui::BnwClientSelectionControl>,std::optional<std::int32_t>> selection_plot_input_without_callbacks()const;
 std::shared_ptr<const BnwAttritionSelectionPlotRead> prepare_selection_plot_owned(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const BnwRetainedTurnCursor&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
 void require_plot_list_parent_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
 void require_damage_wrapper_parent_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
 std::shared_ptr<const BnwAttritionDamageDllWrapperPrefix> prepare_damage_wrapper_owned(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const BnwRetainedTurnCursor&)const;
 std::shared_ptr<const BnwAttritionPlotListDirtyLoan> prepare_plot_list_owned(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const BnwRetainedTurnCursor&,const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
};
class BnwNativePlayerBusyUnitsReturn final {
 public:
 ~BnwNativePlayerBusyUnitsReturn();
 BnwNativePlayerBusyUnitsReturn(const BnwNativePlayerBusyUnitsReturn&)=delete;
 BnwNativePlayerBusyUnitsReturn&operator=(const BnwNativePlayerBusyUnitsReturn&)=delete;
 private:
 friend class BnwNativeUnitAllocatorCore;friend class BnwSelectedStockLuaSourceOwner;friend class BnwRetainedTurnCursor;friend class BnwNativePlayerBusyCitiesReturn;
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwNativePlayerBusyUnitsReturn(std::shared_ptr<Impl>);
  void require_bound_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,
   const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,
   std::optional<std::uint64_t>,const BnwRetainedTurnCursor*,bool after_auto=false)const;
  void require_after_busy_end_turn_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,
   const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const BnwRetainedTurnCursor&)const;
  void require_after_processed_auto_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,
   const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const BnwRetainedTurnCursor&)const;
 void require_current_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,
  const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
 void quarantine_without_callbacks()const noexcept;
 bool completed_without_callbacks()const noexcept;bool busy_without_callbacks()const noexcept;
 std::uint64_t queried_without_callbacks()const noexcept;std::int32_t required_id_without_callbacks()const noexcept;
};
class BnwNativePlayerBusyCitiesReturn final {
 public:
 ~BnwNativePlayerBusyCitiesReturn();
 BnwNativePlayerBusyCitiesReturn(const BnwNativePlayerBusyCitiesReturn&)=delete;
 BnwNativePlayerBusyCitiesReturn&operator=(const BnwNativePlayerBusyCitiesReturn&)=delete;
 private:
 friend class BnwNativeUnitAllocatorCore;friend class BnwSelectedStockLuaSourceOwner;friend class BnwRetainedTurnCursor;
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwNativePlayerBusyCitiesReturn(std::shared_ptr<Impl>);
  void require_bound_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,
   const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,
   std::optional<std::uint64_t>,const BnwRetainedTurnCursor*,bool after_auto=false)const;
  void require_after_busy_end_turn_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,
   const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const BnwRetainedTurnCursor&)const;
  void require_after_processed_auto_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,
   const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,const BnwRetainedTurnCursor&)const;
 void require_current_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,
  const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
 void quarantine_without_callbacks()const noexcept;
 bool completed_without_callbacks()const noexcept;bool busy_without_callbacks()const noexcept;
 std::uint64_t queried_without_callbacks()const noexcept;std::int32_t required_id_without_callbacks()const noexcept;
};
namespace detail {
enum class BnwBuildingHappinessReachedStage;
std::int32_t bnw_happiness_buildings_reached_math_for_initial_unit(const LiveSessionState&,
 const BnwBuildingHappinessRules&,const SessionCheckpointIdentity&,std::int64_t,
 BnwBuildingHappinessReachedStage,const BnwNativeUnitAllocatorCore&);
}
// Copy of the actual native common-search result, without action authority.
struct BnwNativeExistingUnitPathObservation {
 struct Node {std::uint64_t plot=0;std::int32_t x=-1,y=-1,moves=0,turns=0,known=0,total=0;bool operator==(const Node&)const=default;};
 bool found=false;std::vector<Node>nodes;bool operator==(const BnwNativeExistingUnitPathObservation&)const=default;
};
class BnwNativeUnitAllocatorCore final {
 friend class BnwPendingCreatedUnitRead;
 friend class BnwUnitFlagLivingHealthRead;
 friend class BnwAttritionDamageDllWrapperPrefix;
 public:
 ~BnwNativeUnitAllocatorCore();
 BnwNativeUnitAllocatorCore(const BnwNativeUnitAllocatorCore&)=delete;
 BnwNativeUnitAllocatorCore&operator=(const BnwNativeUnitAllocatorCore&)=delete;
 static std::shared_ptr<const BnwNativeUnitAllocatorCore> from_selected_preview(
  const ui::SelectedBnwReadOnlyPreview&,std::int32_t player);
 void require_current()const;
 // Genuine SAME-bank earned SetID->abort history only; a partial diagnostic
 // document, never a birth/full-save or continuation receipt. No caller data.
 saves::CivisSaveDocument checkpoint_native_aborted_unit_serial()const;
 // Only genuine completed popOrder/CreateUnit source may bind TRAIN args.
 void bind_completed_train_source(const ui::SelectedCityTrainCreateSource&)const;
 void bind_trait_iterator_from_selected_preview(const ui::SelectedBnwReadOnlyPreview&)const;
 // Standalone allocator interval; processTech must obtain actual placement
 // BEFORE invoking it. No external DTO/boolean authorizes creation.
 BnwNativeUnitSlotReservation reserve_constructor_reset_prefix()const;
 BnwNativeUnitConstructorStorageSnapshot execute_constructor_storage_tail()const;
 BnwNativeUnitArchiveObservation pending_constructor_archive()const;
 // Abort only an unpublished constructor reservation; no kill/world deletion.
 void discard_unpublished_constructor()const;
 // Derives all numbers from genuine raw primary serial+SAME reserved slot.
 BnwNativeUnitIdAssignment assign_pending_unit_native_id()const;
 // Executes Init promotions/religion and Reset(false) from the SAME genuine
 // internal Trait selection and actual HasTech poststate, through native
 // indexed cache reset. Stops BEFORE hovering promotions/setXY/publication.
 BnwNativeUnitConstructorStorageSnapshot execute_selected_trait_unit_init_reset(const ui::SelectedBnwReadOnlyPreview&)const;
 BnwNativeUnitConstructorStorageSnapshot execute_bound_unit_init_reset(const ui::SelectedBnwReadOnlyPreview&)const;
 // Actual default Init branch on SAME pending node; reached free-hover
 // promotion requires its real native effects before this prefix may store.
 BnwNativeUnitConstructorStorageSnapshot execute_bound_unit_initial_facing_hover_prefix()const;
 // Creates the existing SetXY owner from genuine unpublished TRAIN storage.
 // It executes actual initial popup prefix, never publishes a roster early.
 std::shared_ptr<const BnwUnitMovePending> begin_initial_set_xy_prefix()const;
 // Executes genuine destination collision checks and only then SAME-node
 // coordinate stores. Actual city visibility callback is the next boundary.
 std::shared_ptr<const BnwUnitMovePending> advance_initial_destination_collision_prefix()const;
 // Native GetAt read on the actual imported actor manager. Does not grant Init.
 std::optional<std::int32_t> read_native_existing_unit_id(std::int32_t query_id)const;
 // Actual SDK firstUnit/nextUnit ascending native slots, IsCombatUnit raw
 // m_iBaseCombat>0. Query result alone is never a DoDeficit action permission.
 std::int32_t read_native_existing_combat_unit_count()const;
 // Actual SDK CvPlot count for a genuine imported manager object and target
 // plot. Defaults to native early-break mode; numeric query grants no action.
 std::optional<std::int32_t>read_native_existing_friendly_stack_count(std::int32_t query_id,std::uint64_t plot,bool break_on_limit=true)const;
 // Actual SDK current-plot under-limit branch. Overflow requires the real
 // mutable two-layer destination-search owner and fails explicitly.
 std::optional<bool>read_native_existing_has_no_valid_move(std::int32_t query_id)const;
 // Executes the actual SAME common two-layer tree using native callbacks.
 // Observation only: does not publish LastPath, move orders or world stores.
 std::optional<BnwNativeExistingUnitPathObservation>read_native_existing_unit_path(std::int32_t query_id,std::uint64_t target,bool reuse=false)const;
 // Original Unit::GeneratePath effects: retained destination, CopyPath and
 // native visibility flags. Does not issue a move/mission or relocate unit.
 std::optional<BnwNativeExistingUnitPathObservation>advance_native_existing_generate_path_cache(std::int32_t query_id,std::uint64_t target,bool reuse=false)const;
 // Diagnostic COPY of authentic retained storage; cannot authorize a move.
 std::optional<BnwUnitRetainedPath>read_native_existing_retained_path_cache(std::int32_t query_id)const;
 // Actual CvUnit::UpdatePathCache(flags=0), on the SAME retained path owner.
 // Neither a positive path nor this result grants UnitMove/setXY effects.
 std::optional<bool>advance_native_existing_update_path_cache(std::int32_t query_id,std::uint64_t target)const;
 // Diagnostic GetPathFirstPlot; detached index is not movement authority.
 std::optional<std::uint64_t>read_native_existing_path_first_plot(std::int32_t query_id)const;
 // Lazy genuine original-Read field registration on the canonical imported
 // object. Does not mutate moves, authorize UnitMove or rehydrate other fields.
 std::optional<BnwNativeImportedMovesArchiveObservation>bind_native_existing_moves_archive(std::int32_t query_id)const;
 // Original Read scalar/plain registration only. Not a SetDamage/effect grant.
 std::optional<BnwNativeImportedDamageArchiveObservation>bind_native_existing_damage_archive(std::int32_t query_id)const;
 // Actual canonical retained first-step MovementCost source adapter. No
 // observed/copied path or numeric result grants a store or movement command.
 std::optional<std::int32_t>read_native_existing_retained_step_cost(std::int32_t query_id)const;
 std::optional<BnwNativeImportedMissionObservation>read_native_existing_mission_queue(std::int32_t query_id)const;
 std::optional<bool>read_native_existing_is_doing_partial_move(std::int32_t query_id)const;
 std::optional<bool>read_native_existing_ready_to_auto(std::int32_t query_id)const;
 std::optional<BnwNativeImportedMissionRuntimeObservation>read_native_existing_mission_runtime(std::int32_t query_id)const;
 std::optional<BnwNativeContinueMissionEntryObservation>advance_native_existing_continue_mission_cache_prefix(std::int32_t query_id)const;
 // Consumes SAME earned ClearPathCache journal; real Flags|1 FAutoVariable
 // store precedes mission head. Equal store earns phase without dirty touch.
 BnwNativeContinueMissionPhaseObservation advance_native_existing_continue_mission_flags_prefix(std::int32_t full_id)const;
 // Genuine NULL head + unchanged ACTIVITY_AWAKE only; changed waking and
 // Game::SetCombatWarned remain mandatory owners. Never full native return.
 BnwNativeContinueMissionPhaseObservation advance_native_existing_continue_mission_empty_head_exit_flags_prefix(std::int32_t full_id)const;
 BnwNativeContinueMissionEmptyHeadReturnObservation advance_native_existing_continue_mission_empty_head_native_return(std::int32_t full_id)const;
 // Only genuine canonical pending entry mints the private section43 codec.
 std::shared_ptr<const BnwUnitMissionContinuationCheckpoint>checkpoint_native_existing_continue_mission(std::int32_t full_id)const;
 // Fresh original-import physical owner rebind. This restores the already
 // earned phase, never reruns clear/Flags or claims mission/body completion.
 BnwNativeContinueMissionPhaseObservation restore_native_existing_continue_mission(const saves::CivisSaveSection&)const;
 // Actual original UnitPathTo flags0/non-route prefix. Native early0 and
 // cache effects are real; positive UnitMove/UI/graphical boundary remains
 // privately retained until its genuine consumers finish in source order.
 std::optional<BnwNativeUnitPathToObservation>advance_native_existing_unit_path_to_prefix(std::int32_t query_id,std::int32_t x,std::int32_t y,std::int32_t flags=0,std::int32_t previous_eta=-1)const;
 // Consumes only SAME privately retained UnitPathTo body. Actual destination
 // and UnitMove admission/cost precede missing imported setMoves/ShouldDim/UI.
 std::optional<BnwNativeUnitPathToObservation>advance_native_existing_unit_move_admission(std::int32_t query_id)const;
 // Genuine final original Init SDK publisher sequence after minor quests.
 std::shared_ptr<const BnwUnitMovePending>advance_initial_setup_graphical_enqueue(
  const graphics::BnwInitialUnitSetupGraphicalRendererLoan&,const BnwStockLuaAttemptLease&)const;
 // Closed weak canonical per-actor provider for the actual SAME world.
 BnwPlotUnitReadServices bind_native_existing_plot_unit_reads(BnwPlotUnitReadServices)const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_city_visibility_prefix(
  const std::shared_ptr<graphics::BnwPendingUnitVisualOwner>&)const;
 // Actual native numeric getter; never grants placement or an Init phase.
 std::int32_t read_native_garrison_policy_culture()const;
 // Real hidden-slot predecessor, policy scalar store, exact fortify equality.
 std::shared_ptr<const BnwUnitMovePending> advance_initial_city_garrison_fortify_prefix()const;
 // SAME newborn new-only ring: actual counters with no reached receiver edge.
 std::shared_ptr<const BnwUnitMovePending> advance_initial_no_receiver_sight_prefix()const;
 // Genuine journal-quarantined plot list/area insertion; not full Init/public roster.
 std::shared_ptr<const BnwUnitMovePending> advance_initial_plot_area_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_city_strength_prefix()const;
 // Actual own-city capture/load/meet/adjacency checks before native SetPosition.
 std::shared_ptr<const BnwUnitMovePending> advance_initial_post_city_checks_prefix()const;
 // Real renderer world-position store and SAME hidden-slot issuer proof.
 std::shared_ptr<const BnwUnitMovePending> advance_initial_set_position_prefix(
  const std::shared_ptr<graphics::BnwPendingUnitVisualOwner>&)const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_unit_info_dirty_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_enemy_city_range_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_happiness_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_happiness_native_suffix_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_happiness_return()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_popup_release_and_garrison()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_center_unit_visibility(
  const graphics::BnwInitialCenterUnitRendererLoan&,const BnwStockLuaAttemptLease&)const;
 // Unsaved native weak-center cache born NULL, not an Original post-paint import.
 std::optional<std::pair<std::int32_t,std::int32_t>> read_native_cached_plot_center(std::int32_t x,std::int32_t y)const;
 // SDK Init406-442 ordinary no-unique-name branch, actual clock and Game count.
 // Unitclass/team/player counters and all later Init effects remain mandatory.
 std::shared_ptr<const BnwUnitMovePending> advance_initial_name_game_counter_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_game_class_counter_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_team_class_counter_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_player_class_counter_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_support_zero_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_resource_nuke_zero_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_military_support_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_unit_info_promotion_prefix()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_unit_info_promotions_return()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_traits_promotions_return()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_player_promotions_return()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_embark_promotions_return()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_exotic_xp_return()const;
 std::shared_ptr<const BnwUnitMovePending> advance_initial_immobile_max_moves_prefix()const;
 // Genuine native setMoves: changed value stores before the actual Game/queue
 // call. This stops before selected UnitInfo/PlotListButtons and later Init.
 std::shared_ptr<const BnwUnitMovePending> advance_initial_set_moves_and_should_dim_enqueue(
  const graphics::BnwInitialUnitShouldDimRendererLoan&,const BnwStockLuaAttemptLease&)const;
 std::shared_ptr<const BnwUnitMovePending>advance_initial_set_moves_selection_and_plot_dirty_tail()const;
 // Native religious Init data only; tourism/recon/full Init remain mandatory.
 std::shared_ptr<const BnwUnitMovePending>advance_initial_religion_prefix()const;
 std::shared_ptr<const BnwUnitMovePending>advance_initial_tourism_prefix()const;
 std::shared_ptr<const BnwUnitMovePending>advance_initial_recon_prefix()const;
 std::shared_ptr<const BnwUnitMovePending>advance_initial_best_land_prefix()const;
 std::shared_ptr<const BnwUnitMovePending>advance_initial_active_game_data_prefix()const;
 std::shared_ptr<const BnwUnitMovePending>advance_initial_unit_cycler_prefix()const;
 std::shared_ptr<const BnwUnitMovePending>advance_initial_world_unit_message_prefix()const;
 std::shared_ptr<const BnwUnitMovePending>advance_initial_second_city_strength_prefix()const;
 std::shared_ptr<const BnwUnitMovePending>advance_initial_army_ai_prefix()const;
 std::shared_ptr<const BnwUnitMovePending>advance_initial_maintenance_prefix()const;
 std::shared_ptr<const BnwUnitMovePending>advance_initial_minor_quest_prefix()const;
 void require_completed_unit_initialization()const;
 // Native First begins an internal iterator from the real rebuilt cache.
 // A nonempty result stops BEFORE addFreeUnit placement/RNG/constructor.
 BnwNativeTraitFreeUnitSelection first_trait_free_unit(std::int64_t technology)const;
 BnwNativeTraitFreeUnitSelection next_trait_free_unit()const;
 private:

 friend class LiveSession;
 friend class BnwTwoLayerUnitPathFinder;
 void require_native_common_pathfinder_session_without_callbacks(const LiveSession&)const;
 void require_native_common_pathfinder_binding(const LiveSession&)const;
 struct ExistingUnitPathCache {
  std::array<std::int32_t,5>base_moves{};std::int32_t max_moves=0,denominator=0,owner=-1,team=-1,domain=-1;
  bool human=false,automated=false,immobile=false,no_reveal=false,can_embark=false,embarked=false,can_attack=false,combat=false;
  bool operator==(const ExistingUnitPathCache&)const=default;
 };
 ExistingUnitPathCache prepare_existing_unit_path_cache(const saves::SavedUnitObservation&)const;
 // SAME actual imported unit admission; called only by guarded Core bodies.
 // Native flags0/DESTINATION4/PRETEND_CORRECT128, no public predicate grant.
 bool existing_native_can_move_into(const saves::SavedUnitObservation&,std::uint64_t,const ExistingUnitPathCache&,std::uint8_t)const;
 bool existing_native_is_in_combat(const saves::SavedUnitObservation&)const;
 enum class ExistingPathConsumer {observation,native_retained_cache};
 std::optional<BnwNativeExistingUnitPathObservation>generate_native_existing_unit_path(std::int32_t,std::uint64_t,bool,ExistingPathConsumer)const;
 friend class BnwNativeUnitPathMovementCost;
 class ExistingPathMovementCostDispatch final {
  friend class BnwNativeUnitAllocatorCore;
  struct Storage;std::shared_ptr<const Storage>storage_;
  ExistingPathMovementCostDispatch(const BnwNativeUnitAllocatorCore&,std::shared_ptr<const BnwTwoLayerUnitPathFinder>,const saves::SavedUnitObservation&,const BnwTwoLayerUnitPathFinder::Node&,const BnwTwoLayerUnitPathFinder::Node&,const ExistingUnitPathCache&);
  ExistingPathMovementCostDispatch(const BnwNativeUnitAllocatorCore&,std::shared_ptr<const BnwTwoLayerUnitPathFinder>,const saves::SavedUnitObservation&,const BnwUnitPathOwner&,const BnwUnitRetainedPath&,const ExistingUnitPathCache&);
 };
 struct ExistingPathMovementCostOperands {
  std::shared_ptr<const ui::SelectedBnwPreviewHost>host;std::shared_ptr<const LiveSessionState>state;const saves::SavedSession*source=nullptr;const saves::SavedUnitObservation*unit=nullptr;
  std::pair<std::int64_t,std::int64_t>key;std::uint64_t generation=0,from=0,to=0;
  std::int32_t team=-1,base=0,max_moves=0,denominator=0,remaining=0;bool human=false;
  bool operator==(const ExistingPathMovementCostOperands&)const=default;
 };
 ExistingPathMovementCostOperands existing_path_movement_cost_operands(const ExistingPathMovementCostDispatch&)const;
 bool existing_path_slowed_by_zoc(const ExistingPathMovementCostDispatch&)const;
 friend class graphics::BnwInitialUnitShouldDimRendererLoan;
 friend class graphics::BnwInitialUnitShouldDimEnqueueProof;
 friend class ui::BnwClientSceneFrame;
 class ShouldDimCoreBorrowedOwner;
 class InitialUnitShouldDimPlan final {
  friend class BnwNativeUnitAllocatorCore;
  friend class graphics::BnwInitialUnitShouldDimRendererLoan;
  friend class graphics::BnwInitialUnitShouldDimEnqueueProof;
 public: ~InitialUnitShouldDimPlan();
 private:
  InitialUnitShouldDimPlan(const InitialUnitShouldDimPlan&)=delete;
  InitialUnitShouldDimPlan&operator=(const InitialUnitShouldDimPlan&)=delete;
  InitialUnitShouldDimPlan(InitialUnitShouldDimPlan&&)=delete;
  InitialUnitShouldDimPlan&operator=(InitialUnitShouldDimPlan&&)=delete;
  struct Storage;std::shared_ptr<Storage>storage_;
  explicit InitialUnitShouldDimPlan(std::shared_ptr<Storage>);
  struct Binding {
   const BnwNativeUnitAllocatorCore*core=nullptr;
   std::weak_ptr<const BnwNativeUnitAllocatorCore>core_control;
   std::shared_ptr<const ui::SelectedBnwPreviewHost>host;
   LiveSession*session=nullptr;std::shared_ptr<const LiveSessionState>state;
   const saves::SavedSession*source=nullptr;
   std::shared_ptr<const BnwUnitMovePending>pending;
   const void*bank=nullptr;std::weak_ptr<const void>bank_control;
   const void*node=nullptr;std::weak_ptr<const void>node_control;
   std::int32_t owner=-1,full_id=-1,slot=-1;
   std::uint64_t generation=0,world_revision=0,attempt_serial=0;
   std::int32_t before_moves=0,requested_moves=0;
  };
  class BorrowedOwner final {
   friend class graphics::BnwInitialUnitShouldDimRendererLoan;
   friend class graphics::BnwInitialUnitShouldDimEnqueueProof;
   explicit BorrowedOwner(const InitialUnitShouldDimPlan&);
   explicit BorrowedOwner(std::shared_ptr<const InitialUnitShouldDimPlan>);
   static std::unique_ptr<BorrowedOwner>from_weak(const std::weak_ptr<const InitialUnitShouldDimPlan>&);
   const InitialUnitShouldDimPlan&current()const;
   std::shared_ptr<const InitialUnitShouldDimPlan>owner_;
  public:
   ~BorrowedOwner();
   BorrowedOwner(const BorrowedOwner&)=delete;BorrowedOwner&operator=(const BorrowedOwner&)=delete;
   BorrowedOwner(BorrowedOwner&&)=delete;BorrowedOwner&operator=(BorrowedOwner&&)=delete;
  };
  std::shared_ptr<const InitialUnitShouldDimPlan>retain_factory_owner()const;
  void require_owner_without_callbacks()const;
  void retire_scope_without_callbacks()const noexcept;
  Binding capture_binding()const;
  void require_before_moves_store_without_callbacks()const;
  void require_should_dim_enqueue_dispatch_without_callbacks()const;
  std::int32_t read_dispatch_owner_without_callbacks()const;
  std::int32_t read_dispatch_full_id_without_callbacks()const;
  bool read_dispatch_should_dim_without_callbacks()const;
 };

 class InitialUnitSetupGraphicalPlan;
 class ShouldDimCoreBorrowedOwner final {
  friend class BnwNativeUnitAllocatorCore;
  friend class BnwPendingCreatedUnitRead;
  friend class BnwUnitFlagLivingHealthRead;
  friend class BnwAttritionDamageDllWrapperPrefix;
  friend class ::civ5::runtime::BnwNativeAttritionPrefix;
  // SourceOwner consumes the SAME counted Core inventory. Weak checker locks
  // must move into this owner; temporary retention cannot mask retirement.
  friend class BnwSelectedStockLuaSourceOwner;
  friend class BnwMinorCityStrengthScratch;
  friend class InitialUnitShouldDimPlan;
  friend class BnwUnitCombatWarnedRestorePlan;
  friend class InitialUnitSetupGraphicalPlan;
  friend class graphics::BnwInitialUnitSetupGraphicalRendererLoan;
  friend class graphics::BnwInitialUnitSetupGraphicalEnqueueProof;
  friend class graphics::BnwInitialUnitShouldDimRendererLoan;
  friend class graphics::BnwInitialUnitShouldDimEnqueueProof;
  explicit ShouldDimCoreBorrowedOwner(std::shared_ptr<const BnwNativeUnitAllocatorCore>);
  static std::unique_ptr<ShouldDimCoreBorrowedOwner>from_weak(const std::weak_ptr<const BnwNativeUnitAllocatorCore>&);
  const BnwNativeUnitAllocatorCore&current()const;
  std::shared_ptr<const BnwNativeUnitAllocatorCore>owner_;
 public:
  ~ShouldDimCoreBorrowedOwner();
  ShouldDimCoreBorrowedOwner(const ShouldDimCoreBorrowedOwner&)=delete;
  ShouldDimCoreBorrowedOwner&operator=(const ShouldDimCoreBorrowedOwner&)=delete;
  ShouldDimCoreBorrowedOwner(ShouldDimCoreBorrowedOwner&&)=delete;
  ShouldDimCoreBorrowedOwner&operator=(ShouldDimCoreBorrowedOwner&&)=delete;
 };
 // The graphics Loan's ONE retained Core pin, this producer's call pin and
 // each Plan->Core pure checker pin share exactly this inventory. Temporary
 // pins cannot attest an independently alive session facade owner.
 mutable std::atomic<std::size_t>should_dim_borrowed_core_pins_{0};
 void require_should_dim_outside_owner_without_callbacks()const;

 void require_initial_should_dim_plan_without_callbacks(const InitialUnitShouldDimPlan&,bool)const;
 void require_initial_set_moves_stored_without_callbacks()const;
 bool initial_immobile_moves_node_matches_without_callbacks()const;
 // Scene post-callback acceptance uses this earned return, after the issuing
 // Plan scope has retired. It never renews an old Lua scope or Plan lifetime.
 void require_initial_should_dim_return_without_callbacks(const BnwUnitMovePending&)const;
 void require_initial_set_moves_tail_return_without_callbacks()const;
 // Owner-authenticated frontier for closed TRAIN/Scene admission. This is
 // not a completion token and cannot grant queue/cache/fanout/full Init.
 BnwUnitMoveBoundary initial_set_moves_earned_frontier_without_callbacks(const BnwUnitMovePending&)const;
 void require_initial_religion_return_without_callbacks()const;
 void require_initial_tourism_return_without_callbacks()const;
 void require_initial_recon_return_without_callbacks()const;
 void require_initial_best_land_return_without_callbacks()const;
 std::uint64_t initial_best_land_prior_world_revision_without_callbacks()const;
 bool initial_best_land_prior_unit_dirty_matches_without_callbacks(const std::shared_ptr<ui::BnwClientSelectionControl>&,const ui::BnwClientUnitDirtySnapshot&)const;
 std::uint64_t initial_moves_prior_input_revision_without_callbacks(const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
 struct InitialMovesDirtyOperands {
  std::int32_t owner,full_id;std::uint64_t generation,world_revision,input_revision,occurrence;
  ui::BnwClientUnitDirtyKind kind;
 };
 InitialMovesDirtyOperands initial_moves_dirty_operands(const ui::BnwClientSelectionControl&,ui::BnwClientUnitDirtyKind)const;
 void record_initial_moves_ui_dirty_store(ui::BnwClientUnitDirtyKind,std::uint64_t)const noexcept;
 InitialMovesDirtyOperands initial_best_land_dirty_operands(const ui::BnwClientSelectionControl&)const;
 void record_initial_best_land_dirty_store(std::uint64_t)const noexcept;
 struct InitialUnitGameDataDirtyOperands {
  std::int32_t owner,full_id;std::uint64_t generation,world_revision,input_revision,setters_before;std::optional<bool>flag_before;
 };
 InitialUnitGameDataDirtyOperands initial_unit_game_data_dirty_operands(const ui::BnwClientSelectionControl&)const;
 void record_initial_unit_game_data_dirty_store()const noexcept;
 bool initial_unit_game_data_prior_unit_dirty_matches_without_callbacks(const std::shared_ptr<ui::BnwClientSelectionControl>&,const ui::BnwClientUnitDirtySnapshot&)const;
 void require_initial_active_game_data_return_without_callbacks()const;
 void require_initial_unit_cycler_return_without_callbacks()const;
 void require_initial_world_unit_message_return_without_callbacks()const;
 void require_initial_second_city_strength_return_without_callbacks()const;
 void require_initial_army_ai_return_without_callbacks()const;
 enum class InitialUnitMaintenanceDirtyKind {city_info,game_data};
 struct InitialUnitMaintenanceDirtyOperands {
  std::int32_t owner,full_id;std::uint64_t generation,world_revision,input_revision;
  InitialUnitMaintenanceDirtyKind kind;
  std::optional<bool> city_flag,game_flag;std::uint64_t city_count,game_count;
 };
 InitialUnitMaintenanceDirtyOperands initial_unit_maintenance_dirty_operands(const ui::BnwClientSelectionControl&)const;
 void record_initial_unit_maintenance_dirty_store(InitialUnitMaintenanceDirtyKind)const noexcept;
 bool initial_maintenance_prior_unit_dirty_matches_without_callbacks(const std::shared_ptr<ui::BnwClientSelectionControl>&,const ui::BnwClientUnitDirtySnapshot&)const;
 bool initial_maintenance_prior_game_data_matches_without_callbacks(const std::shared_ptr<ui::BnwClientSelectionControl>&,std::optional<bool>,std::uint64_t,std::uint64_t)const;
 void require_initial_maintenance_return_without_callbacks()const;
 void require_initial_minor_quest_return_without_callbacks()const;
 friend class graphics::BnwInitialUnitSetupGraphicalRendererLoan;
 friend class graphics::BnwInitialUnitSetupGraphicalEnqueueProof;
 class InitialUnitSetupGraphicalPlan final {
  friend class BnwNativeUnitAllocatorCore;
  friend class BnwPendingCreatedUnitRead;
  friend class graphics::BnwInitialUnitSetupGraphicalRendererLoan;
  friend class graphics::BnwInitialUnitSetupGraphicalEnqueueProof;
  struct Storage;std::shared_ptr<Storage>storage_;
  explicit InitialUnitSetupGraphicalPlan(std::shared_ptr<Storage>);
 public:
  ~InitialUnitSetupGraphicalPlan();
  InitialUnitSetupGraphicalPlan(const InitialUnitSetupGraphicalPlan&)=delete;
  InitialUnitSetupGraphicalPlan&operator=(const InitialUnitSetupGraphicalPlan&)=delete;
  InitialUnitSetupGraphicalPlan(InitialUnitSetupGraphicalPlan&&)=delete;
  InitialUnitSetupGraphicalPlan&operator=(InitialUnitSetupGraphicalPlan&&)=delete;
 private:
  struct Binding {
   const BnwNativeUnitAllocatorCore*core=nullptr;
   std::weak_ptr<const BnwNativeUnitAllocatorCore>core_control;
   std::shared_ptr<const ui::SelectedBnwPreviewHost>host;
   LiveSession*session=nullptr;std::shared_ptr<const LiveSessionState>state;
   const saves::SavedSession*source=nullptr;
   std::shared_ptr<const BnwUnitMovePending>pending;
   const void*bank=nullptr;std::weak_ptr<const void>bank_control;
   const void*node=nullptr;std::weak_ptr<const void>node_control;
   std::int32_t owner=-1,full_id=-1,slot=-1,x=-1,y=-1;
   std::uint64_t generation=0,world_revision=0,input_revision=0,plot=0,attempt_serial=0;
  };
  class BorrowedOwner final {
   friend class BnwNativeUnitAllocatorCore;
   friend class graphics::BnwInitialUnitSetupGraphicalRendererLoan;
   friend class graphics::BnwInitialUnitSetupGraphicalEnqueueProof;
   explicit BorrowedOwner(const InitialUnitSetupGraphicalPlan&);
   explicit BorrowedOwner(std::shared_ptr<const InitialUnitSetupGraphicalPlan>);
   static std::unique_ptr<BorrowedOwner>from_weak(const std::weak_ptr<const InitialUnitSetupGraphicalPlan>&);
   const InitialUnitSetupGraphicalPlan&current()const;
   std::shared_ptr<const InitialUnitSetupGraphicalPlan>owner_;
  public:
   ~BorrowedOwner();
   BorrowedOwner(const BorrowedOwner&)=delete;BorrowedOwner&operator=(const BorrowedOwner&)=delete;
   BorrowedOwner(BorrowedOwner&&)=delete;BorrowedOwner&operator=(BorrowedOwner&&)=delete;
  };
  std::shared_ptr<const InitialUnitSetupGraphicalPlan>retain_factory_owner()const;
  void require_owner_without_callbacks()const;
  void retire_scope_without_callbacks()const noexcept;
  Binding capture_binding()const;
  // Callbackful genuine UnitInfo/defender preparation ONLY after Loan's
  // actual graphics true gate. False gate does not evaluate these operands.
  void prepare_native_readers()const;
  void require_setup_graphical_enqueue_dispatch_without_callbacks()const;
  std::int32_t read_dispatch_owner_without_callbacks()const;
  std::int32_t read_dispatch_full_id_without_callbacks()const;
  bool read_dispatch_embarked_without_callbacks()const;
  std::shared_ptr<const BnwPendingCreatedUnitRead>capture_created_pending_read_without_callbacks()const;
  std::int32_t read_dispatch_original_fog_without_callbacks()const;
  bool read_dispatch_original_not_invisible_without_callbacks()const;
  bool read_dispatch_should_dim_without_callbacks()const;
  std::int32_t read_dispatch_fortify_turns_without_callbacks()const;
  std::int32_t read_dispatch_damage_without_callbacks()const;
  bool read_dispatch_is_work_without_callbacks()const;
  std::int32_t read_dispatch_build_type_without_callbacks()const;
  bool read_dispatch_is_garrisoned_without_callbacks()const;
 };
 void require_initial_setup_graphical_plan_without_callbacks(const InitialUnitSetupGraphicalPlan&)const;
 // Earned original Init publisher return after scoped Plan retirement.
 // No cache, pump, Lua receipt or revival of a previous Attempt/Plan.
 void require_initial_setup_graphical_return_without_callbacks(const BnwUnitMovePending&)const;



 bool initial_army_prior_scalars_match_without_callbacks(const std::map<std::string,std::int32_t,std::less<>>&,const std::map<std::string,std::int32_t,std::less<>>&)const;
 bool initial_army_prior_dirty_match_without_callbacks(const std::set<std::size_t>&,const std::set<std::size_t>&)const;
 std::uint64_t initial_second_city_prior_world_revision_without_callbacks()const;
 bool initial_second_city_prior_events_match_without_callbacks(const std::vector<BnwCityDirtyEvent>&,std::uint64_t)const;
 bool initial_cycle_prior_scalars_matches_without_callbacks(const std::map<std::string,std::int32_t,std::less<>>&,const std::map<std::string,std::int32_t,std::less<>>&)const;

 friend class graphics::BnwInitialCenterUnitRendererLoan;
 friend class graphics::BnwInitialCenterUnitVisibilityProof;
 friend class ui::BnwClientSelectionControl;
 class InitialCenterUnitVisibilityPlan final {
  friend class BnwNativeUnitAllocatorCore;
  friend class graphics::BnwInitialCenterUnitRendererLoan;
  friend class graphics::BnwInitialCenterUnitVisibilityProof;
  InitialCenterUnitVisibilityPlan(const InitialCenterUnitVisibilityPlan&)=delete;
  InitialCenterUnitVisibilityPlan&operator=(const InitialCenterUnitVisibilityPlan&)=delete;
  struct Storage;std::shared_ptr<Storage> storage_;
  explicit InitialCenterUnitVisibilityPlan(std::shared_ptr<Storage>);
  struct Binding {
   const BnwNativeUnitAllocatorCore*core=nullptr;
   std::weak_ptr<const BnwNativeUnitAllocatorCore> core_control;
   std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
   LiveSession*session=nullptr;std::shared_ptr<const LiveSessionState> state;
   std::shared_ptr<const BnwUnitMovePending> pending;
   const saves::SavedSession*source=nullptr;
   const void*registry=nullptr;std::weak_ptr<const void> registry_control;
   const void*bank=nullptr;std::weak_ptr<const void> bank_control;
   std::int32_t active_player=-1,active_team=-1,x=-1,y=-1;
   std::uint64_t generation=0,world_revision=0,attempt_serial=0,plot=0;
  };
  struct Entry {
   std::int32_t owner=-1,asked_id=-1,full_id=-1,unit_type=-1;
   const saves::SavedUnitObservation*imported=nullptr;
   const void*native_node=nullptr;std::weak_ptr<const void> native_control;
   bool pending=false,resolved=false,visible=false;
   // Exact native arguments for every actual resolved roster entry.
   bool check_visibility=false;float duration=0.01f;
  };
  struct Setter {
   // -1 denotes the true null weak UnitHandle; otherwise an ordered Entry index.
   std::int32_t before=-1,after=-1;
   bool unit_info_dirty=false;
  };
  Binding capture_binding()const;
  const std::vector<Entry>& ordered_entries()const;
  const std::vector<Setter>& setter_journal()const;
  bool requires_visibility_fanout()const;
  void require_current_without_callbacks()const;
  void require_visibility_dispatch_without_callbacks()const;
 };
 void require_initial_center_plan_without_callbacks(const InitialCenterUnitVisibilityPlan&)const;
 struct InitialCenterDirtyOperands {std::int32_t owner,full_id;std::uint64_t generation,world_revision,input_revision;};
 InitialCenterDirtyOperands initial_center_dirty_operands(std::size_t setter)const;
 void record_initial_center_dirty_store(std::size_t,std::uint64_t)const noexcept;
 friend class ui::SelectedCityTrainCreateSource;
 friend class BnwSelectedStockLuaBootstrap;friend class BnwSelectedStockLuaSourceOwner;
 void require_live_gold_roster_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::int32_t actor)const;
 void require_live_gold_roster_source_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::int32_t actor)const;
  friend class BnwNativePlayerBusyUnitsReturn;friend class BnwNativePlayerBusyCitiesReturn;friend class BnwRetainedTurnCursor;
 friend class BnwNativeAttritionPrefix;
 void prepare_player_attrition_first_owned(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
  std::shared_ptr<const BnwSelectedStockLuaSourceOwner>,std::weak_ptr<const void>,std::uint64_t,std::int32_t,
  std::shared_ptr<const BnwNativeAttritionPrefix>&)const;
   void query_player_turn_busy_cities_owned(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
    std::shared_ptr<const BnwSelectedStockLuaSourceOwner>,std::weak_ptr<const void>,std::uint64_t,std::int32_t,
    std::shared_ptr<const BnwNativePlayerBusyUnitsReturn>,std::shared_ptr<const BnwNativePlayerBusyCitiesReturn>&)const;
  void query_player_turn_busy_units_owned(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
   std::shared_ptr<const BnwSelectedStockLuaSourceOwner>,std::weak_ptr<const void>,std::uint64_t,std::int32_t,
   std::shared_ptr<const BnwNativePlayerBusyUnitsReturn>&)const;

 // Earned opaque SAME-VM job. Numeric payload is Bootstrap-only, never a
 // public DTO/source grant. Constructor and creation are exclusively Core.
 class InitialUnitSetXYDispatch final {
  friend class BnwNativeUnitAllocatorCore;friend class BnwSelectedStockLuaBootstrap;friend class BnwSelectedStockLuaSourceOwner;
  InitialUnitSetXYDispatch(const InitialUnitSetXYDispatch&)=delete;
  InitialUnitSetXYDispatch&operator=(const InitialUnitSetXYDispatch&)=delete;
  struct Storage;std::shared_ptr<Storage> storage_;
  explicit InitialUnitSetXYDispatch(std::shared_ptr<Storage>);
  std::int32_t owner()const;std::int32_t full_id()const;
  std::int32_t x()const;std::int32_t y()const;
 };
 std::shared_ptr<const InitialUnitSetXYDispatch> begin_initial_unit_setxy_dispatch(
  const ui::SelectedCityTrainCreateSource&,const BnwStockLuaAttemptLease&)const;
 void finish_initial_unit_setxy_dispatch(const ui::SelectedCityTrainCreateSource&,const InitialUnitSetXYDispatch&)const;
 void fail_initial_unit_setxy_dispatch(const ui::SelectedCityTrainCreateSource&,const InitialUnitSetXYDispatch&)const noexcept;
 void require_initial_unit_setxy_dispatch_current(const InitialUnitSetXYDispatch&)const;
 class InitialHappinessFounderDispatch final {
  friend class BnwNativeUnitAllocatorCore;friend class BnwSelectedStockLuaBootstrap;friend class BnwSelectedStockLuaSourceOwner;
  InitialHappinessFounderDispatch(const InitialHappinessFounderDispatch&)=delete;
  InitialHappinessFounderDispatch&operator=(const InitialHappinessFounderDispatch&)=delete;
  struct Storage;std::shared_ptr<Storage> storage_;
  explicit InitialHappinessFounderDispatch(std::shared_ptr<Storage>);
  std::int32_t owner()const;
 };
 std::shared_ptr<const InitialHappinessFounderDispatch> begin_initial_happiness_founder_dispatch(
  const ui::SelectedCityTrainCreateSource&,const BnwStockLuaAttemptLease&)const;
 void finish_initial_happiness_founder_dispatch(const ui::SelectedCityTrainCreateSource&,const InitialHappinessFounderDispatch&,
  std::optional<std::int64_t> actual_accumulator)const;
 void fail_initial_happiness_founder_dispatch(const ui::SelectedCityTrainCreateSource&,const InitialHappinessFounderDispatch&)const noexcept;
 void require_initial_happiness_founder_dispatch_current(const InitialHappinessFounderDispatch&)const;

 void require_initial_happiness_prefix(const ui::SelectedCityTrainCreateSource&)const;
 void require_initial_happiness_suffix_prefix(const ui::SelectedCityTrainCreateSource&)const;
 void require_initial_happiness_return(const ui::SelectedCityTrainCreateSource&)const;
 std::uint64_t initial_happiness_prior_revision_without_callbacks()const;

 friend class BnwUnitMovePending;
 friend class BnwInitialUnitSightOwner;
 friend class BnwMinorCityStrengthScratch;
 friend std::int32_t detail::bnw_happiness_buildings_reached_math_for_initial_unit(const LiveSessionState&,const BnwBuildingHappinessRules&,const SessionCheckpointIdentity&,std::int64_t,detail::BnwBuildingHappinessReachedStage,const BnwNativeUnitAllocatorCore&);
 friend class graphics::BnwPendingUnitVisualOwner;
 friend class graphics::BnwSelectedPendingUnitVisualLoan;
 // Private actual Scene opening, issued only after SAME-node constructor storage
 // and before the real native SetID. A Loan cannot manufacture these operands.
 struct InitialVisibilityOpeningBinding {
  const BnwNativeUnitAllocatorCore* core=nullptr;
  std::weak_ptr<const BnwNativeUnitAllocatorCore> core_control;
  std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
  LiveSession* session=nullptr;std::shared_ptr<LiveSessionState> state;
  const saves::SavedSession* source=nullptr;std::string fingerprint;
  const void* bank=nullptr;std::weak_ptr<const void> bank_control;
  const void* node=nullptr;std::weak_ptr<const void> node_control;
  std::weak_ptr<const ui::SelectedCityTrainCreateSource> train_control;
  std::int32_t owner=-1,slot=-1,serial_before=0,city=-1,type=-1,unit_ai=-1,x=-1,y=-1;
  std::uint64_t generation=0,before_revision=0;
 };
 InitialVisibilityOpeningBinding capture_initial_visibility_opening_before_assignment()const;
 void require_initial_visibility_opening_identity_without_callbacks(const InitialVisibilityOpeningBinding&)const;
 void require_initial_visibility_opening_current_without_callbacks(const InitialVisibilityOpeningBinding&)const;
 void require_initial_visibility_hidden_slot_descends_from_opening_without_callbacks(
  const InitialVisibilityOpeningBinding&,const graphics::BnwPendingUnitHiddenSlotProof&)const;
 struct InitialVisibilityBinding {
  std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
  LiveSession* session=nullptr;std::shared_ptr<LiveSessionState> state;
  std::shared_ptr<const BnwUnitMovePending> pending;
  const void* node=nullptr;std::weak_ptr<const void> node_control;
  std::weak_ptr<const BnwNativeUnitAllocatorCore> core_control;
  std::int32_t owner=-1,id=-1,x=-1,y=-1,city=-1,base_combat=0;
  std::uint64_t generation=0,revision=0;
 };
 InitialVisibilityBinding capture_initial_visibility_binding(const BnwUnitMovePending&)const;
 // The original hidden binding descends from the pre-ID opening. No fresh
 // operand DTO, x/y-only position or caller-completed receipt is accepted.
 InitialVisibilityBinding capture_initial_set_position_binding(const BnwUnitMovePending&,const graphics::BnwPendingUnitHiddenSlotProof&)const;
 void require_initial_set_position_binding_current(const InitialVisibilityBinding&,const graphics::BnwPendingUnitHiddenSlotProof&)const;
 void require_initial_set_position_binding_identity_without_callbacks(const InitialVisibilityBinding&,const graphics::BnwPendingUnitHiddenSlotProof&)const;
 void require_initial_set_position_without_callbacks()const;
 // Genuine SDK numeric queries for the graphics-only original SetPosition payload.
 std::int32_t initial_set_position_domain(const InitialVisibilityBinding&,const graphics::BnwPendingUnitHiddenSlotProof&)const;
 bool initial_set_position_is_hovering(const InitialVisibilityBinding&,const graphics::BnwPendingUnitHiddenSlotProof&)const;
 bool initial_set_position_can_move_impassable(const InitialVisibilityBinding&,const graphics::BnwPendingUnitHiddenSlotProof&)const;
 bool initial_happiness_city_has_garrison(std::int32_t city)const;
 void require_initial_unit_info_promotion_loop_without_callbacks()const;
 bool initial_unit_info_promotion_loop_node_matches_without_callbacks()const;
 void require_initial_traits_promotions_without_callbacks()const;
 bool initial_traits_promotion_node_matches_without_callbacks()const;
 void require_initial_player_promotions_without_callbacks()const;
 bool initial_player_promotion_node_matches_without_callbacks()const;
 void require_initial_embark_promotions_without_callbacks()const;
 bool initial_embark_promotion_node_matches_without_callbacks()const;
 void require_initial_exotic_xp_without_callbacks()const;
 bool initial_exotic_xp_node_matches_without_callbacks()const;
 void require_initial_immobile_moves_without_callbacks()const;
 bool initial_unit_info_promotion_node_matches_without_callbacks()const;
 void require_initial_unit_info_promotion_without_callbacks()const;
 bool initial_name_node_matches_without_callbacks()const;
 void require_initial_military_support_prefix(const ui::SelectedCityTrainCreateSource&)const;
 void require_initial_military_support_without_callbacks()const;
 std::uint64_t initial_military_prior_revision_without_callbacks()const;
 std::uint64_t initial_center_earned_input_revision_without_callbacks(const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
 void require_initial_resource_nuke_zero_prefix_without_callbacks()const;
 void require_initial_support_zero_prefix_without_callbacks()const;
 void require_initial_player_class_counter_prefix(const ui::SelectedCityTrainCreateSource&)const;
 void require_initial_player_class_counter_without_callbacks()const;
 std::uint64_t initial_player_class_prior_revision_without_callbacks()const;
 const std::vector<std::uint64_t>& initial_current_happiness_finish_inputs_without_callbacks()const;
 void require_initial_team_class_counter_prefix(const ui::SelectedCityTrainCreateSource&)const;
 void require_initial_team_class_counter_without_callbacks()const;
 std::uint64_t initial_team_class_prior_revision_without_callbacks()const;
 void require_initial_game_class_counter_prefix(const ui::SelectedCityTrainCreateSource&)const;
 void require_initial_game_class_counter_without_callbacks()const;
 std::uint64_t initial_game_class_prior_revision_without_callbacks()const;
 void require_initial_name_game_counter_prefix(const ui::SelectedCityTrainCreateSource&)const;
 void require_initial_name_game_counter_without_callbacks()const;
 std::uint64_t initial_setxy_earned_world_revision_without_callbacks()const;
 void require_initial_setxy_tail_without_callbacks()const;
 std::optional<std::int32_t> initial_actual_garrison_city()const;
 void require_initial_unit_info_predecessors_without_callbacks()const;
 void require_initial_visibility_binding_current(const InitialVisibilityBinding&)const;
 void require_initial_visibility_binding_controls_without_callbacks(const InitialVisibilityBinding&)const;
 void require_initial_visibility_hidden_slot_current(const InitialVisibilityBinding&,const graphics::BnwPendingUnitHiddenSlotProof&)const;
 void require_initial_visibility_hidden_slot_identity_without_callbacks(const InitialVisibilityBinding&,const graphics::BnwPendingUnitHiddenSlotProof&)const;
 void require_initial_visibility_binding_identity_without_callbacks(const InitialVisibilityBinding&)const;
 std::int32_t require_completed_train_unit_id(const ui::SelectedCityTrainCreateSource&)const;
 std::int32_t require_completed_train_create_unit_return(const ui::SelectedCityTrainCreateSource&)const;
 struct InitialPlacementOperands {
  std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
  LiveSession* session=nullptr;std::shared_ptr<LiveSessionState> state;
  std::int32_t owner=-1,id=-1,x=-1,y=-1,city=-1,base_combat=0;
  std::uint64_t generation=0,revision=0;
 };
 InitialPlacementOperands initial_placement_operands()const;
 InitialPlacementOperands initial_setup_graphical_native_operands()const;
 InitialPlacementOperands initial_second_city_strength_operands()const;
 void require_initial_placement_current(const BnwUnitMovePending*)const;
 void execute_initial_coordinate_store(const BnwUnitMovePending&)const;
 struct InitialFltaOwnerSeal {std::int32_t actor;std::vector<std::int64_t> baseline;};
 std::vector<InitialFltaOwnerSeal> prepare_initial_destination_flta_lookup(const BnwUnitMovePending&)const;
 void require_initial_flta_current(const BnwUnitMovePending&,const std::vector<InitialFltaOwnerSeal>&)const;
 const saves::SavedUnitObservation* initial_native_unit_at(const BnwUnitMovePending&,std::int32_t,std::int32_t)const;
 std::shared_ptr<const BnwNativeUnitAllocatorCore> retain_factory_owner()const;
 void require_train_id_prefix(const ui::SelectedCityTrainCreateSource&)const;
 void require_initial_garrison_prefix(const ui::SelectedCityTrainCreateSource&)const;
 void require_initial_sight_prefix(const ui::SelectedCityTrainCreateSource&)const;
 void require_initial_plot_area_prefix(const ui::SelectedCityTrainCreateSource&)const;
 void require_initial_city_strength_prefix(const ui::SelectedCityTrainCreateSource&)const;
 // Private native resolver: actual bank/node/control and issued plot-area phase.
 // No caller may supply a unit facade or manufacture a SavedUnit surrogate.
 void require_initial_city_strength_node_without_callbacks()const;
 void require_initial_post_city_checks_without_callbacks()const;
 const std::map<std::string,std::int32_t,std::less<>>& initial_city_strength_scalars()const;
 const std::map<std::string,std::vector<std::int32_t>,std::less<>>& initial_city_strength_indexed()const;
 struct InitialSightOperands {
  InitialVisibilityBinding visual;
  std::int32_t type=-1,team=-1,extra_visibility=0,see_invisible=-1;
 };
 InitialSightOperands initial_sight_operands()const;
 void require_initial_sight_operands_without_callbacks(const InitialSightOperands&)const;
 std::weak_ptr<const BnwNativeUnitAllocatorCore> factory_self_;
 friend class ui::SelectedBnwPreviewHost;
 static std::shared_ptr<const BnwNativeUnitAllocatorCore>from_actual_host(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int32_t);
 static std::shared_ptr<const BnwNativeUnitAllocatorCore>from_unpublished_mission_restore_host(std::shared_ptr<const ui::SelectedBnwPreviewHost>,const saves::CivisSaveSection&);
 BnwPathfinderCacheControlOwner make_original_payload_mission_pathfinder_controls()const;
 BnwUnitPathOwner make_original_payload_mission_unit_paths()const;
 void require_restored_mission_owner_current_without_callbacks(const saves::CivisSaveSection&)const;
 static void require_restored_mission_weak_owner_continuity_current_without_callbacks(const std::weak_ptr<const BnwNativeUnitAllocatorCore>&,const saves::CivisSaveSection&);
 void require_restored_mission_checkpoint_current_without_callbacks(const saves::CivisSaveSection&)const;
 static void require_restored_mission_weak_owner_current_without_callbacks(const std::weak_ptr<const BnwNativeUnitAllocatorCore>&,const saves::CivisSaveSection&);
 friend class BnwUnitCombatWarnedRestorePlan;
 void initialize_unit_combat_warned_restore_plan(BnwUnitCombatWarnedRestorePlan&,const saves::CivisSaveSection&)const;
 void require_unit_combat_warned_restore_plan_without_callbacks(const BnwUnitCombatWarnedRestorePlan&)const;
 BnwNativeContinueMissionPhaseObservation restore_native_existing_continue_mission_completed(const saves::CivisSaveSection&)const;
 struct ContinueMissionCombatWarnedAttempt;
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwNativeUnitAllocatorCore(std::shared_ptr<Impl>);
};
}
