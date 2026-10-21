#pragma once
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_normal_plot_yield.hpp"
#include <array>
#include <atomic>
#include <memory>
namespace civ5::ui { class BnwPlotYieldRefreshUiAdapter; }
namespace civ5::runtime {
struct BnwPlotTerrainCheckpointPair;
class BnwPlotTerrainRulesSnapshot;class BnwPolicyAdoptionPending;
class BnwCityYieldRefreshPending;class BnwCityFoodGrowthPending;class BnwCityPopulationPending;
enum class BnwPlotYieldRefreshPhase { prepared, city_screen_predicate, city_screen_dirty, symbols, complete, skipped_no_area, terminal_failure };
struct BnwPlotYieldRefreshProgress {
 std::int64_t plot=-1;
 std::uint64_t operation=0;
 std::uint8_t next_yield=0,plot_writes=0,terrain_writes=0;
 BnwPlotYieldRefreshPhase phase=BnwPlotYieldRefreshPhase::prepared;
 bool effect_started=false;
 bool fresh_six_yields=false;
};
class BnwPlotYieldRefreshPending;
class BnwPlotYieldRefreshCheckpoint;
// Created only by the session. Its lifetime/current allocation is independent
// of the mutable save; no public issuer, epoch reset or source truth setter.
class BnwPlotYieldRefreshIssuer final {
 public: ~BnwPlotYieldRefreshIssuer();
 BnwPlotYieldRefreshIssuer(const BnwPlotYieldRefreshIssuer&)=delete;
 BnwPlotYieldRefreshIssuer& operator=(const BnwPlotYieldRefreshIssuer&)=delete;
 private:
 friend class LiveSession;friend class BnwCityFoodGrowthPending;friend class BnwPlotYieldRefreshPending;friend class BnwCityYieldRefreshPending;
 friend class BnwNormalPlotYieldRules;friend class BnwPolicyAdoptionPending;
 friend class BnwLuaCalculatedPlotYieldOwner;
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwPlotYieldRefreshIssuer(const std::shared_ptr<LiveSessionState>&,std::uint64_t);
 void retire()noexcept;
 // Existing opaque issuer Impl is the separately revocable read control.
 // Only the real session can attach actual field storage/source, not a flag.
 void bind_game_debug(const std::shared_ptr<LiveSession::BnwGameDebugReadStorage>&,
                      const std::shared_ptr<const saves::SavedSession>&);
 bool require_game_debug(const LiveSessionState&,const saves::SavedSession&,
                         const SessionCheckpointIdentity&)const;
};
class BnwPlotYieldRefreshPending final {
 public:
 ~BnwPlotYieldRefreshPending();
 BnwPlotYieldRefreshPending(const BnwPlotYieldRefreshPending&)=delete;
 BnwPlotYieldRefreshPending& operator=(const BnwPlotYieldRefreshPending&)=delete;
 BnwPlotYieldRefreshProgress progress()const;
 // Refuses missing/stale issuer before reading the retained state allocation.
 void require_current()const;
 static constexpr bool full_city_refresh_ready()noexcept{return false;}
 private:
 friend class LiveSession;friend class BnwPolicyAdoptionPending;friend class BnwCityFoodGrowthPending;friend class civ5::ui::BnwPlotYieldRefreshUiAdapter;friend class BnwCityYieldRefreshPending;friend class BnwCityEntryPending;friend class BnwCityYieldRefreshRegistry;
 friend BnwPlotTerrainCheckpointPair make_bnw_plot_terrain_checkpoint_pair(
  const LiveSession&,const SessionCheckpointIdentity&,const BnwPlotTerrainRulesSnapshot&);
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwPlotYieldRefreshPending(std::unique_ptr<Impl>)noexcept;
 static std::shared_ptr<BnwPlotYieldRefreshPending> prepare(
  const std::shared_ptr<BnwPlotYieldRefreshIssuer>&,
  const std::shared_ptr<LiveSessionState>&,std::int64_t);
 static std::shared_ptr<BnwPlotYieldRefreshPending> prepare_normal(
  const std::shared_ptr<BnwPlotYieldRefreshIssuer>&,
  const std::shared_ptr<LiveSessionState>&,
  const std::shared_ptr<const saves::SavedSession>&,
  const SessionCheckpointIdentity&,const BnwNormalPlotYieldRules&,std::int64_t);
 static std::shared_ptr<BnwPlotYieldRefreshPending> prepare_normal_policy(
  const std::shared_ptr<BnwPlotYieldRefreshIssuer>&,const std::shared_ptr<LiveSessionState>&,
  const std::shared_ptr<const saves::SavedSession>&,const SessionCheckpointIdentity&,
  const BnwNormalPlotYieldRules&,std::int64_t,const BnwPolicyAdoptionPending&);
 static std::shared_ptr<BnwPlotYieldRefreshPending> prepare_normal_child(
  const std::shared_ptr<BnwPlotYieldRefreshIssuer>&,const std::shared_ptr<LiveSessionState>&,
  const std::shared_ptr<const saves::SavedSession>&,const SessionCheckpointIdentity&,
  const BnwNormalPlotYieldRules&,BnwCityYieldRefreshPending&);
 static std::shared_ptr<BnwPlotYieldRefreshPending> prepare_normal_population(
  const std::shared_ptr<BnwPlotYieldRefreshIssuer>&,const std::shared_ptr<LiveSessionState>&,
  const std::shared_ptr<const saves::SavedSession>&,const SessionCheckpointIdentity&,
  const BnwNormalPlotYieldRules&,std::int64_t,const BnwCityPopulationPending&);
 static std::shared_ptr<BnwPlotYieldRefreshPending> prepare_normal_bound(
  const std::shared_ptr<BnwPlotYieldRefreshIssuer>&,const std::shared_ptr<LiveSessionState>&,
  const std::shared_ptr<const saves::SavedSession>&,const SessionCheckpointIdentity&,
  const BnwNormalPlotYieldRules&,std::int64_t,const BnwCityYieldRefreshPending*,const BnwCityPopulationPending* = nullptr,const BnwPolicyAdoptionPending* = nullptr);
 std::pair<std::int64_t,std::uint64_t> issued_identity()const;
 void require_policy_parent_source(const BnwPolicyAdoptionPending&)const;
 struct TerrainPrefixSource {
  std::shared_ptr<const LiveSessionState> state;
  std::shared_ptr<const saves::SavedSession> source;
  SessionCheckpointIdentity identity;
  std::uint64_t generation=0,operation=0,ledger_revision=0;
  std::optional<std::pair<std::int32_t,std::int32_t>> city;
  std::array<std::int64_t,6> initial_terrain{},current_terrain{};
  std::map<std::int64_t,CityYieldComponents> initial_owned,current_owned;
  std::uint8_t written_yields=0,terrain_write_count=0,next_yield=0;
 };
 TerrainPrefixSource bound_terrain_prefix_source()const;
 struct PopulationPlotPrefixSource {
  TerrainPrefixSource terrain;
  std::array<std::int64_t,6> initial_cached{},current_cached{};
  BnwPlotYieldRefreshProgress progress;
 };
 PopulationPlotPrefixSource population_prefix_source(const BnwCityPopulationPending&)const;
 PopulationPlotPrefixSource population_city_prefix_source(const BnwCityYieldRefreshPending&,const BnwCityPopulationPending&)const;
 TerrainPrefixSource terrain_prefix_source(const BnwCityYieldRefreshPending&,
  const BnwCityEntryPending&)const;
 std::shared_ptr<const BnwPlotYieldRefreshCheckpoint> checkpoint_history()const;
 static std::shared_ptr<const BnwPlotYieldRefreshCheckpoint> quarantined_history(
  const BnwPlotYieldRefreshCheckpoint&);
 // Root stores the issued object in its once slot BEFORE this mutation.
 void advance_prefix();
 static void prepare_insertions(Impl&);
 static std::array<std::int16_t,6> normal_targets(const BnwPlotYieldRefreshIssuer&,
  const LiveSessionState&,const saves::SavedSession&,const SessionCheckpointIdentity&,
  const BnwNormalPlotYieldRules&,std::int64_t);
 // No caller can construct/consume this private borrowed allocation seam.
 // The actual adapter verifies the session's registered pointer as well.
 struct UiTailSource {
  std::shared_ptr<LiveSessionState> state;
  std::uint64_t generation=0,operation=0;
  std::int64_t plot=-1;
  std::array<std::uint8_t,16> map_guid{};
  std::uint64_t x=0,y=0;
  std::array<std::int16_t,6> yields{};
  std::optional<std::pair<std::int32_t,std::int32_t>> city;
  std::uint8_t next_yield=0;
  BnwPlotYieldRefreshPhase phase=BnwPlotYieldRefreshPhase::prepared;
 };
 UiTailSource ui_tail_source()const;
 // Only actual predicate/event publication uses these private transitions.
 void begin_required_effect();
 void complete_city_screen_predicate(bool);
 void complete_required_effect();
 void fail_required_effect()noexcept;
 void invalidate()noexcept;
 // Root calls only after all replacement/cursor/map allocations succeeded.
 void commit_quarantine()noexcept;
 std::shared_ptr<const BnwPlotYieldRefreshCheckpoint> checkpoint()const;
 bool has_irreversible_history()const;
 // Existing file pair is scoped to real early-zero history, including every
 // once slot whose output happens to be unchanged. No public mode permission.
 void require_early_zero_checkpoint_mode()const;
 // Metadata/actual rules only; Root calls before copying checkpoint's world.
 // Retired/quarantined history does not invoke old DB/source callbacks.
 static bool preflight_restore_rules(const BnwPlotYieldRefreshCheckpoint&,
  const std::optional<LiveSession::BnwGameDebugReadState>&);
 static std::shared_ptr<BnwPlotYieldRefreshPending> quarantine_for_new_allocation(
  const std::shared_ptr<BnwPlotYieldRefreshIssuer>&,
  const std::shared_ptr<LiveSessionState>&,const BnwPlotYieldRefreshPending&);
 static std::shared_ptr<BnwPlotYieldRefreshPending> restore_checkpoint(
  const std::shared_ptr<BnwPlotYieldRefreshIssuer>&,
  const std::shared_ptr<LiveSessionState>&,
  const BnwPlotYieldRefreshCheckpoint&);
};
class BnwPlotYieldRefreshCheckpoint final {
 private:
 friend class LiveSession;friend class BnwPlotYieldRefreshPending;friend class BnwCityYieldRefreshPending;
 struct Impl;std::shared_ptr<const Impl> impl_;
 explicit BnwPlotYieldRefreshCheckpoint(std::shared_ptr<const Impl>)noexcept;
};
}
