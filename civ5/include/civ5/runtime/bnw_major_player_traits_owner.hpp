#pragma once
#include "civ5/saves/player_traits.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include "civ5/runtime/bnw_unhappiness.hpp"
#include <stdexcept>
#include <string_view>
#include <vector>
namespace civ5::ui {class SelectedBnwReadOnlyPreview;class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class LiveSession;
class BnwPolicyAdoptionPending;
class BnwMajorPlayerTraitsUnavailable:public std::runtime_error{public:using std::runtime_error::runtime_error;};
struct BnwTraitFreeResourceXCities {
 std::int32_t resource=0,num_cities=0,quantity=0;
 bool operator==(const BnwTraitFreeResourceXCities&)const=default;
};
enum class BnwTraitCombatImprovementProvenance { native_info_value, imported_opaque_cache, canonical_unobserved_cache };
struct BnwMajorPlayerTraitsSnapshot {
 std::int32_t player=-1;std::int64_t leader=-1,team=-1;
 std::vector<std::int64_t> active_traits;
 std::vector<saves::PlayerTraitsField> cache;
 std::vector<BnwTraitFreeResourceXCities> free_resource_x_cities;
 // Native TraitEntry NULL field has no portable constructor seed in SDK.
 // An imported cache value is preserved, never promoted to an Info/rule seed.
 BnwTraitCombatImprovementProvenance combat_improvement_provenance=BnwTraitCombatImprovementProvenance::native_info_value;
};
struct BnwMajorPlayerTraitsResetEffect {
 std::int32_t player=-1,wounded_before=0,wounded_after=0;
 std::int64_t technology=-1;std::int32_t change=0;
 // This is an isolated Reset/Init effect, NOT a processTech predecessor receipt.
 // Existing persistent BNW trait mirrors are updated with the cache.
 // Policy cost, free units, actual plot/city yield updates and Lua remain required.
};
struct BnwMajorPolicyFreeUnitEffect {
 std::int32_t player=-1,before=0,after=0,policy=0,building=0,minor=0,traits=0,floor=0;
 std::int64_t technology=-1,first_free_unit_class=-1;
 std::uint64_t free_unit_index=0;
 bool operator==(const BnwMajorPolicyFreeUnitEffect&)const=default;
};
// Genuine selected-source known native Reset/Init domains plus exact imported
// opaque cache provenance for the uninitialized NULL TraitEntry enum.
// Known derived fields agree with reconstruction; opaque enum agrees with the
// authentic imported cache. A zero-effect native/Lua proof may canonicalize only
// the unobserved enum during Init; it does not identify its original seed. Histories are
// retained, not reset as if this imported player were newly created.
// Isolated native cache writes are supported; no technology acquisition,
// full processTech/cursor advancement or checkpoint grant.
class BnwMajorPlayerTraitsOwner final {
 public:
  ~BnwMajorPlayerTraitsOwner();
  BnwMajorPlayerTraitsOwner(const BnwMajorPlayerTraitsOwner&)=delete;
  BnwMajorPlayerTraitsOwner& operator=(const BnwMajorPlayerTraitsOwner&)=delete;
  static std::shared_ptr<const BnwMajorPlayerTraitsOwner> from_selected_preview(
      const ui::SelectedBnwReadOnlyPreview&,std::int32_t player);
  void require_current()const;
  // Owner must have been issued against the genuine pre-transition cache.
  // Requires the exact paired raw/owned single flag poststate; never sets HasTech.
  BnwMajorPlayerTraitsResetEffect reset_init_after_technology_flag_transition(
      std::int64_t technology,std::int32_t change)const;
  // CvPlayer::GetExcessHappiness: current cached m_iHappiness minus fresh
  // native GetUnhappiness. Does NOT execute DoUpdateHappiness or its religion
  // Lua/positive-cache graph, acquire technology or grant full turn readiness.
  // All players are prepared before SAMEstate excess/current-guard stores.
  std::map<std::int64_t,BnwUnhappinessQuote> refresh_cached_happiness_getter_projection()const;
  std::int32_t cache_value(std::string_view field,std::int64_t index=0)const;
  BnwMajorPlayerTraitsSnapshot snapshot()const;
  std::optional<BnwMajorPolicyFreeUnitEffect> policy_free_unit_effect()const;
 private:
  friend class BnwMajorTeamHasTechPending;
  friend class BnwNativeUnitAllocatorCore;
  friend class BnwPolicyAdoptionPending;
  struct PolicyCostObserverRetirement;
  std::shared_ptr<const PolicyCostObserverRetirement> prepare_policy_cost_observer_retirement(
      const std::shared_ptr<const BnwPolicyAdoptionPending>&,
      std::int64_t before_cost,std::int64_t after_cost,
      std::uint64_t before_revision,std::uint64_t after_revision)const;
  static void accept_policy_cost_store(const std::shared_ptr<const PolicyCostObserverRetirement>&);

  // Issued only by the genuine core before its paired HasTech stores.
  static std::shared_ptr<const BnwMajorPlayerTraitsOwner> from_source_host(
      std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int32_t player);
  // Private predecessor is the actual core, never public revision adoption.
  BnwMajorPlayerTraitsResetEffect reset_init_for_pending(
      const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,
      std::uint64_t generation,std::uint64_t before_revision,
      std::uint64_t after_revision,std::int64_t technology)const;
  // Actual first-member SDK policy scalar and GetFirstFreeUnit iterator.
  BnwMajorPolicyFreeUnitEffect recompute_policy_and_first_free_unit(
      const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,std::int64_t technology)const;
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwMajorPlayerTraitsOwner(std::shared_ptr<Impl>);
};
}
