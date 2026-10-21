#pragma once
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include <memory>
#include <optional>
#include <vector>
#include <stdexcept>
namespace civ5::ui {class SelectedBnwPreviewHost;class BnwClientSelectionControl;enum class BnwClientUnitDirtyKind:std::int32_t;}
namespace civ5::runtime {
class BnwNativeUnitAllocatorCore;class BnwUnitMoveRegistry;class BnwSelectedStockLuaSourceOwner;
class BnwSelectedStockLuaBootstrap;class BnwStockLuaAttemptLease;
class BnwUnitMoveUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
enum class BnwUnitMoveBoundary {
 dim_receiver, visible_to_watching_human, set_xy_popup_owner, selected_ui_tail,
 common_pathfinder_reset_owner, selection_plot_owner, unit_info_dirty_owner, plot_list_dirty_owner,
 initial_destination_collision_owner, initial_city_visibility_owner, initial_city_garrison_policy_owner, initial_new_sight_owner, initial_plot_add_unit_owner, initial_city_strength_owner, initial_post_city_setxy_owner, initial_set_position_owner, initial_post_position_owner, initial_enemy_city_range_owner, initial_happiness_prefix_owner, initial_happiness_founder_owner, initial_happiness_excess_owner, initial_unit_setxy_owner, initial_popup_release_owner, initial_garrison_dirty_owner, initial_center_unit_owner, initial_unit_name_owner, initial_unit_class_count_owner, initial_team_unit_class_count_owner, initial_player_unit_class_count_owner, initial_num_builders_owner, initial_resource_used_owner, initial_military_support_owner, initial_free_unit_promotions_owner, initial_promotion_native_remainder_owner, initial_traits_promotions_owner, initial_player_free_promotions_owner, initial_embark_promotions_owner, initial_exotic_goods_owner, initial_immobile_moves_owner, initial_moves_dim_owner, initial_moves_selection_owner, initial_post_moves_owner
};
// Actual native wrapper identity is represented by the current full Unit ID.
// This is a queued new-client intent, NOT completed original engine fanout.
struct BnwUnitMoveDimIntent {
 BnwUnitOrderKey unit;
 std::uint64_t occurrence=0,generation=0,world_revision=0;
 bool dim=false;
};
struct BnwUnitMovePrefix {
 std::int32_t cost=0,before_moves=0,after_moves=0;
 bool moves_stored=false;
 // Actual private receiver return only; never a supplied acknowledgement.
 bool dim_attempted=false,dim_returned=false;
 // Diagnostic facts from the private canonical input getter, never an ack.
 bool selection_attempted=false;
 std::optional<bool> unit_selected;
 bool common_cache_reset=false,selection_plot_read=false,set_moves_completed=false;
 bool unit_info_dirty_stored=false,plot_list_dirty_stored=false;
 std::uint64_t unit_info_dirty_occurrence=0,plot_list_dirty_occurrence=0;
 std::optional<std::int32_t> before_plot,selected_plot;
 // Initial Unit::Init calls setXY directly, without move cost/setMoves/Dim.
 bool initial_unit=false,popup_hold_stored=false,initial_coordinates_stored=false,initial_visibility_returned=false;
 bool initial_happiness_prefix_stored=false;
 bool initial_happiness_native_suffix_stored=false;
 bool initial_happiness_excess_stored=false,initial_happiness_returned=false,initial_unit_setxy_returned=false;
 std::uint64_t initial_game_data_occurrence=0;
 bool initial_popup_released=false,initial_garrison_dirty_returned=false;
 bool initial_immobile_moves_computed=false;std::int32_t initial_moves_before=0,initial_moves_requested=0;
 bool initial_moves_value_changed=false,initial_moves_stored=false,initial_should_dim_enqueue_returned=false,initial_moves_setter_returned=false;
 bool initial_moves_selection_checked=false,initial_moves_unit_selected=false,initial_moves_common_reset=false,initial_moves_selection_plot_checked=false,initial_moves_tail_returned=false;
 std::optional<std::uint64_t>initial_moves_selected_plot;
 std::uint64_t initial_moves_unit_info_occurrence=0,initial_moves_plot_list_occurrence=0;
 bool initial_religion_returned=false,initial_religion_applied=false;
 std::int32_t initial_religion_majority=-1,initial_religion_value=-1,initial_religion_strength=0,initial_religion_spreads=0;
 bool initial_tourism_returned=false,initial_recon_returned=false;
 std::int32_t initial_tourism_one_shot=0,initial_tourism_creation=0,initial_recon_count=0;
 bool initial_best_land_returned=false,initial_best_land_game_read=false,initial_best_land_setter_called=false,initial_best_land_changed=false,initial_best_land_stored=false;
 std::int32_t initial_best_land_type=-1;std::uint64_t initial_best_land_unit_info_occurrence=0;
 bool initial_active_game_data_returned=false,initial_active_game_data_applied=false;
 std::uint64_t initial_active_game_data_occurrence=0;
 bool initial_unit_cycler_returned=false,initial_unit_cycler_applied=false;
 std::uint64_t initial_unit_cycler_units=0,initial_unit_cycler_plain_stores=0;
 bool initial_world_unit_message_returned=false;
 std::int32_t initial_world_unit_class=-1,initial_world_unit_max_global=-1;
 bool initial_second_city_strength_returned=false,initial_second_city_strength_applied=false;
 std::int32_t initial_second_city_strength_before=0,initial_second_city_strength_after=0;
 std::uint64_t initial_second_city_strength_occurrence=0;
 bool initial_setup_graphical_returned=false;
 bool initial_minor_quest_returned=false,initial_minor_quest_tested=false;std::uint32_t initial_minor_quest_minors=0,initial_minor_quest_entries=0,initial_minor_quest_gp_checks=0;
 bool initial_maintenance_returned=false,initial_maintenance_active=false;std::int32_t initial_maintenance_value=0;
 bool initial_army_ai_returned=false;
 std::int32_t initial_army_before=0,initial_army_after=-1,initial_unit_ai_before=-1,initial_unit_ai_after=-1;
 bool initial_exotic_xp_returned=false;
 bool initial_embark_promotions_returned=false;std::size_t initial_embark_promotion_grants=0;
 bool initial_player_promotions_returned=false;std::size_t initial_player_promotion_grants=0;
 bool initial_traits_promotions_returned=false;std::size_t initial_traits_promotion_grants=0;
 bool initial_unit_info_promotions_returned=false;std::size_t initial_unit_info_promotion_grants=0;
 bool initial_unit_info_promotion_prefix_stored=false;std::int32_t initial_unit_info_promotion_id=-1;std::size_t initial_unit_info_promotion_slots=0;
 bool initial_center_unit_returned=false,initial_name_game_counter_returned=false,initial_game_class_counter_returned=false,initial_team_class_counter_returned=false,initial_player_class_counter_returned=false,initial_support_zero_returned=false,initial_resource_nuke_zero_returned=false,initial_military_support_returned=false,initial_military_support_incremented=false;
 std::int32_t initial_military_count_before=0,initial_military_count_after=0;
 std::uint64_t initial_military_game_data_occurrence=0;
 std::size_t initial_resource_slots=0;
 std::int32_t initial_player_class_before=0,initial_player_class_after=0;
 std::int32_t initial_unit_class_team=-1,initial_team_class_before=0,initial_team_class_after=0;
 std::int32_t initial_unit_class=-1,initial_game_class_before=0,initial_game_class_after=0;
 std::int32_t initial_game_turn_created=0,initial_unit_count_before=0,initial_unit_count_after=0;
 std::uint64_t initial_center_setters=0,initial_center_visibility_receivers=0;
 std::uint64_t popup_release_setter_occurrence=0,popup_release_client_revision=0,initial_garrison_dirty_occurrence=0;
 std::optional<std::int32_t> initial_new_garrison_city;
 bool initial_enemy_city_range_returned=false;
 std::uint64_t initial_enemy_city_plots_checked=0;
 bool initial_info_bar_dirty_stored=false;
 std::uint64_t initial_info_bar_dirty_occurrence=0;
 bool initial_garrison_policy_returned=false,initial_fortify_returned=false,initial_sight_returned=false,initial_plot_area_returned=false,initial_city_strength_returned=false,initial_post_city_checks_returned=false,initial_set_position_returned=false;
 std::uint64_t initial_no_effect_meet_returns=0,initial_adjacent_plots_checked=0;
 std::int32_t initial_garrison_culture_modifier=0;
 std::uint64_t initial_collision_entries=0;
 std::uint64_t popup_setter_occurrence=0,popup_client_revision=0;
 BnwUnitMoveBoundary boundary=BnwUnitMoveBoundary::dim_receiver;
};
class BnwUnitMovePending final {
 public:
 ~BnwUnitMovePending();
 BnwUnitMovePending(const BnwUnitMovePending&)=delete;
 BnwUnitMovePending& operator=(const BnwUnitMovePending&)=delete;
 void require_current()const;
 const BnwUnitMovePrefix& prefix()const;
 // Owned diagnostic copy; it does not borrow Session registry storage.
 std::vector<BnwUnitMoveDimIntent> queued_new_client_intents()const;
 private:
 friend class LiveSession;friend class BnwUnitMoveRegistry;friend class BnwNativeUnitAllocatorCore;
 friend class BnwSelectedStockLuaSourceOwner;friend class BnwSelectedStockLuaBootstrap;
 friend class ui::BnwClientSelectionControl;
 struct StockDimOperands {
  std::int32_t owner,full_id,moves;
  std::uint64_t occurrence,generation,world_revision;
 };
 void require_same_registered_pending(const ui::SelectedBnwPreviewHost*)const;
 void begin_stock_dim(const ui::SelectedBnwPreviewHost*,const BnwStockLuaAttemptLease&)const;
 StockDimOperands stock_dim_operands()const;
 void accept_stock_dim_return(const ui::SelectedBnwPreviewHost*)const;
 void continue_post_dim_selection(const ui::SelectedBnwPreviewHost*)const;
 struct DirtyOperands {
  std::int32_t owner,full_id,moves;
  std::uint64_t occurrence,generation,world_revision;
  ui::BnwClientUnitDirtyKind kind;
  std::int32_t before_plot;
  std::uint64_t input_revision;
 };
 DirtyOperands require_dirty_store(const ui::BnwClientSelectionControl&)const;
 void record_dirty_store(ui::BnwClientUnitDirtyKind,std::uint64_t)const noexcept;
 void fail_stock_dim()const noexcept;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwUnitMovePending(std::unique_ptr<Impl>)noexcept;
 static std::shared_ptr<BnwUnitMovePending> create(LiveSession&,
  const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,BnwUnitOrderKey,
  std::int32_t x,std::int32_t y,bool show);
 static std::shared_ptr<BnwUnitMovePending> create_for_initial_unit(
  const std::shared_ptr<const BnwNativeUnitAllocatorCore>&);
 void execute_initial_set_xy_prefix();
 void execute_initial_destination_collision_prefix();
 void require_initial_coordinate_store(const BnwNativeUnitAllocatorCore&)const;
 void require_initial_visibility_phase_without_callbacks()const;
 void require_initial_visibility_source_without_callbacks()const;
 const BnwUnitMovePrefix& initial_setxy_tail_prefix_without_callbacks()const;
 void require_initial_visibility_hidden_slot_phase_without_callbacks()const;
 void record_initial_visibility_return()const noexcept;
 void record_initial_garrison_fortify_return(std::int32_t,std::uint64_t,std::uint64_t)const noexcept;
 void record_initial_sight_return(std::uint64_t)const noexcept;
 void record_initial_plot_area_return(std::uint64_t)const noexcept;
 void record_initial_city_strength_return(std::uint64_t)const noexcept;
 void require_initial_city_strength_phase_without_callbacks()const;
 void record_initial_post_city_checks_return(std::uint64_t,std::uint64_t)const noexcept;
 void require_initial_set_position_phase_without_callbacks()const;
 void require_initial_position_returned_without_callbacks()const;
 struct InitialUnitInfoDirtyOperands {
  std::int32_t owner,full_id;std::uint64_t occurrence,generation,world_revision,input_revision;
 };
 InitialUnitInfoDirtyOperands require_initial_unit_info_dirty_store(const ui::BnwClientSelectionControl&)const;
 void record_initial_unit_info_dirty_store(std::uint64_t)const noexcept;
 void record_initial_enemy_city_range_return(std::uint64_t)const noexcept;
 void require_initial_happiness_prefix_without_callbacks()const;
 void record_initial_happiness_prefix_return(std::uint64_t)const noexcept;
 void record_initial_happiness_native_suffix_return(std::uint64_t)const noexcept;
 std::uint64_t preflight_initial_happiness_game_data()const;
 std::uint64_t execute_initial_happiness_game_data_dirty(std::uint64_t expected_before)const;
 void record_initial_happiness_excess_store(std::uint64_t)const noexcept;
 void record_initial_happiness_return(std::uint64_t)const noexcept;
 void record_initial_unit_setxy_return()const noexcept;
 void require_initial_popup_release_store(const ui::BnwClientSelectionControl&)const;
 void record_initial_popup_release_store(std::uint64_t,std::uint64_t)const noexcept;
 void execute_initial_popup_release_and_garrison();
 void record_initial_center_unit_return(std::uint64_t,std::uint64_t)const noexcept;
 std::shared_ptr<ui::BnwClientSelectionControl> retain_initial_military_game_data_input()const;
 void require_initial_military_game_data_identity_without_callbacks(const std::shared_ptr<ui::BnwClientSelectionControl>&)const;
 void record_initial_military_count_store(bool,std::int32_t,std::int32_t,std::uint64_t)const noexcept;
 void record_initial_immobile_moves_computed(std::int32_t,std::int32_t)const noexcept;
 void record_initial_moves_store(bool)const noexcept;
 void record_initial_should_dim_return(bool)const noexcept;
 void record_initial_moves_selection(bool)const noexcept;
 void record_initial_moves_common_reset()const noexcept;
 void record_initial_moves_selection_plot(std::optional<std::uint64_t>)const noexcept;
 void record_initial_moves_ui_dirty(bool,std::uint64_t)const noexcept;
 void record_initial_moves_tail_return()const noexcept;
 void record_initial_religion_return(bool,std::int32_t,std::int32_t,std::int32_t,std::int32_t)const noexcept;
 void record_initial_tourism_return(std::int32_t,std::int32_t)const noexcept;
 void record_initial_recon_return(std::int32_t)const noexcept;
 void record_initial_best_land_return(bool,bool,bool,std::int32_t)const noexcept;
 void record_initial_best_land_game_store(std::int32_t,std::uint64_t)const noexcept;
 void record_initial_best_land_dirty_store(std::uint64_t)const noexcept;
 void record_initial_active_game_data_store(std::uint64_t)const noexcept;
 void record_initial_active_game_data_return(bool)const noexcept;
 void record_initial_unit_cycler_return(bool,std::size_t,std::size_t)const noexcept;
 void record_initial_world_unit_message_return(std::int32_t,std::int32_t)const noexcept;
 void record_initial_second_city_strength_return(bool,std::int32_t,std::int32_t,std::uint64_t,std::uint64_t)const noexcept;
 void record_initial_army_ai_return(std::int32_t,std::int32_t,std::int32_t)const noexcept;
 void record_initial_maintenance_return(bool,std::int32_t)const noexcept;
 void record_initial_minor_quest_return(bool,std::uint32_t,std::uint32_t,std::uint32_t)const noexcept;
 void record_initial_setup_graphical_return()const noexcept;
 void record_initial_exotic_xp_return()const noexcept;
 void record_initial_embark_promotions_return(std::size_t)const noexcept;
 void record_initial_player_promotions_return(std::size_t)const noexcept;
 void record_initial_traits_promotions_return(std::size_t)const noexcept;
 void record_initial_unit_info_promotions_return(std::size_t)const noexcept;
 void record_initial_unit_info_promotion_prefix(std::int32_t,std::size_t)const noexcept;
 void record_initial_military_support_return(std::uint64_t)const noexcept;
 void record_initial_resource_nuke_zero_return(std::size_t)const noexcept;
 void record_initial_support_zero_return()const noexcept;
 void record_initial_player_class_counter_return(std::int32_t,std::int32_t,std::uint64_t)const noexcept;
 void record_initial_team_class_counter_return(std::int32_t,std::int32_t,std::int32_t,std::uint64_t)const noexcept;
 void record_initial_game_class_counter_return(std::int32_t,std::int32_t,std::int32_t,std::uint64_t)const noexcept;
 void record_initial_name_game_counter_return(std::int32_t,std::int32_t,std::int32_t,std::uint64_t)const noexcept;
 struct InitialGarrisonDirtyOperands {std::int32_t owner,city;std::uint64_t generation,world_revision,input_revision;};
 InitialGarrisonDirtyOperands require_initial_garrison_dirty_store(const ui::BnwClientSelectionControl&)const;
 void record_initial_garrison_dirty_return(std::optional<std::int32_t>,std::uint64_t)const noexcept;
 void execute_initial_unit_info_dirty_prefix();
 void record_initial_set_position_return()const noexcept;
 void require_initial_visibility_core_identity_without_callbacks(const BnwNativeUnitAllocatorCore&)const;
 void record_initial_coordinate_store()const noexcept;
 void require_initial_popup_store(const ui::BnwClientSelectionControl&)const;
 void record_initial_popup_store(std::uint64_t,std::uint64_t)const noexcept;
 void execute_prefix();
 void retire()noexcept;
};
// Session-owned journal survives handle drop; it never owns Host. Restoring
// checkpoints retires this registry and retains its touched-ID quarantine.
class BnwUnitMoveRegistry final {
 public:
 ~BnwUnitMoveRegistry();
 void retire()noexcept;
 bool has_history()const noexcept;
 private:
 friend class LiveSession;friend class BnwUnitMovePending;
 struct Impl;std::unique_ptr<Impl> impl_;
 BnwUnitMoveRegistry();
};
namespace detail {
// Arithmetic helper only. A supplied cost is never a movement capability.
std::int32_t bnw_unit_move_after_cost(std::int32_t moves,std::int32_t cost);
}
}
