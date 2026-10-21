#pragma once
#include "civ5/runtime/bnw_city_dirty_event.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>
namespace civ5::runtime {class BnwCityFoodGrowthPending;class LiveSession;class BnwCityEntryPending;class BnwUnitMovePending;class BnwRetainedTurnCursor;class BnwSelectedStockLuaSourceOwner;}
namespace civ5::runtime {class BnwMajorTechClientEndpoint;class BnwNativeUnitAllocatorCore;class BnwPolicyAdoptionPending;class BnwLivePlayerGoldPending;enum class TreasuryDirty;class BnwStockLuaAttemptLease;struct LiveSessionState;}
namespace civ5::runtime {class BnwAttritionCityInfoDirtyLoan;class BnwAttritionUnitInfoDirtyLoan;class BnwAttritionUnitSelectedRead;class BnwAttritionSelectedUnitInfoDirtyLoan;class BnwAttritionSelectionPlotRead;class BnwAttritionPlotListDirtyLoan;class BnwNativeAttritionPrefix;}
namespace civ5::ui {
class SelectedBnwPreviewHost;class BnwClientFrameOwner;class LiveUiHost;class BnwTechAwardInputDispatch;
// SDK InterfaceDirtyBits values. Diagnostics are not completion capabilities.
enum class BnwClientUnitDirtyKind:std::int32_t {plot_list_buttons=5,unit_info=19};
enum class BnwClientScoreDirtyKind:std::int32_t {score=12};
enum class BnwClientGameDataDirtyKind:std::int32_t {game_data=11};
// Actual SDK TurnTimer_DIRTY_BIT=13, independent from GameData11.
struct BnwClientGameTurnDirtyPair {
 std::uint64_t before=0,after=0,timer_before=0,game_before=0;
 bool operator==(const BnwClientGameTurnDirtyPair&)const=default;
};
// Actual SDK InterfaceDirtyBits, not a GameData substitute.
enum class BnwClientLeagueDirtyKind:std::int32_t {league_screen=35};
struct BnwClientUnitDirtyEvent {
 std::uint64_t occurrence=0,move_occurrence=0,generation=0,world_revision=0,client_revision=0;
 std::int32_t owner=-1,full_id=-1;
 BnwClientUnitDirtyKind kind=BnwClientUnitDirtyKind::unit_info;
 bool previous_value=false;
 bool operator==(const BnwClientUnitDirtyEvent&)const=default;
};
struct BnwClientUnitDirtySnapshot {
 bool unit_info=false,plot_list_buttons=false;
 std::uint64_t next_occurrence=1,client_revision=0;
 std::vector<BnwClientUnitDirtyEvent> events;
 bool operator==(const BnwClientUnitDirtySnapshot&)const=default;
};
struct BnwSelectionPlotUnavailable : std::runtime_error {using std::runtime_error::runtime_error;};
// No public constructor, source token, callback or supplied selection bool.
// Actual Frame/LiveUiHost input producers share this canonical private value.
class BnwClientSelectionControl final {
 public:
 ~BnwClientSelectionControl();
 // Actual Civis DLLUI flag. Unknown until a genuine SDK setter runs;
 // it is not reconstructed from game saves or a supplied readiness bit.
 std::optional<bool> popups_suppressed()const;
 std::uint64_t popup_hold_setter_count()const;
 std::vector<runtime::BnwSpecificCityInfoEvent> initial_garrison_dirty_events()const;
 BnwClientSelectionControl(const BnwClientSelectionControl&)=delete;
 BnwClientSelectionControl& operator=(const BnwClientSelectionControl&)=delete;
 private:
 friend class BnwClientFrameOwner;friend class LiveUiHost;friend class BnwTechAwardInputDispatch;
 friend class runtime::BnwMajorTechClientEndpoint;
 friend class runtime::BnwNativeUnitAllocatorCore;
 friend class BnwEngineUiTurnOwner;friend class runtime::BnwPolicyAdoptionPending;
 void consume_policy_process_dirty(const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
 void require_policy_process_dirty_return(const runtime::BnwPolicyAdoptionPending&)const;
 void consume_outer_policy_dirty(const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
 void require_outer_policy_dirty_return(const runtime::BnwPolicyAdoptionPending&)const;
 void consume_golden_age_dirty(const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
 void require_golden_age_dirty_return(const runtime::BnwPolicyAdoptionPending&)const;
 void consume_happiness_dirty(const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
 void require_happiness_dirty_return(const runtime::BnwPolicyAdoptionPending&)const;
 void consume_resource_policy_layout(const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
 void require_resource_policy_layout_return(const runtime::BnwPolicyAdoptionPending&)const;
 bool resource_policy_plot_layout_matches_without_callbacks(const runtime::LiveSessionState&,std::uint64_t,std::int64_t)const;
 void consume_free_tech_dirty(const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
 void require_free_tech_dirty_return(const runtime::BnwPolicyAdoptionPending&)const;
 friend class runtime::BnwLivePlayerGoldPending;
 void consume_live_gold_dirty(const std::shared_ptr<const runtime::BnwLivePlayerGoldPending>&,runtime::LiveSessionState&,runtime::TreasuryDirty,const runtime::BnwStockLuaAttemptLease&);
 std::uint64_t require_live_gold_dirty_return(const runtime::BnwLivePlayerGoldPending&,runtime::TreasuryDirty)const;
 void store_free_tech_research_buttons_dirty_without_callbacks();
 void store_outer_policies_dirty_without_callbacks();
 friend class runtime::BnwAttritionCityInfoDirtyLoan;friend class runtime::BnwNativeAttritionPrefix;
 struct AttritionCityInfoState {
  std::uint64_t revision=0,city_count=0,generation=0;std::optional<bool> flag;
  const void*host=nullptr;const void*state=nullptr;const void*source=nullptr;
 };
 AttritionCityInfoState capture_attrition_city_info_without_callbacks()const;
 bool attrition_city_info_state_matches_without_callbacks(const AttritionCityInfoState&,bool)const;
 void consume_attrition_city_info_dirty(const runtime::BnwAttritionCityInfoDirtyLoan&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
 void require_attrition_city_info_dirty_return(const runtime::BnwAttritionCityInfoDirtyLoan&)const;
 friend class runtime::BnwAttritionUnitInfoDirtyLoan;
 struct AttritionUnitInfoState {
  BnwClientUnitDirtySnapshot unit;std::optional<bool>city_flag;std::uint64_t city_count=0,generation=0;
  const void*host=nullptr;const void*state=nullptr;const void*source=nullptr;
  std::optional<std::pair<std::int64_t,std::int64_t>>selected;std::optional<std::int64_t>selected_unit;
  std::optional<std::int32_t>selected_owner,selected_plot;
 };
 AttritionUnitInfoState capture_attrition_unit_info_without_callbacks()const;
 bool attrition_unit_info_state_matches_without_callbacks(const AttritionUnitInfoState&,const BnwClientUnitDirtyEvent&,bool)const;
 void consume_attrition_unit_info_dirty(const runtime::BnwAttritionUnitInfoDirtyLoan&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
 void require_attrition_unit_info_dirty_return(const runtime::BnwAttritionUnitInfoDirtyLoan&)const;
 friend class runtime::BnwAttritionUnitSelectedRead;
 void require_attrition_unit_selected_current(const runtime::BnwAttritionUnitSelectedRead&)const;
 bool consume_attrition_unit_selected_read(const runtime::BnwAttritionUnitSelectedRead&,const runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&)const;
 void require_attrition_unit_selected_read_return(const runtime::BnwAttritionUnitSelectedRead&)const;
 friend class runtime::BnwAttritionSelectedUnitInfoDirtyLoan;
 void require_attrition_selected_unit_info_current(const runtime::BnwAttritionSelectedUnitInfoDirtyLoan&)const;
 void consume_attrition_selected_unit_info_dirty(const runtime::BnwAttritionSelectedUnitInfoDirtyLoan&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
 void require_attrition_selected_unit_info_dirty_return(const runtime::BnwAttritionSelectedUnitInfoDirtyLoan&)const;
 friend class runtime::BnwAttritionSelectionPlotRead;
 void require_attrition_selection_plot_current(const runtime::BnwAttritionSelectionPlotRead&)const;
 std::optional<std::int32_t> consume_attrition_selection_plot_read(const runtime::BnwAttritionSelectionPlotRead&,const runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&)const;
 void require_attrition_selection_plot_read_return(const runtime::BnwAttritionSelectionPlotRead&)const;
 friend class runtime::BnwAttritionPlotListDirtyLoan;
 void require_attrition_plot_list_current(const runtime::BnwAttritionPlotListDirtyLoan&)const;
 bool attrition_plot_list_state_matches_without_callbacks(const AttritionUnitInfoState&,const BnwClientUnitDirtyEvent&,bool)const;
 void consume_attrition_plot_list_dirty(const runtime::BnwAttritionPlotListDirtyLoan&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
 void require_attrition_plot_list_dirty_return(const runtime::BnwAttritionPlotListDirtyLoan&)const;
 void store_policy_city_info_dirty_without_callbacks();
 void store_game_data_dirty_without_callbacks();
 std::optional<std::pair<std::int32_t,std::int64_t>> initial_center_selection()const;
 std::uint64_t initial_center_revision_without_callbacks()const;
 void preflight_initial_center_info_dirty_setters(std::size_t)const;
 std::uint64_t set_initial_center_info_dirty(const runtime::BnwNativeUnitAllocatorCore&,std::size_t);
 bool initial_moves_dirty_matches_without_callbacks(const BnwClientUnitDirtySnapshot&)const;
 void set_initial_moves_dirty(const runtime::BnwNativeUnitAllocatorCore&,BnwClientUnitDirtyKind);
 // Actual changed CvGame::setBestLandUnit UnitInfo19 producer.
 std::uint64_t set_initial_best_land_dirty(const runtime::BnwNativeUnitAllocatorCore&);
 void set_initial_unit_game_data_dirty(const runtime::BnwNativeUnitAllocatorCore&);
 void set_initial_unit_maintenance_dirty(const runtime::BnwNativeUnitAllocatorCore&);
 std::optional<bool> initial_unit_maintenance_city_info_dirty()const;
 std::uint64_t initial_unit_maintenance_city_info_setter_count()const;
 bool initial_unit_maintenance_matches_without_callbacks(std::optional<bool>,std::uint64_t,std::optional<bool>,std::uint64_t,std::uint64_t)const;
 bool initial_unit_game_data_matches_without_callbacks(std::optional<bool>,std::uint64_t,std::uint64_t)const;
 std::shared_ptr<const SelectedBnwPreviewHost> retained_host_for_tech_transport()const;
 friend class runtime::BnwSelectedStockLuaSourceOwner;
 friend class BnwPlotYieldRefreshUiAdapter;friend class BnwCityProductionUiAdapter;
 friend class runtime::LiveSession;friend class runtime::BnwCityEntryPending;friend class runtime::BnwCityFoodGrowthPending;
 friend class runtime::BnwUnitMovePending;
 friend class runtime::BnwRetainedTurnCursor;
 void require_retained_cycle_session(const runtime::LiveSession&)const;
 void reset_no_selection_list_cycle();
 bool no_selection_list_cycle()const;
 // SDK Score_DIRTY_BIT=12. Only the actual retained score producer may issue
 // this Civis-owned setter; an unknown imported UI flag is never inferred.
 void preflight_score_dirty_setters(std::size_t count)const;
 void set_score_dirty(BnwClientScoreDirtyKind);
 // SDK GameData_DIRTY_BIT=11; actual retained deal producer only.
 BnwClientGameTurnDirtyPair set_game_turn_dirty_pair();
 bool game_turn_dirty_pair_matches_without_callbacks(const BnwClientGameTurnDirtyPair&)const;
 std::optional<bool> turn_timer_dirty()const;
 std::uint64_t turn_timer_dirty_setter_count()const;
 void preflight_game_data_dirty_setter()const;
 void preflight_league_screen_dirty_setter()const;
 void set_league_screen_dirty(BnwClientLeagueDirtyKind);
 void set_game_data_dirty(BnwClientGameDataDirtyKind);
 std::optional<bool> game_data_dirty()const;
 std::uint64_t game_data_dirty_setter_count()const;
 std::optional<bool> score_dirty()const;
 std::uint64_t score_dirty_setter_count()const;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwClientSelectionControl(std::unique_ptr<Impl>)noexcept;
 static std::shared_ptr<BnwClientSelectionControl> create(std::shared_ptr<const SelectedBnwPreviewHost>);
 void bind_actual_ui_host();
 void require_current(bool need_ui=true)const;
 void require_thread()const;
 void retire()noexcept;
 std::uint64_t revision()const;
 std::optional<bool> league_screen_dirty()const;
 std::uint64_t league_screen_dirty_setter_count()const;
 std::uint64_t world_revision()const;
 std::int32_t active_player()const;
 std::int32_t active_team()const;
 std::int32_t city_team(std::int32_t owner,std::int32_t city)const;
 std::optional<std::pair<std::int64_t,std::int64_t>> city()const;
 void select(std::optional<std::pair<std::int64_t,std::int64_t>>);
 void select_input(std::optional<std::pair<std::int64_t,std::int64_t>>,std::optional<std::int64_t> unit);
 bool is_selected(std::int32_t owner,std::int32_t city)const;
 // Genuine setMoves consumer only. No supplied selection or completion token.
 bool is_unit_selected(std::int32_t owner,std::int32_t full_id)const;
 std::optional<std::int64_t> unit()const;
 // Actual nullable new-client picking input; no engine import membership.
 void select_plot(std::optional<std::int64_t> plot);
 std::optional<std::uint64_t> selection_plot()const;
 std::vector<runtime::BnwCityDirtyEvent> take_events();
 // Sole setter consumer is the genuine registered post-Dim native Pending.
 void begin_initial_set_xy_popup_hold(const runtime::BnwUnitMovePending&);
 void finish_initial_set_xy_popup_hold(const runtime::BnwUnitMovePending&);
 void set_initial_garrison_dirty(const runtime::BnwUnitMovePending&);
 std::vector<runtime::BnwSpecificCityInfoEvent> take_initial_garrison_dirty_events();
 std::uint64_t set_moves_dirty(const runtime::BnwUnitMovePending&);
 std::uint64_t set_initial_newborn_info_dirty(const runtime::BnwUnitMovePending&);
 BnwClientUnitDirtySnapshot unit_dirty_state()const;
 std::vector<BnwClientUnitDirtyEvent> take_unit_dirty_events();
};
}
