#pragma once
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include <cstdint>
#include <atomic>
#include <thread>
#include "civ5/runtime/bnw_happiness_receipt_types.hpp"
#include "civ5/runtime/bnw_city_dirty_event.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <array>
#include <map>
#include <vector>
#include <string>
namespace civ5::saves {struct SavedSession;struct PlayerAdministrationRecord;}
namespace civ5::ui {class SelectedBnwPreviewHost;class BnwEngineUiTurnOwner;class BnwClientSelectionControl;}
namespace civ5::runtime {
struct LiveSessionState;struct SessionCheckpointIdentity;class BnwPlotYieldRefreshIssuer;class BnwPlotYieldRefreshPending;class BnwNormalPlotYieldRules;
class LiveSession;class BnwSelectedStockLuaSourceOwner;class BnwPolicyAdoptionPending;
class BnwPolicyAdoptionUnavailable final:public std::runtime_error {public:using std::runtime_error::runtime_error;};
struct BnwPolicyAdoptionCommand {
 std::int64_t player=-1,policy=-1,expected_turn=-1;
 std::uint64_t expected_generation=0,expected_ui_source_revision=0;
};
enum class BnwPolicyAdoptionPhase {prepared,denied,branch_finisher_pending,branch_finisher_executing,finisher_process_returned,process_policies_pending,uncertain,worker_speed_pending,treasury_route_gold_pending,city_strength_pending,spaceship_hurry_pending,one_shot_pending,yield_arrays_pending,policy_cost_pending,plot_yield_pending,specialist_yield_pending,unit_combat_pending,plot_ui_pending,hurry_population_pending,improvement_yield_pending,improvement_plot_yield_pending,free_promotions_pending,unit_promotions_pending,capital_reveal_pending,other_minor_friendship_pending,city_effects_pending,city_free_building_pending,city_garrison_pending,city_building_effects_pending,policy_disables_pending,recursive_policy_disable_pending,golden_age_pending,golden_age_map_pending,golden_age_plot_ui_pending,golden_age_history_pending,golden_age_replay_pending,golden_age_replay_executing,golden_age_gameplay_pending,golden_age_gameplay_executing,golden_age_notification_pending,golden_age_notification_executing,golden_age_dirty_pending,golden_age_dirty_executing,free_techs_pending,free_techs_executing,free_tech_dirty_pending,free_tech_dirty_executing,free_tech_notification_pending,free_tech_notification_executing,free_policy_notification_pending,one_shot_free_units_pending,free_policy_notification_executing,free_great_people_pending,free_units_create_pending,minor_great_people_allies_pending,reformation_pending,reformation_executing,reformation_returned,post_reformation_pending,spy_progress_pending,resource_policy_yield_pending,resource_policy_layout_pending,resource_policy_layout_executing,resource_policy_layout_returned,policy_happiness_pending,policy_happiness_founder_pending,policy_happiness_foreign_founder_pending,policy_happiness_suffix_pending,policy_happiness_dirty_pending,policy_happiness_dirty_executing,policy_happiness_dirty_returned,policy_trade_pending,policy_great_people_traits_pending,policy_great_people_policies_pending,policy_great_people_friendships_pending,policy_post_gp_cost_pending,policy_camp_visibility_pending,policy_camp_plot_ui_pending,policy_camp_notification_pending,policy_process_dirty_pending,policy_process_dirty_executing,policy_outer_next_cost_pending,policy_outer_branch_pending,policy_outer_branch_rewards_pending,policy_outer_dirty_pending,policy_outer_dirty_executing,policy_outer_hook_pending,policy_outer_hook_executing,policy_outer_yield_pending,policy_outer_plot_ui_pending,adoption_returned,policy_outer_branch_notifications_pending,policy_outer_branch_notifications_executing,policy_outer_branch_notifications_returned,policy_outer_branch_publication_pending};
// Historical scalar stores only. A locked branch and pending notifications
// remain OPEN; this value grants no current source/GUI/unlock permission.
struct BnwPolicyBranchCreditHistory {
 std::int32_t branch=-1,delta=0;
 bool tenets=false,free_policy_notification_required=false;
 std::int64_t credits_before=0,credits_after=0,ever_before=0,ever_after=0;
 std::uint64_t revision_before=0,revision_after=0;
};
// Measured actual Add/FIFO history. No delivery/current Scene authority.
struct BnwPolicyBranchNotificationOperands {
 std::int32_t ordinal=-1,recipient=-1,actor=-1,branch=-1;
 bool free_policy=false,skip=false,has_met=false,present=false;
 std::uint64_t generation=0,revision=0;
 bool operator==(const BnwPolicyBranchNotificationOperands&)const=default;
};
struct BnwPolicyBranchNotificationHistory {
 BnwPolicyBranchNotificationOperands operands;
 bool inserted=false;
 std::uint64_t revision_after=0,client_effects_emitted=0;
};
enum class BnwPolicyUiEffect {game_data,policy_notification_seen,achievement_xp2_45};
struct BnwPolicyUiRequest {std::uint64_t occurrence=0,generation=0,revision=0;std::int64_t player=-1;BnwPolicyUiEffect effect;bool value=false;};
// Measured checkpoint projection, not a permission to overwrite live state.
// A writer must retain and authenticate the opaque pending owner separately.
struct BnwPolicyNativeObservation {
 std::int64_t player=-1,culture=0,cost=0,free_policies=0,free_tenets=0,count=0;
 std::map<std::int64_t,bool> has,unlocked,blocked,finished,one_shot,one_shot_free_units;
 std::int64_t extra_branches=0;
 std::array<std::int64_t,3> history{};
 std::map<std::string,std::int64_t,std::less<>> process_scalars;
 std::map<std::string,std::array<std::int64_t,6>,std::less<>> process_arrays;
 std::map<std::int64_t,std::pair<std::vector<std::int64_t>,std::vector<std::int64_t>>> city_yield_ranks,city_yield_rank_numbers;
 std::map<std::string,std::vector<std::int64_t>,std::less<>> combat_hurry_arrays;
 std::vector<std::int64_t> improvement_yields;
 std::map<std::int64_t,std::int64_t> free_promotions,city_policy_culture;
 std::vector<BnwPolicyUiRequest> ui;
 std::map<std::int64_t,std::int64_t> city_strengths;
 std::vector<BnwCityDirtyEvent> city_dirty_events;
 bool owned_pair=false;std::uint64_t generation=0,revision=0;
};
BnwPolicyNativeObservation observe_bnw_policy_native(const ui::SelectedBnwPreviewHost&,std::int64_t player);
// Closed SAME-VM issuance. These targets are never a public TestAll provider.
class BnwPolicyAdoptionHooks final {
 public:~BnwPolicyAdoptionHooks();
 BnwPolicyAdoptionHooks(const BnwPolicyAdoptionHooks&)=delete;
 private:
 friend class BnwSelectedStockLuaSourceOwner;friend class BnwPolicyAdoptionPending;
 using CanAdopt=std::function<std::optional<bool>(std::int64_t,std::int64_t)>;
 using Accept=std::function<void(const BnwPolicyAdoptionPending&)>;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwPolicyAdoptionHooks(std::unique_ptr<Impl>);
 static std::shared_ptr<const BnwPolicyAdoptionHooks> issue(
  std::shared_ptr<const ui::SelectedBnwPreviewHost>,BnwStockLuaAttemptLease,CanAdopt,Accept,
  std::shared_ptr<const BnwSelectedStockLuaSourceOwner>);
};
// Native ordered prefix through payment, HasPolicy, policy-count/history,
// policy-notification transport and non-finishing branch flag. No successful
// adoption/UI/save claim before processPolicies and the remaining suffix.
class BnwPolicyAdoptionPending final {
 public:
 ~BnwPolicyAdoptionPending();
 BnwPolicyAdoptionPending(const BnwPolicyAdoptionPending&)=delete;
 static std::shared_ptr<const BnwPolicyAdoptionPending> execute(LiveSession&,
  std::shared_ptr<const BnwPolicyAdoptionHooks>,const BnwPolicyAdoptionCommand&);
 BnwPolicyAdoptionPhase phase()const;
 std::uint8_t yield_cursor()const;
 std::optional<std::size_t> hurry_frontier()const;
 std::uint64_t improvement_yield_cursor()const;
 std::size_t free_promotion_cursor()const;
 std::size_t city_effect_cursor()const;
 std::uint8_t reward_cursor()const;
 // Measured actual native change, not authority to finish its callbacks.
 std::array<std::int32_t,8> golden_age_operands()const;
 bool golden_age_turns_stored()const;
 bool golden_age_change_returned()const;
 bool golden_age_map_returned()const;
 bool golden_age_count_stored()const;
 bool golden_age_replay_returned()const;
 bool golden_age_gameplay_stored()const;
 bool golden_age_gameplay_returned()const;
 std::optional<std::int64_t> finishing_policy()const;
 bool finisher_granted()const;
 bool finisher_ever_stored()const;
 bool finisher_returned()const;
 std::int64_t observed_effect_policy()const;
 std::optional<BnwPolicyNativeObservation> finisher_process_before()const;
 std::optional<BnwPolicyNativeObservation> finisher_process_after()const;
 bool golden_age_notification_returned()const;
 // Measured history only; none of these diagnostic values issues an action.
 bool reformation_notification_native_returned()const;
 bool reformation_notification_returned()const;
 bool reformation_notification_inserted()const;
 std::shared_ptr<const saves::PlayerAdministrationRecord> reformation_notification_before()const;
 bool free_tech_count_stored()const;
 bool free_tech_choice_returned()const;
 std::array<std::int32_t,4> free_tech_choice_operands()const;
 bool golden_age_achievement_backend_disabled()const;
 std::uint64_t golden_age_map_scanned()const;
 bool resource_policy_loop_reached()const;
 bool resource_policy_loop_returned()const;
 std::uint64_t resource_policy_scanned()const;
 std::vector<std::int64_t> resource_policy_yield_returned_plots()const;
 std::optional<std::int64_t> resource_policy_layout_plot()const;
 std::vector<std::int64_t> resource_policy_layout_returned_plots()const;
 bool resource_policy_layout_effects_current()const;
 std::uint8_t units_reward_cursor()const;
 std::uint8_t great_people_reset_cursor()const;
 std::uint8_t great_people_traits_cursor()const;
 std::array<std::int32_t,3> great_people_traits_operands()const;
 std::uint8_t great_people_modifiers_cursor()const;
 std::array<std::int32_t,12> great_people_modifiers_operands()const;
 bool great_people_recompute_returned()const;
 std::array<std::int32_t,4> great_people_suffix_operands()const;
 std::uint8_t post_great_people_scalar_cursor()const;
 std::array<std::int32_t,4> post_great_people_scalar_operands()const;
 std::uint64_t camp_visibility_cursor()const;
 bool camp_visibility_returned()const;
 bool process_policies_returned()const;
 bool early_process_policies_returned()const;
 bool next_policy_cost_returned()const;
 bool outer_policy_branch_returned()const;
 std::optional<std::int32_t> outer_branch_free_policy_operand()const;
 std::optional<BnwPolicyBranchCreditHistory> outer_branch_credit_history()const noexcept;
 std::vector<BnwPolicyBranchNotificationHistory> outer_branch_notification_history()const;
 bool outer_policy_dirty_returned()const;
 bool outer_policy_hook_returned()const;
 bool outer_policy_yield_returned()const;
 std::uint64_t outer_policy_yield_scanned()const;
 std::array<std::int32_t,4> next_policy_cost_operands()const;
 std::optional<std::int64_t> camp_visibility_changed_plot()const;
 std::vector<BnwHappinessNativeStore> happiness_stores()const;
 std::vector<BnwHappinessSuffixStore> happiness_suffix_stores()const;
 bool happiness_achievement_requested()const;
 bool happiness_native_suffix_returned()const;
 bool happiness_dirty_returned()const;
 bool happiness_achievement_backend_disabled()const;
 std::map<std::int64_t,bool> observed_city_garrisons()const;
 std::int64_t player()const;std::int64_t policy()const;
 std::uint64_t before_revision()const;std::uint64_t previous_revision()const;std::uint64_t after_revision()const;
 void require_stored_current(const ui::SelectedBnwPreviewHost&)const;
 private:friend class ui::BnwClientSelectionControl;friend class BnwMajorPlayerTraitsOwner;friend struct BnwPolicyHappinessMathAccess;friend class BnwSelectedStockLuaSourceOwner;friend class BnwPlotYieldRefreshPending;friend class ui::BnwEngineUiTurnOwner;
 // The production SourceOwner retains the genuine issued owner BEFORE any
 // initial native store. A throwing return expression cannot erase history.
 // The sink is borrowed only for this call; no pointer/callback persists.
 static std::shared_ptr<const BnwPolicyAdoptionPending> execute_owned(LiveSession&,
  std::shared_ptr<const BnwPolicyAdoptionHooks>,const BnwPolicyAdoptionCommand&,
  std::shared_ptr<const BnwPolicyAdoptionPending>& issued_receipt);
 static std::shared_ptr<const BnwPolicyAdoptionPending> retain_factory_owner(const std::shared_ptr<const BnwPolicyAdoptionPending>&);
 // Pure historical observation for the SAME closed SourceOwner after a late
 // throwing bookend. It grants neither currentness nor action/UI completion.
 bool observed_earned_native_return_for(const BnwSelectedStockLuaSourceOwner&)const noexcept;
 void require_free_policy_notification_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 BnwStockLuaAttemptLease begin_free_policy_notification_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_free_policy_notification_return(const ui::BnwEngineUiTurnOwner&)const;
 void fail_free_policy_notification_dispatch()const noexcept;
 void advance_branch_finisher()const;
 void resume_after_finisher_process()const;
 std::int64_t effect_policy()const;
 void advance_process_initial_scalars()const;
 void advance_process_worker_scalars()const;
 void advance_process_treasury_scalars()const;
 void advance_process_city_scalars()const;
 void advance_process_extra_scalars()const;
 void advance_process_one_shot()const;
 void advance_process_yield_arrays()const;
 void advance_process_plot_yields()const;
 void consume_actual_policy_plot_ui()const;
 void advance_process_combat_hurry()const;
 void advance_process_improvement_yields()const;
 void advance_process_free_promotions()const;
 void advance_process_city_effects()const;
 void advance_process_rewards()const;
 void advance_process_golden_age()const;
 void advance_process_golden_age_history()const;
 BnwStockLuaAttemptLease begin_golden_age_replay_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_golden_age_replay_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_golden_age_replay_return(const ui::BnwEngineUiTurnOwner&)const;
 void fail_golden_age_replay_dispatch()const noexcept;
 BnwStockLuaAttemptLease begin_golden_age_gameplay_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_golden_age_gameplay_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_golden_age_gameplay_return(const ui::BnwEngineUiTurnOwner&)const;
 void fail_golden_age_gameplay_dispatch()const noexcept;
 BnwStockLuaAttemptLease begin_golden_age_notification_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_golden_age_notification_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_golden_age_notification_return(const ui::BnwEngineUiTurnOwner&)const;
 void fail_golden_age_notification_dispatch()const noexcept;
 BnwStockLuaAttemptLease begin_golden_age_dirty_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_golden_age_dirty_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_golden_age_dirty_return(const ui::BnwClientSelectionControl&)const;
 void fail_golden_age_dirty_dispatch()const noexcept;
 BnwStockLuaAttemptLease begin_free_tech_text_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_free_tech_text_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_free_tech_text_and_count(const ui::BnwEngineUiTurnOwner&)const;
 BnwStockLuaAttemptLease begin_free_tech_dirty_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_free_tech_dirty_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_free_tech_dirty_return(const ui::BnwClientSelectionControl&)const;
 BnwStockLuaAttemptLease begin_free_tech_notification_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_free_tech_notification_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_free_tech_notification_return(const ui::BnwEngineUiTurnOwner&)const;
 void fail_free_tech_dispatch()const noexcept;
 void advance_process_reward_tail()const;
 void advance_process_one_shot_units()const;
 // Closed actual Reformation native Add and one-shot post-Add continuation.
 BnwStockLuaAttemptLease begin_reformation_notification_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_reformation_notification_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void fail_reformation_notification_dispatch()const noexcept;
 std::uint64_t private_reformation_operation_serial()const;
 void accept_actual_reformation_notification_return(const ui::BnwEngineUiTurnOwner&)const;
 void advance_process_post_reformation_tail()const;
 void advance_process_happiness_prefix()const;
 void advance_process_happiness_suffix()const;
 BnwStockLuaAttemptLease begin_happiness_dirty_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_happiness_dirty_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_happiness_dirty_return(const ui::BnwClientSelectionControl&)const;
 void fail_happiness_dirty_dispatch()const noexcept;
 std::uint64_t private_happiness_dirty_operation_serial()const;
 void resume_after_happiness_dirty()const;
 void advance_process_trade_and_great_people_prefix()const;
 void advance_process_great_people_traits()const;
 void advance_process_great_people_policy_buildings()const;
 void advance_process_great_people_suffix()const;
 void advance_process_policy_cost_and_experience()const;
 void advance_process_camp_visibility()const;
 void advance_outer_next_policy_cost()const;
 void advance_outer_policy_branch()const;
 // Source-compatible ordered branch reward PREFIX. Positive credits are
 // actually stored; pending notifications are never a completed unlock.
 void advance_outer_policy_branch_zero_reward()const;
 BnwStockLuaAttemptLease begin_outer_branch_notification_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_outer_branch_notification_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 BnwPolicyBranchNotificationOperands outer_branch_notification_operands_without_callbacks()const;
 std::uint64_t private_outer_branch_notification_serial()const;
 void accept_actual_outer_branch_notification_return(const ui::BnwEngineUiTurnOwner&)const;
 void fail_outer_branch_notification_dispatch()const noexcept;
 void finish_outer_branch_notifications_without_publication()const;
 BnwStockLuaAttemptLease begin_outer_policy_dirty_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_outer_policy_dirty_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 bool outer_policy_actor_is_active(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_outer_policy_dirty_return(const ui::BnwClientSelectionControl&)const;
 void fail_outer_policy_dirty_dispatch()const noexcept;
 BnwStockLuaAttemptLease begin_outer_policy_hook_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_outer_policy_hook_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_outer_policy_hook_return(const BnwSelectedStockLuaSourceOwner&)const;
 void fail_outer_policy_hook_dispatch()const noexcept;
 BnwStockLuaAttemptLease begin_policy_process_dirty_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_policy_process_dirty_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void accept_actual_policy_process_dirty_return(const ui::BnwClientSelectionControl&)const;
 void fail_policy_process_dirty_dispatch()const noexcept;
 void require_policy_cost_observer_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::int64_t,std::int64_t,std::int64_t,std::uint64_t,std::uint64_t,bool)const;
 std::int64_t begin_happiness_founder_dispatch()const;
 void finish_happiness_founder_dispatch(std::optional<std::int64_t>)const;
 std::shared_ptr<const ui::SelectedBnwPreviewHost> require_happiness_math_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::int64_t)const;
 void require_unissued_policy_plot_source(const BnwPlotYieldRefreshIssuer&,const LiveSessionState&,const saves::SavedSession&,const SessionCheckpointIdentity&,std::int64_t)const;
 std::pair<std::uint64_t,std::uint64_t> private_policy_plot_identity()const;
 bool finish_resource_policy_plot()const;
 BnwStockLuaAttemptLease begin_resource_policy_layout_dispatch(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_resource_policy_layout_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 std::int64_t private_resource_policy_layout_plot()const;
 std::uint64_t private_resource_policy_layout_operation_serial()const;
 void accept_actual_resource_policy_layout_return(const ui::BnwClientSelectionControl&)const;
 void fail_resource_policy_layout_dispatch()const noexcept;
 void resume_after_resource_policy_layout()const;
 const std::thread::id creator_=std::this_thread::get_id();mutable std::atomic_bool retired_{false};
 struct Impl;mutable std::unique_ptr<Impl> impl_;
 explicit BnwPolicyAdoptionPending(std::unique_ptr<Impl>);
};
}
