#pragma once
#include "civ5/runtime/bnw_attrition_city_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_unit_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_unit_selected_read.hpp"
#include "civ5/runtime/bnw_attrition_selected_unit_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_selection_plot_read.hpp"
#include "civ5/runtime/bnw_attrition_plot_list_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_damage_dll_wrapper_prefix.hpp"
#include "civ5/runtime/bnw_game_turn_timer_owner.hpp"
#include <cstdint>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>
namespace civ5::ui { class SelectedBnwPreviewHost; class LiveUiHost; class BnwClientSelectionControl; class BnwEngineUiTurnOwner; }
namespace civ5::ui {class BnwOwnedCameraGuiScreen;class BnwClientSceneFrame;}
namespace civ5::runtime {
class BnwNativeUnitAllocatorCore;class BnwNativeAttritionPrefix;struct BnwAttritionPrefixProgress;struct BnwAttritionMountainProgress;struct BnwAttritionDamageProgress;struct BnwAttritionGarrisonProgress;struct BnwAttritionCityStrengthProgress;class BnwNativePlayerBusyUnitsReturn;class BnwNativePlayerBusyCitiesReturn;class BnwPlayerTurnActiveDirtyIssuer;class BnwNetMessageDebugManagerOwner;enum class BnwNetDebugOwnDelivery;
class LiveSession;struct LiveSessionState;class BnwSelectedStockLuaBootstrap;class BnwStockLuaAttemptLease;class BnwSelectedStockLuaSourceOwner;
enum class BnwRetainedTurnFrontier { timer_reset, selection_cycle_owner_unavailable, engine_utility_do_turn_unavailable, barbarians_begin, game_cache_update, world_religion_tech_progress, game_update_score, game_deals_turn, teams_turn, first_team_minor_technology, first_team_small_awards, first_team_circumnavigation, first_team_post_circumnavigation, remaining_teams_turn, map_turn_unavailable, quarantined, retired, next_team_minor_technology, pending_team_small_awards, pending_team_circumnavigation, pending_team_return_tail, map_plots_turn, engine_ui_do_turn_unavailable, barbarian_camps_turn, barbarian_camp_plot_finder_unavailable, barbarian_units_turn, game_religions_turn, religion_holy_city_pressure_unavailable, religion_spy_pressure_unavailable, game_trade_turn_unavailable, engine_ui_active_summary_unavailable, engine_ui_net_gold_unavailable, engine_ui_deficit_localization_unavailable, game_leagues_turn_unavailable, game_culture_turn_unavailable, culture_public_opinion_source_owner, engine_ui_end_turn_reset_unavailable, culture_ideology_pressure_owner, culture_notification_source_owner, culture_log_source_owner, culture_next_player_source_owner, game_ai_autoplay_source_owner, game_turn_increment_source_owner, game_turn_paired_clock_store_owner, game_turn_ui_dirty_owner, game_turn_sent_auto_moves_owner, game_turn_gameplay_changed_owner, game_turn_timer_reset_owner, game_turn_elapsed_increment_owner, game_turn_dynamic_turns_owner, game_turn_dynamic_notification_owner, game_turn_simultaneous_shuffle_owner, game_turn_player_activation_owner, game_turn_victory_owner, game_turn_player_activation_dirty_owner, game_turn_player_activation_dirty_delivery_owner, game_turn_player_end_turn_owner, game_turn_player_end_turn_busy_owner, game_turn_player_end_turn_processed_auto_owner, game_turn_player_unit_attrition_owner,game_turn_player_attrition_minor_ob_owner,game_turn_player_attrition_damage_owner,game_turn_player_attrition_mountain_owner,game_turn_player_attrition_mountain_damage_owner,game_turn_player_attrition_death_owner,game_turn_player_deferred_fog_owner,game_turn_player_active_count_owner, game_turn_player_busy_runtime_owner, game_turn_player_busy_city_owner, game_turn_player_busy_city_runtime_owner, game_turn_player_remote_turn_complete_owner, game_turn_player_end_turn_after_busy_owner,game_turn_player_set_damage_garrison_owner,game_turn_player_set_damage_death_owner,game_turn_player_set_damage_city_strength_owner,game_turn_player_set_damage_info_bar_dirty_owner,game_turn_player_set_damage_city_info_dirty_owner,game_turn_player_set_damage_selected_owner,game_turn_player_set_damage_selected_info_dirty_owner,game_turn_player_set_damage_selection_plot_owner,game_turn_player_set_damage_plot_list_dirty_owner,game_turn_player_set_damage_gameplay_damage_owner,game_turn_player_set_damage_popup_owner };
// Actual source return observed by operation12, never a caller permission.
enum class BnwCulturePlatformStatus { not_reached, disabled_standalone };
enum class BnwCultureDiagnosticStatus { not_reached, backend_disabled };
enum class BnwRetainedCircumnavigationReturn { already_circumnavigated, map_not_wrapped, land_density, unrevealed_x_column, unrevealed_y_row };
// Retained actual prefix history only. It grants no currentness or continuation.
struct BnwDynamicTurnModeProgress {
    bool attempted=false,native_prefix_returned=false,source_adopted=false;
    std::int64_t dynamic_option=0,simultaneous_option=0;
    std::optional<std::int32_t> sequential_player;
    std::uint64_t teams_visited=0,returned_setters=0,native_scalar_stores=0;
    std::optional<std::pair<std::int32_t,std::int32_t>> notification_frontier;
};
// Historical shuffle effects only; no scheduler continuation permission.
struct BnwTurnShuffleProgress {
 bool attempted=false,rng_stored=false,source_adopted=false,scan_returned=false,ai_loop_returned=false;
 std::uint64_t draws=0,scanned=0,equal_active_returns=0,seed_before=0,seed_after=0,calls_before=0,calls_after=0;
 std::optional<std::int32_t> pending_player;
 std::array<std::int32_t,64> permutation{};
};
// Actual changed Player member history. Dirty publication remains mandatory.
struct BnwPlayerTurnActiveProgress {
 bool attempted=false,stored=false,source_adopted=false,publication_returned=false;
 bool publication_attempted=false,queued=false,delivery_attempted=false,delivered=false;
 std::optional<std::int32_t> player;
 std::uint64_t before_revision=0,after_revision=0;
};
// Creator-only earned history; no continuation/save/full-turn permission.
struct BnwPlayerEndTurnProgress {
 bool attempted=false,end_flag_stored=false,source_adopted=false,setter_returned=false;
 bool busy_guard_required=false,processed_auto_owner_required=false;
 std::optional<std::int32_t> player;
 std::optional<bool> before_end_turn,before_auto_moves;
 std::uint64_t before_revision=0,after_revision=0;
};
// Retained actual query/error history only; no saved/ready boolean grant.
struct BnwPlayerBusyUnitsProgress {
 bool attempted=false,issued=false,native_prefix_returned=false,units_query_completed=false,setter_early_returned=false;
 std::uint64_t queried_units=0;std::optional<bool> has_busy_unit;std::optional<std::int32_t> required_unit;
};
struct BnwProcessedAutoMovesProgress {
 bool attempted=false,issued=false,auto_stored=false,processed_stored=false,source_adopted=false,setter_returned=false;
 std::optional<std::int32_t>player;std::optional<bool>before_processed;
 std::uint64_t before_revision=0,after_revision=0,owner_before=0,owner_after=0;
};
struct BnwPlayerEndAfterBusyProgress {
 bool attempted=false,issued=false,end_flag_stored=false,source_adopted=false,setter_returned=false,processed_auto_owner_required=false;
 std::optional<std::int32_t>player;std::optional<bool>before_end_turn,before_auto_moves;
 std::uint64_t before_revision=0,after_revision=0;
};
struct BnwPlayerBusyCitiesProgress {
 bool attempted=false,issued=false,native_prefix_returned=false,cities_query_completed=false,setter_early_returned=false;
 std::uint64_t queried_cities=0;std::optional<bool>has_busy_city;std::optional<std::int32_t>required_city;
};
struct BnwRetainedTurnView {
    BnwRetainedTurnFrontier frontier=BnwRetainedTurnFrontier::timer_reset;
    std::uint64_t operation=0;
    bool timer_attempted=false;
    bool selection_cycle_attempted=false;
    bool engine_utility_attempted=false;
    bool barbarians_attempted=false;
    bool game_cache_attempted=false;
    bool religion_progress_attempted=false;
    bool score_attempted=false;
    bool deals_attempted=false;
    bool first_team_war_attempted=false;
    std::optional<std::int32_t> first_team;
    bool first_team_no_trade_attempted=false;
    std::optional<bool> first_team_is_minor;
    bool first_team_small_awards_attempted=false;
    std::optional<std::uint64_t> first_team_small_award_slots;
    bool first_team_circumnavigation_attempted=false;
    std::optional<BnwRetainedCircumnavigationReturn> first_team_circumnavigation_return;
    std::optional<std::uint64_t> first_team_circumnavigation_axis;
    bool first_team_return_attempted=false;
    std::optional<std::uint64_t> first_team_tail_assertions;
    std::optional<std::int32_t> team_loop_next_slot;
    std::optional<std::int32_t> next_alive_team;
    bool next_team_war_attempted=false;
    std::optional<std::int32_t> next_team;
    std::optional<std::int32_t> active_team;
    std::optional<bool> active_team_is_minor;
    std::optional<std::uint64_t> active_team_small_award_slots;
    std::optional<BnwRetainedCircumnavigationReturn> active_team_circumnavigation_return;
    std::optional<std::uint64_t> active_team_circumnavigation_axis;
    std::uint64_t completed_teams=0;
    std::optional<std::int32_t> last_completed_team;
    bool map_attempted=false;
    std::uint64_t completed_map_plots=0;
    std::optional<std::uint64_t> active_map_plot;
    bool camps_turn_attempted=false,units_turn_attempted=false;
    std::optional<std::int32_t> camps_native_to_add;
    std::optional<bool> camps_native_wants_coastal;
    bool religion_city_turn_attempted=false;
    std::optional<std::pair<std::int32_t,std::int32_t>> active_religion_city;
    bool engine_ui_attempted=false,engine_ui_returned=false;
    std::optional<std::int32_t> engine_ui_player;
    std::optional<std::uint64_t> engine_ui_earned_revision;

};
// Read-only first due DoMinorCivTech query on actual retained owners.
// This DTO grants no HasTech mutation, Lua/engine event, execution or readiness.
struct BnwRetainedMinorTechnologyPlan {
    std::int32_t team=-1, leader=-1;
    std::int64_t technology=-1;
    std::int32_t alive_major_teams=0, major_owners=0;
    std::int64_t teams_needed=0;
    bool operator==(const BnwRetainedMinorTechnologyPlan&) const = default;
};
enum class BnwRetainedTeamsStop { blocked, map_boundary, quarantined };
struct BnwRetainedTeamsDiagnostic {
    BnwRetainedTurnView before,after;
    std::uint64_t returned_operations=0;
    BnwRetainedTeamsStop stop=BnwRetainedTeamsStop::blocked;
    std::string blocker;
};
struct BnwRetainedMapDiagnostic {
    BnwRetainedTurnView before,after;
    std::uint64_t returned_plots=0;
    std::string blocker;
};
// Actual bounded CvGame::doTurn prefix, NOT a completed engine/game turn.
// First binding must occur on the actual live world owner thread. Subsequent
// calls enforce that private thread before borrowing the facade/world.
// Session ownership must be genuine (no no-op deleter). The host overload
// retains the real parent through an aliasing shared_ptr.
class BnwRetainedTurnCursor final {
  public:
    static std::shared_ptr<BnwRetainedTurnCursor> bind(std::shared_ptr<const LiveSession>);
    static std::shared_ptr<BnwRetainedTurnCursor> bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>);
    ~BnwRetainedTurnCursor();
    BnwRetainedTurnCursor(const BnwRetainedTurnCursor&)=delete;
    BnwRetainedTurnCursor& operator=(const BnwRetainedTurnCursor&)=delete;
    BnwRetainedTurnView view() const;
    BnwDynamicTurnModeProgress dynamic_turn_mode_progress() const;
    BnwTurnShuffleProgress turn_shuffle_progress() const;
    BnwPlayerTurnActiveProgress player_turn_active_progress() const;
    BnwPlayerEndTurnProgress player_end_turn_progress()const;
    BnwPlayerBusyUnitsProgress player_busy_units_progress()const;
    BnwPlayerBusyCitiesProgress player_busy_cities_progress()const;
    BnwPlayerEndAfterBusyProgress player_end_after_busy_progress()const;
    BnwProcessedAutoMovesProgress processed_auto_moves_progress()const;
    BnwAttritionPrefixProgress attrition_first_progress()const;
    BnwAttritionMountainProgress attrition_mountain_progress()const;
    BnwAttritionDamageProgress attrition_damage_progress()const;
    BnwAttritionGarrisonProgress attrition_garrison_progress()const;
    BnwAttritionCityStrengthProgress attrition_city_strength_progress()const;
    BnwRetainedTurnView advance_game_tail_to_next_required_owner();
    // Autoplay=0 is checked before stores. The private attempt is recorded
    // BEFORE the real timer owner's reset(true). No caller completion input.
    // Success leaves the cursor BEFORE a required actual initialized UI receiver.
    BnwGameTurnTimerSample execute_timer_prefix();
    // SAME actual initialized Civis UI; no caller bool or completion receipt.
    // Runs only after this cursor's actual timer reset, once. Stops before DoTurn.
    void execute_selection_cycle_prefix(const ui::LiveUiHost&);
    // Executes the private source-equivalent empty retail403694 DX9 BNW body.
    // Requires this SAME actual Host/UI operation 2; no receipt/callback input.
    // Once history precedes the actual call. Return stops BEFORE barbarians.
    void execute_engine_utility_prefix();
    // SAME live counter arrays, source order, once AFTER actual operation3.
    // No NoBarbarians skip: that option gates later DoCamps/DoUnits only.
    // Return stops BEFORE game caches, score, technology grants and Lua.
    void execute_barbarians_begin_prefix();
    // SDK9875-9878 is an empty body, not a native-disassembly parity claim.
    // SAME actual stock Host/UI operation4 is required. Stops before religion.
    void execute_game_cache_prefix();
    // Precompute from SAME actual Host rules and owned live team state, then
    // store only prefix.scalar23 in the SAME allocation. Stops BEFORE score.
    // Changed cache invalidates captured frames through ui_source_revision.
    void execute_world_religion_tech_progress_prefix();
    // Actual source condition and SAME Host rules/UI. Dirty scores include all
    // six arrays, 63 replay updates and Score_DIRTY_BIT=12 when a score changes.
    // Stops BEFORE m_kGameDeals.DoTurn; no full-turn readiness or persistence.
    void execute_game_score_prefix();
    // Complete SDK no-expiration/no-resource-query branch on SAME world.
    // Required ending/resource effects refuse before any operation8 attempt.
    // Returns BEFORE the first live CvTeam::doTurn loop.
    void execute_game_deals_prefix();
    // Source first alive team only; actual nonbarbarian war/lock scalar stores.
    // Stops BEFORE isMinorCiv/DoMinorCivTech, not before/after all team turns.
    // Barbarian-first or no-team require other mandatory producers and refuse.
    void execute_first_alive_team_war_prefix();
    // Actual source isMinorCiv then nonminor SetNoTrade(false), SAME team.
    // True minor requires DoMinorCivTech and refuses BEFORE any operation10.
    // Paired raw/owned stores stop BEFORE DoTestSmallAwards, not team completion.
    void execute_first_team_nonminor_no_trade_prefix();
    // Actual DoTestSmallAwards collection and SAME decoded team/city state.
    // Completes only when every source award predicate has no effect. Reached
    // award/points/notification owners refuse BEFORE operation11 or any store.
    // Stops BEFORE testCircumnavigated; does not complete any team/game turn.
    void execute_first_team_small_awards_prefix();
    // Real testCircumnavigated after SAME actual operation11. Executes proved
    // source early returns; reached makeCircumnavigated requires missing full
    // flag/moves/message/Lua/replay persistence owners and refuses preattempt.
    // Stops at post-circumnavigation tail, never completes the team/game loop.
    void execute_first_team_circumnavigation_prefix();
    // Actual CvTeam return tail and CvGame ascending loop after SAME operation12.
    // Unknown FINAL_RELEASE is not guessed: all reached debug assertions must
    // be concretely effect-free. Stops BEFORE the next alive team or CvMap turn.
    // This returns only the first team, never all teams/the game/player turn.
    void execute_first_team_return_and_advance_prefix();
    // SAME actual op13 pending loop slot, next alive nonbarbarian team only.
    // Common war/lock source stores stop BEFORE isMinorCiv/DoMinorCivTech.
    // Barbarian63 refuses before op14: DoBarbarianTech effects are mandatory.
    void execute_next_alive_team_war_prefix();
    // Common ascending queue: team/phase come only from SAME private history.
    // Each method executes exactly one actual admitted source phase. Required
    // technology, award, bonus, assertion and map effects never become skips.
    void execute_pending_team_war_prefix();
    void execute_pending_team_no_trade_prefix();
    // SAME private first/pending minor-tech boundary only, no chosen team ID.
    // nullopt proves no due grant (or actual nonminor); required missing data
    // still throws. Positive plan is diagnostic, not an acquisition receipt.
    std::optional<BnwRetainedMinorTechnologyPlan> pending_minor_technology_plan() const;
    void execute_pending_team_small_awards_prefix();
    void execute_pending_team_circumnavigation_prefix();
    void execute_pending_team_return_and_advance_prefix();
    // Diagnostic continuation preserves the committed prefix on a mandatory
    // barrier; no rollback, full-turn success or persistence receipt is granted.
    BnwRetainedTeamsDiagnostic execute_pending_teams_until_boundary();
    // SAME private completed team loop only. Executes exactly the next plot
    // in original index order; no plot ID, callback or completion witness.
    // Required unit relocation/anchor effects refuse before this plot attempt.
    // A failure after stores quarantines and preserves the actual prefix.
    void execute_next_map_plot_prefix();
    BnwRetainedMapDiagnostic execute_map_plots_until_boundary();
    // Strict source order AFTER actual ICvUserInterface2::doTurn return.
    // The owned EngineUI receiver must actually return its admitted branch;
    // these bodies cannot be reached by a caller bool/receipt or skipped UI.
    // Native camp draw prefix remains before real finder/creation effects.
    void execute_engine_ui_prefix();
    void execute_fresh_engine_ui_end_turn_reset();
    void execute_game_ai_autoplay_zero_return();
    // Original setGameTurn log BEFORE any clock/score/UI/timer stores. A typed
    // Boot-owned manager must belong to the genuine SAME Host resource Parent.
    void execute_game_turn_net_log_prefix(std::shared_ptr<const BnwNetMessageDebugManagerOwner>);
    std::optional<BnwNetDebugOwnDelivery> game_turn_net_log_delivery()const;
    void execute_barbarian_camps_prefix();
    // Completes only genuine NoBarbarians/elapsed<10/no-due-camp branches.
    // Actual spawn and camp activation remain mandatory before religion.
    void execute_barbarian_units_prefix();
    // First actual alive-player/forward native-city slot only, after real
    // DoUnits. Stores original trade-route pressure reset, then stops before
    // holy/spy/adjacent pressure; no city/religion return is fabricated.
    void execute_game_religion_first_city_prefix();
    // Actual zero-pressure route only after genuine first-city Reset.
    // Complete current raw/owned religion and city operands prove every
    // holy/spy/adjacent call unreachable; remaining native resets execute.
    void execute_game_religion_no_pressure_prefix();
    // Genuine GameTrade Reset/Build technology cache, without moving routes.
    void execute_game_trade_technology_difference_prefix();
    // Actual diplo votes/cache + LeagueScreen, current no-congress founding branch.
    void execute_game_leagues_unfounded_prefix();
    // Actual first living player influence copy + initial public-opinion stores.
    // Reached met-civ tourism/Locale/ideology effects remain mandatory.
    void execute_game_culture_first_unmet_prefix();
    void execute_game_culture_public_opinion_prefix();
    void execute_game_culture_unmet_notifications_standalone_prefix();
    BnwCulturePlatformStatus game_culture_platform_status()const;
    void execute_game_culture_standalone_diagnostics_return();
    void execute_game_culture_next_player_unmet_prefix();
    BnwCultureDiagnosticStatus game_culture_diagnostic_status()const;
    std::uint64_t completed_game_culture_players()const;
    std::optional<std::int32_t> active_game_culture_player()const;
    // Optional unknown until this actual owned UI setter is reached.
    std::uint64_t turn_timer_dirty_setter_count(const ui::LiveUiHost&)const;
    std::optional<bool> turn_timer_dirty(const ui::LiveUiHost&)const;
    std::uint64_t game_data_dirty_setter_count(const ui::LiveUiHost&)const;
 std::optional<bool> game_data_dirty(const ui::LiveUiHost&)const;
 std::optional<bool> score_dirty(const ui::LiveUiHost&)const;
    std::uint64_t score_dirty_setter_count(const ui::LiveUiHost&)const;
    // Unknown until the actual setter has executed. Not an imported EXE flag.
    bool no_selection_list_cycle(const ui::LiveUiHost&) const;
    std::shared_ptr<const BnwGameTurnTimerOwner> timer_owner() const;
    void require_current() const;
    void retire() noexcept;
  private:
    friend class BnwSelectedStockLuaSourceOwner;
    friend class BnwPlayerTurnActiveDirtyIssuer;
    friend class BnwAttritionCityInfoDirtyLoan;friend class BnwNativeAttritionPrefix;
    friend class BnwAttritionUnitInfoDirtyLoan;
    friend class BnwAttritionUnitSelectedRead;
    friend class BnwAttritionSelectedUnitInfoDirtyLoan;
    friend class BnwAttritionSelectionPlotRead;
    friend class BnwAttritionPlotListDirtyLoan;
    friend class BnwAttritionDamageDllWrapperPrefix;
    friend class BnwGameTurnTimerOwner;
    void execute_game_turn_elapsed_with_actual_stock(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void execute_game_turn_activation_prefix_with_actual_stock(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void execute_game_turn_shuffle_with_actual_stock(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void execute_game_turn_player_active_with_actual_stock(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void execute_game_turn_player_dirty_with_actual_stock(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void execute_game_turn_player_end_turn_with_actual_stock(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_game_turn_player_end_turn_return_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::uint64_t,std::uint64_t,std::uint64_t,const BnwSelectedStockLuaSourceOwner&,bool,bool busy_continuation=false,bool city_continuation=false)const;
    std::int32_t player_busy_actor_without_callbacks(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&)const;
    void require_player_busy_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_game_turn_player_busy_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_game_turn_player_busy_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,
     const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool city_continuation=false)const;
    void fail_player_busy_without_callbacks()noexcept;
    void require_player_busy_city_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    std::int32_t player_busy_city_actor_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&)const;
    void execute_game_turn_player_busy_city_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_game_turn_player_busy_city_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,
     const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
    void fail_player_busy_city_without_callbacks()noexcept;
    friend class BnwNativePlayerBusyUnitsReturn;friend class BnwNativePlayerBusyCitiesReturn;
    void require_player_end_after_busy_transition_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
     const BnwSelectedStockLuaSourceOwner&,std::uint64_t,std::int32_t)const;
    void require_player_end_after_busy_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_game_turn_player_end_after_busy_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_player_end_after_busy_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
     const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,std::uint64_t,bool)const;
    void fail_player_end_after_busy_without_callbacks()noexcept;
    std::uint64_t player_end_after_busy_before_without_callbacks()const;
    void require_processed_auto_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_processed_auto_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_processed_auto_transition_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,std::uint64_t,std::int32_t)const;
    void require_processed_auto_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
     const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool)const;
    void fail_processed_auto_without_callbacks()noexcept;
    void require_attrition_first_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_attrition_first_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_attrition_first_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
     const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool)const;
    void require_attrition_first_bound_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
     const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool,bool)const;
    void require_attrition_mountain_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_attrition_mountain_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_attrition_mountain_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
     const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool)const;
    void fail_attrition_mountain_without_callbacks()noexcept;
    std::uint64_t attrition_damage_previous_serial_without_callbacks()const;
    void require_attrition_damage_parent_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
     const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
    void require_attrition_damage_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_attrition_damage_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&,const std::shared_ptr<const ui::LiveUiHost>&);
    void require_attrition_damage_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
     const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool)const;
    void fail_attrition_damage_without_callbacks()noexcept;
    std::uint64_t attrition_garrison_previous_serial_without_callbacks()const;
    void require_attrition_garrison_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_attrition_garrison_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_attrition_garrison_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool)const;
    void fail_attrition_garrison_without_callbacks()noexcept;
    std::uint64_t attrition_city_strength_previous_serial_without_callbacks()const;
    void require_attrition_city_strength_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_attrition_city_strength_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_attrition_city_strength_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t,bool)const;
    void fail_attrition_city_strength_without_callbacks()noexcept;
    void require_attrition_city_info_facade_without_callbacks()const;
    std::uint64_t attrition_city_info_previous_serial_without_callbacks()const;
    void require_attrition_city_info_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_attrition_city_info_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_attrition_city_info_operation_without_callbacks(std::uint64_t,std::uint64_t,bool)const;
    void record_attrition_city_info_return_without_callbacks(const BnwAttritionCityInfoDirtyLoan&,std::uint64_t,std::uint64_t)noexcept;
    void require_attrition_city_info_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void fail_attrition_city_info_without_callbacks()noexcept;
    std::uint64_t attrition_unit_info_previous_serial_without_callbacks()const;
    void require_attrition_unit_info_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_attrition_unit_info_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_attrition_unit_info_parent_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
    void require_attrition_unit_info_operation_without_callbacks(std::uint64_t,std::uint64_t,bool)const;
    void record_attrition_unit_info_return_without_callbacks(const BnwAttritionUnitInfoDirtyLoan&,std::uint64_t,std::uint64_t)noexcept;
    void require_attrition_unit_info_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void fail_attrition_unit_info_without_callbacks()noexcept;
    std::uint64_t attrition_unit_selected_previous_serial_without_callbacks()const;
    void require_attrition_unit_selected_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_attrition_unit_selected_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_attrition_unit_selected_parent_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
    void require_attrition_unit_selected_operation_without_callbacks(std::uint64_t,std::uint64_t,bool)const;
    void record_attrition_unit_selected_return_without_callbacks(const BnwAttritionUnitSelectedRead&,bool)noexcept;
    void require_attrition_unit_selected_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void fail_attrition_unit_selected_without_callbacks()noexcept;
    std::uint64_t attrition_selected_unit_info_previous_serial_without_callbacks()const;
    void require_attrition_selected_unit_info_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_attrition_selected_unit_info_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_attrition_selected_unit_info_parent_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
    void require_attrition_selected_unit_info_operation_without_callbacks(std::uint64_t,std::uint64_t,bool)const;
    void record_attrition_selected_unit_info_return_without_callbacks(const BnwAttritionSelectedUnitInfoDirtyLoan&,std::uint64_t,std::uint64_t)noexcept;
    void require_attrition_selected_unit_info_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void fail_attrition_selected_unit_info_without_callbacks()noexcept;
    std::uint64_t attrition_selection_plot_previous_serial_without_callbacks()const;
    void require_attrition_selection_plot_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_attrition_selection_plot_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_attrition_selection_plot_parent_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
    void require_attrition_selection_plot_operation_without_callbacks(std::uint64_t,std::uint64_t,bool)const;
    void record_attrition_selection_plot_return_without_callbacks(const BnwAttritionSelectionPlotRead&,std::optional<std::int32_t>,std::int32_t)noexcept;
    void require_attrition_selection_plot_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void fail_attrition_selection_plot_without_callbacks()noexcept;
    std::uint64_t attrition_plot_list_previous_serial_without_callbacks()const;
    void require_attrition_plot_list_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_attrition_plot_list_with_actual_stock(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_attrition_plot_list_parent_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
    void require_attrition_plot_list_operation_without_callbacks(std::uint64_t,std::uint64_t,bool)const;
    void record_attrition_plot_list_return_without_callbacks(const BnwAttritionPlotListDirtyLoan&,std::uint64_t,std::uint64_t)noexcept;
    void require_attrition_plot_list_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void fail_attrition_plot_list_without_callbacks()noexcept;
    std::uint64_t attrition_dll_wrapper_previous_serial_without_callbacks()const;
    void require_attrition_dll_wrapper_entry_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void require_attrition_dll_wrapper_parent_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t,std::uint64_t)const;
    void require_attrition_dll_wrapper_operation_without_callbacks(std::uint64_t,std::uint64_t,bool)const;

    void execute_attrition_dll_wrapper_until_popup(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&,const std::shared_ptr<ui::BnwClientSceneFrame>&,const std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>&);
    void record_attrition_dll_wrapper_return_without_callbacks(const BnwAttritionDamageDllWrapperPrefix&,bool)noexcept;
    void require_attrition_dll_wrapper_return_without_callbacks(const BnwNativeUnitAllocatorCore&,const BnwSelectedStockLuaSourceOwner&)const;
    void fail_attrition_dll_wrapper_without_callbacks()const noexcept;
    void fail_attrition_first_without_callbacks()noexcept;
    std::uint64_t attrition_first_before_without_callbacks()const;
    std::uint64_t end_after_busy_serial_without_callbacks()const;
    std::uint64_t processed_auto_serial_without_callbacks()const;
    std::uint64_t attrition_first_serial_without_callbacks()const;
    std::uint64_t processed_auto_before_without_callbacks()const;
    void require_player_end_after_busy_bound_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
     const BnwSelectedStockLuaSourceOwner&,std::uint64_t,std::int32_t,bool)const;
    void require_player_busy_units_parent_for_city_without_callbacks(const BnwNativeUnitAllocatorCore&,const ui::SelectedBnwPreviewHost&,
     const LiveSessionState&,const BnwSelectedStockLuaSourceOwner&,const void*,std::uint64_t)const;

    void resume_game_turn_player_after_dirty_delivery_with_actual_stock(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_player_turn_dirty_operation_without_callbacks(const BnwPlayerTurnActiveDirtyIssuer&,const BnwSelectedStockLuaSourceOwner&,std::uint64_t,bool)const;
    void require_player_turn_dirty_operands_without_callbacks(const BnwPlayerTurnActiveDirtyIssuer&,const ui::BnwEngineUiTurnOwner&,const BnwSelectedStockLuaSourceOwner&)const;
    void require_player_turn_dirty_return_without_callbacks(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&,bool)const;
    void require_player_turn_dirty_outer_return_without_callbacks(const BnwSelectedStockLuaSourceOwner&,bool)const;
    void fail_player_turn_dirty_without_callbacks()noexcept;
    void require_game_turn_player_active_store_return(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::uint64_t,std::uint64_t,std::uint64_t,const BnwSelectedStockLuaSourceOwner&,bool after_adoption=false)const;
    void require_game_turn_shuffle_store_return(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::uint64_t,std::uint64_t,std::uint64_t,const BnwSelectedStockLuaSourceOwner&,bool after_adoption=false)const;
    void require_game_turn_activation_store_return(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::uint64_t,std::uint64_t,std::uint64_t,const BnwSelectedStockLuaSourceOwner&,bool after_adoption=false)const;
    void require_game_turn_elapsed_store_return(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::uint64_t,std::uint64_t,std::uint64_t,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_game_turn_end_timer_with_actual_stock(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_actual_end_turn_timer_reset_source(const BnwGameTurnTimerOwner&,const LiveSession&,const LiveSessionState&,std::uint64_t,std::int64_t)const;
    void require_actual_timer_clock_adoption(const BnwGameTurnTimerOwner&,const LiveSession&,const LiveSessionState&,std::uint64_t,std::int64_t)const;
    friend class ui::BnwEngineUiTurnOwner;
    void execute_engine_ui_end_turn_reset_with_actual_stock(const BnwStockLuaAttemptLease&);
    void execute_engine_ui_end_turn_reset_impl(const BnwStockLuaAttemptLease*);
    std::shared_ptr<BnwRetainedTurnCursor> retain_engine_factory_owner()const;
    void require_engine_host(const ui::SelectedBnwPreviewHost&)const;
    std::uint64_t engine_actual_revision()const;
    std::uint64_t game_tail_actual_revision()const;
    void execute_game_culture_public_opinion_impl(BnwSelectedStockLuaBootstrap*,const BnwStockLuaAttemptLease*);
    void execute_game_culture_public_opinion_with_actual_stock(BnwSelectedStockLuaBootstrap&,const BnwStockLuaAttemptLease&);

    void execute_game_turn_paired_with_actual_stock(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void execute_game_turn_ui_dirty_with_actual_stock(const ui::LiveUiHost&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void execute_game_turn_sent_auto_with_actual_stock(const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void execute_game_turn_gameplay_with_actual_stock(const ui::LiveUiHost&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&);
    void require_game_turn_gameplay_source(const ui::LiveUiHost&,const ui::BnwEngineUiTurnOwner&,const LiveSessionState&,const BnwStockLuaAttemptLease&)const;
    void accept_actual_game_turn_gameplay_return(const ui::BnwEngineUiTurnOwner&,const BnwSelectedStockLuaSourceOwner&,const BnwStockLuaAttemptLease&)const;
    void require_game_turn_gameplay_store_return(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::uint64_t,std::uint64_t,std::uint64_t,const BnwSelectedStockLuaSourceOwner&)const;
    void require_game_turn_sent_auto_store_return(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::uint64_t,std::uint64_t,std::uint64_t,const BnwSelectedStockLuaSourceOwner&)const;
    void require_game_turn_clock_store_return(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
        std::uint64_t,std::int64_t,std::uint64_t,std::uint64_t,const BnwSelectedStockLuaSourceOwner&)const;
    void execute_engine_ui_impl(BnwSelectedStockLuaBootstrap*,const BnwStockLuaAttemptLease*,const BnwSelectedStockLuaSourceOwner*);
    void execute_engine_ui_with_actual_stock(BnwSelectedStockLuaBootstrap&,const BnwStockLuaAttemptLease&,const BnwSelectedStockLuaSourceOwner&);
    void execute_team_war_prefix(bool first_next);
    void execute_team_no_trade_prefix(bool first);
    void execute_team_small_awards_prefix(bool first);
    void execute_team_circumnavigation_prefix(bool first);
    void execute_team_return_and_advance_prefix(bool first);
    struct History;
    struct Impl;
    std::shared_ptr<ui::BnwClientSelectionControl> require_actual_ui(const ui::LiveUiHost&) const;
    explicit BnwRetainedTurnCursor(std::shared_ptr<Impl> impl):impl_(std::move(impl)){}
    std::shared_ptr<Impl> impl_;
};
} // namespace civ5::runtime
