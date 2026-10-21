#pragma once
#include <map>
#include <string>
#include "civ5/runtime/live_session.hpp"
#include <array>
namespace civ5::ui { class SelectedBnwReadOnlyPreview; }
namespace civ5::runtime {
class BnwNativeUnitAllocatorCore;
class BnwMinorCityStrengthUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
struct BnwMinorCityStrengthUpdate {
 std::int32_t owner=-1,city=-1,before=0,after=0,known_technologies=0;
 std::int64_t plot=-1;
 std::uint64_t dirty_occurrence=0;
};
// One actor only, at the source boundary BEFORE free promotions/traits.
// Array order: feature production, unit fortification, unit base heal, worker
// speed, influence spread, extra votes per diplomat. No acquisition receipt.
struct BnwTechFirstPlayerScalarPrefix {
 std::int32_t team=-1,technology=-1,player=-1,change=0;
 std::array<std::int32_t,6> before{},delta{},after{};
};
struct BnwMinorCityStrengthScratchRead {
 std::int32_t strength=0;
 std::vector<BnwCityDirtyEvent> events;
 std::uint64_t next_occurrence=0,revision=0;
 std::vector<std::int64_t> technologies;
 SimulationRandomState rng,map_rng;
 bool operator==(const BnwMinorCityStrengthScratchRead&)const=default;
};
// Actual canonical Host comes ONLY from the private-owned selected preview
// handoff. This factory clones its SAME actual lease; no supplied State/Host,
// mutable accessor, external receipt, or readiness input exists.
// Source and scratch relevant semantic baselines are sealed across preparation
// and stores. refresh changes ONLY the private scratch cache + staged CityInfo.
// Acquisition/TeamSetHasTech/world/resource/Lua/renderer effects remain missing.
class BnwMinorCityStrengthScratch final {
 public:
 static std::shared_ptr<BnwMinorCityStrengthScratch> from_selected_preview(const ui::SelectedBnwReadOnlyPreview&);
 ~BnwMinorCityStrengthScratch();
 BnwMinorCityStrengthScratch(const BnwMinorCityStrengthScratch&)=delete;
 BnwMinorCityStrengthScratch& operator=(const BnwMinorCityStrengthScratch&)=delete;
 BnwMinorCityStrengthScratch(BnwMinorCityStrengthScratch&&)=delete;
 BnwMinorCityStrengthScratch& operator=(BnwMinorCityStrengthScratch&&)=delete;
 BnwMinorCityStrengthUpdate refresh(std::int32_t owner,std::int32_t city);
 // Isolated processTech six-scalar interval for the FIRST ascending matching
 // EVER-alive actor (+1/-1). No second actor is visited: that actor's promotion
 // and trait callbacks are still mandatory. nullopt means no matching actor,
 // no prefix/stores/advancement; it does not fail or finish the technology.
 std::optional<BnwTechFirstPlayerScalarPrefix> first_player_scalar_prefix(std::int32_t team,std::int32_t technology,std::int32_t change);
 // Forensic private values, including after refusal; no readiness authority.
 std::array<std::int32_t,6> inspect_player_scalar_prefix(std::int32_t player)const;
 // Forensic VALUE snapshot of private scratch only, including after refusal.
 // It provides no current source, acquisition or publication permission.
 BnwMinorCityStrengthScratchRead inspect(std::int32_t owner,std::int32_t city)const;
 private:
 friend class BnwMajorTeamHasTechPending;
 friend struct BnwPolicyHappinessMathAccess;
 friend class BnwPolicyAdoptionPending;
 friend class BnwNativeUnitAllocatorCore;
 friend class BnwNativeAttritionCityStrengthRead;
 struct LiveQuote {
  const saves::SavedCityObservation* raw=nullptr;
  std::int32_t owner=-1,city=-1,before=0,after=0,known=0;
  std::int64_t plot=-1;
 };
 // Created only by the actual canonical pending Core after native plot links.
 // Its vector is a private semantic seal, not a serialized/caller readiness DTO.
 struct InitialQuote {
  LiveQuote quote;
  std::shared_ptr<const LiveSessionState> state;
  LiveSession* session=nullptr;bool happiness_current=false;
  std::weak_ptr<const BnwNativeUnitAllocatorCore> core;
  std::vector<std::int64_t> inputs;
  std::vector<BnwCityDirtyEvent> before_events,after_events;
  std::uint64_t generation=0,before_revision=0,after_revision=0,before_occurrence=0;
  SimulationRandomState rng,map_rng;
  bool stored=false;
  // Separate earned descendant: immutable original city receipt remains intact.
  std::vector<std::int64_t> happiness_inputs;
  std::int32_t happiness_value=0;std::uint64_t happiness_revision=0;
  bool happiness_prefix_stored=false,happiness_cache_current=false;
 std::vector<std::int64_t> name_inputs;bool name_prefix_stored=false;
 std::vector<std::int64_t> player_class_inputs;bool player_class_stored=false;
 std::vector<std::int64_t> military_inputs;bool military_stored=false;
 std::vector<std::int64_t> promotion_inputs;bool promotion_stored=false;
 std::vector<std::int64_t> immobile_moves_inputs;bool immobile_moves_stored=false;
 std::vector<std::int64_t> set_moves_inputs;bool set_moves_stored=false;
 std::vector<std::int64_t> cycle_order_inputs;bool cycle_order_stored=false;
 std::vector<std::int64_t>army_ai_inputs;bool army_ai_stored=false;
 std::vector<std::int64_t> exotic_xp_inputs;bool exotic_xp_stored=false;
 std::vector<std::int64_t> embark_promotion_inputs;bool embark_promotion_stored=false;
 std::vector<std::int64_t> player_promotion_inputs;bool player_promotion_stored=false;
 std::vector<std::int64_t> traits_promotion_inputs;bool traits_promotion_stored=false;
 std::vector<std::int64_t> promotion_loop_inputs;bool promotion_loop_stored=false;
 };
 static std::vector<std::int64_t> prepare_initial_happiness_inputs(const BnwNativeUnitAllocatorCore&,const InitialQuote&,std::int32_t,std::optional<std::int32_t> connection={},std::optional<std::int32_t> excess={});
 static std::vector<std::int64_t> prepare_initial_military_inputs(const BnwNativeUnitAllocatorCore&,const InitialQuote&,const saves::PlayerCoreField&,const std::vector<std::int64_t>&);
 static std::vector<std::int64_t> prepare_initial_player_class_inputs(const BnwNativeUnitAllocatorCore&,const InitialQuote&,const saves::PlayerCoreField&,const std::vector<std::int64_t>&);
 static std::vector<std::int64_t> prepare_initial_name_inputs(const BnwNativeUnitAllocatorCore&,const InitialQuote&,
  const std::map<std::string,std::int32_t,std::less<>>&,const std::map<std::string,std::vector<std::int32_t>,std::less<>>&);
 struct InitialSuffixQuote {
  LiveQuote quote;std::weak_ptr<const BnwNativeUnitAllocatorCore>core;
  std::shared_ptr<const LiveSessionState>state;std::uint64_t generation=0,before_revision=0;
  std::vector<std::int64_t>inputs,army_ai_inputs;bool army_ai_stored=false;SimulationRandomState rng,map_rng;
 };
 static InitialSuffixQuote prepare_initial_suffix_strength(const BnwNativeUnitAllocatorCore&);
 static std::vector<std::int64_t>prepare_initial_suffix_node_inputs(const BnwNativeUnitAllocatorCore&,const InitialSuffixQuote&,const std::map<std::string,std::int32_t,std::less<>>&,const std::map<std::string,std::vector<std::int32_t>,std::less<>>&);
 static void require_initial_suffix_strength_without_callbacks(const BnwNativeUnitAllocatorCore&,const InitialSuffixQuote&,bool);
 static InitialQuote prepare_initial_pending(const BnwNativeUnitAllocatorCore&);
 static bool has_initial_pending_garrison(const BnwNativeUnitAllocatorCore&,std::int32_t city);
 struct InitialCenterEntry {std::int32_t owner=-1,asked_id=-1,id=-1,type=-1;const saves::SavedUnitObservation*raw=nullptr;bool pending=false,resolved=false;};
 struct InitialCenterSelection {std::vector<InitialCenterEntry> entries;std::optional<std::size_t> selected,best,head;};
 static InitialCenterSelection prepare_initial_center_selection(const BnwNativeUnitAllocatorCore&,std::optional<std::pair<std::int32_t,std::int64_t>> selected);
 static std::optional<std::int32_t> selected_initial_garrison_unit_id(const BnwNativeUnitAllocatorCore&,std::int32_t city);
 struct InitialSetupGarrisonQuote {std::shared_ptr<const LiveSessionState>state;std::int32_t owner=-1,city=-1;std::uint64_t generation=0,revision=0;std::optional<std::int32_t>selected;std::vector<std::int64_t>inputs;SimulationRandomState rng,map_rng;};
 static InitialSetupGarrisonQuote prepare_initial_setup_garrison(const BnwNativeUnitAllocatorCore&,std::int32_t city);
 static void require_initial_setup_garrison_without_callbacks(const BnwNativeUnitAllocatorCore&,const InitialSetupGarrisonQuote&);
 static void require_initial_pending_without_callbacks(const BnwNativeUnitAllocatorCore&,const InitialQuote&,bool after);
 struct LiveStored {std::int32_t strength=0,known_technologies=0;std::uint64_t generation=0;};
 static bool has_same_live_policy_garrison(const LiveSessionState&,const ui::SelectedBnwPreviewHost&,std::int32_t,std::int32_t);
 static LiveQuote quote_same_live_policy(const LiveSessionState&,const ui::SelectedBnwPreviewHost&,
     std::int32_t owner,std::int32_t city,std::int32_t prepared_modifier);
 static LiveQuote quote_same_live(const LiveSessionState&,const ui::SelectedBnwPreviewHost&,
     std::int32_t owner,std::int32_t city);
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwMinorCityStrengthScratch(std::shared_ptr<Impl>);
};
}
