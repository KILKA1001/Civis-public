#pragma once
#include "civ5/runtime/bnw_city_strategy_runtime.hpp"
#include "civ5/runtime/bnw_city_dirty_event.hpp"
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <tuple>
#include <utility>
#include <optional>
#include <string_view>
namespace civ5::content { class RulesDatabase; }
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class LiveSession;
struct MapGeometry;
class BnwCityEntryFeatureRules final {
 public:
 // Actual Features/Builds cache replay, retaining the SAME immutable DB Impl
 // through its closed currentness guard. No borrowed facade/caller callback.
 static BnwCityEntryFeatureRules from_rules(const content::RulesDatabase&);
 void require_current()const;
 // Factual derived cache getter only; never a source-stage permission.
 bool feature_clearable(std::int64_t id)const;
 private:
 friend class BnwCityEntryPending;
 struct Impl;std::shared_ptr<const Impl> impl_;
 explicit BnwCityEntryFeatureRules(std::shared_ptr<const Impl>)noexcept;
};
// Actual current native clock, owner, canonical full city ID and core span.
// The key is a factual component identity, never a player-phase receipt.
using BnwCityEntrySourceKey=std::tuple<std::int32_t,std::int32_t,std::int32_t,
                                      std::uint64_t,std::uint64_t>;
enum class BnwCityCitizensVerifyFrontier { not_started, none, inner_city_screen_predicate, remaining_terrain, outer_city_selected, reassignment, specialist_scoring, default_specialist_process, focus_and_specialists, specialist_process, specialist_update_religion, puppet_next_setting, nonhuman_focus_specialization, reallocation_forced_plot, reallocation_remove_working, reallocation_building_scan };
enum class BnwCityEntryPhase { prepared, feature_surrounded_pending, citystrategy_pending, quarantined };
enum class BnwCityStrategyLoopFrontier { none, gameplay_lua, missing_trigger,
 strategy_effect_publication, native_null_strategy, source_inputs_unavailable,
 loop_end_before_remaining_city_ai };
struct BnwCityEntryProgress {
 std::int32_t turn=0,owner=-1,city=-1,slot=-1;
 std::array<bool,3> before{},after{};
 std::uint8_t native_calls=0,raw_assignments=0;
 std::uint8_t feature_positions=0,feature_null_positions=0,feature_clearable_positions=0;
 bool feature_before=false,feature_after=false,feature_assigned=false;
 // Owned ordered source facts only; NO AI or Lua/publication readiness.
 bool strategy_started=false;
 std::int32_t strategy_next_id=0,strategy_slots=0;
 std::uint32_t strategy_steps=0,strategy_trigger_calls=0;
 std::uint64_t strategy_revision=0,strategy_effect_attempts=0,strategy_publications=0;
 BnwCityStrategyLoopFrontier strategy_frontier=BnwCityStrategyLoopFrontier::none;
 bool citizens_started=false;
 std::uint8_t citizens_next_plot=0,citizens_working_slot=0;
 std::uint64_t citizens_native_writes=0;
 std::uint64_t citizens_attempts=0;
 BnwCityCitizensVerifyFrontier citizens_frontier=BnwCityCitizensVerifyFrontier::not_started;
 BnwCityEntryPhase phase=BnwCityEntryPhase::prepared;
};
namespace detail {
// Supplied arithmetic only; never a session or a Citizens completion receipt.
struct BnwCitizensUnworkPrefix {
 std::int32_t working=0,unassigned=0;
 std::optional<std::pair<std::uint8_t,std::int32_t>> first_terrain;
};
// Pure ordered notification values; not a supplied selection/acknowledgement grant.
std::vector<BnwCityDirtyKind> bnw_citizens_outer_dirty_kinds(bool selected);
BnwCitizensUnworkPrefix bnw_citizens_work_prefix(bool home,bool assigning,
 std::int32_t working,std::int32_t unassigned,
 const std::array<std::int16_t,6>& cached,
 const std::array<std::int32_t,6>& terrain);
BnwCitizensUnworkPrefix bnw_citizens_unwork_prefix(bool home,
 std::int32_t working,std::int32_t unassigned,
 const std::array<std::int16_t,6>& cached,
 const std::array<std::int32_t,6>& terrain);
// Pure native scalar boundary, shared by the genuine source capture and
// boundary tests. No state, issuer, freshness or mutable permission follows.
void require_bnw_city_entry_damage_zero_native(std::int64_t);
// Native reached subtraction only; supplied arithmetic never issues a cursor.
std::int32_t bnw_citizens_forced_demotions(std::int64_t forced,std::int64_t working);
// Pure SDK plotXYWithRangeCheck(range2) positions in dx-major wire order.
// Nulls remain slots; wrapped duplicates retain their full multiplicity.
std::array<std::optional<std::int64_t>,25> bnw_city_feature_positions(
 const MapGeometry&,std::int64_t x,std::int64_t y);
bool bnw_city_feature_majority(const std::array<bool,25>&)noexcept;
}
class BnwCityYieldRefreshPending;class BnwPlotYieldRefreshPending;
class BnwCityEntryCheckpoint;
class BnwCityEntryPending final {
 public:
 ~BnwCityEntryPending();
 BnwCityEntryPending(const BnwCityEntryPending&)=delete;
 BnwCityEntryPending& operator=(const BnwCityEntryPending&)=delete;
 // Owned facts remain observable after retirement; no borrowed world or grant.
 BnwCityEntryProgress progress()const;
 void require_current()const;
 // Actual registered component read. Copied numbers never authorize AI/effects.
 BnwCityStrategyRuntimeValues strategy_runtime_values()const;
 static constexpr bool full_city_turn_ready()noexcept{return false;}
 static constexpr bool growth_ready()noexcept{return false;}
 static constexpr bool scheduler_predecessors_ready()noexcept{return false;}
 private:
 friend class LiveSession;friend class BnwCityYieldRefreshPending;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwCityEntryPending(std::unique_ptr<Impl>)noexcept;
 static std::shared_ptr<BnwCityEntryPending> prepare(const LiveSession&,
  std::int64_t owner,std::int64_t city);
 BnwCityEntrySourceKey source_key()const;
 // The session publishes the once slot BEFORE these native setter calls.
 void advance_damage_zero_prefix();
 void advance_feature_surrounded(const BnwCityEntryFeatureRules&);
 void advance_strategy_one(std::shared_ptr<const ui::SelectedBnwPreviewHost>);
 void advance_citizens_reassignment_one(std::shared_ptr<const ui::SelectedBnwPreviewHost>);
 std::int32_t citizens_plot_value(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::uint64_t,bool);
 std::optional<std::uint64_t> citizens_worst_plot(std::shared_ptr<const ui::SelectedBnwPreviewHost>,bool);
 void advance_citizens_do_turn_one(std::shared_ptr<const ui::SelectedBnwPreviewHost>);
 void begin_citizens_specialist_removal(std::shared_ptr<const ui::SelectedBnwPreviewHost>,
  std::int32_t building,std::int32_t specialist,bool forced,bool forced_default);
 std::optional<std::pair<std::int32_t,std::int32_t>> citizens_worst_specialist(
  std::shared_ptr<const ui::SelectedBnwPreviewHost>);
 void begin_citizens_specialist(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int32_t building,std::int32_t specialist);
 void advance_citizens_specialist_one(std::shared_ptr<const ui::SelectedBnwPreviewHost>);
 void set_citizens_working_plot(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::uint64_t,bool);
 void advance_citizens_ui_one(std::shared_ptr<const ui::SelectedBnwPreviewHost>);
 void advance_citizens_verify_one(LiveSession&,std::shared_ptr<const ui::SelectedBnwPreviewHost>);
 void advance_citizens_update_religion(LiveSession&,std::shared_ptr<const ui::SelectedBnwPreviewHost>);
 void advance_citizens_player_happiness(LiveSession&,std::shared_ptr<const ui::SelectedBnwPreviewHost>);
 void accept_city_plot_child_prefix(const BnwCityYieldRefreshPending&,const BnwPlotYieldRefreshPending&);
 void invalidate()noexcept;
 std::shared_ptr<const BnwCityEntryCheckpoint> checkpoint()const;
 std::shared_ptr<const BnwCityEntryCheckpoint> checkpoint_history()const;
 static std::shared_ptr<BnwCityEntryPending> restore_quarantined(
  const BnwCityEntryCheckpoint&);
 void commit_quarantine()noexcept;
};
class BnwCityEntryCheckpoint final {
 private:
 friend class LiveSession;friend class BnwCityEntryPending;
 struct Impl;std::shared_ptr<const Impl> impl_;
 explicit BnwCityEntryCheckpoint(std::shared_ptr<const Impl>)noexcept;
};
}
