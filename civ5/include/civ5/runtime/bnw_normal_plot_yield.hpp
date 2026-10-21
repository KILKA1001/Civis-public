#pragma once
#include "civ5/content/rules_database.hpp"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>
namespace civ5::saves {struct SavedSession;}
namespace civ5::runtime {
struct LiveSessionState;struct SessionCheckpointIdentity;
class BnwPlotYieldRefreshIssuer;
class BnwNormalPlotYieldUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
class BnwNormalPlotYieldRules final {
 public:
  static BnwNormalPlotYieldRules load(const content::RulesDatabase&,std::string_view rules_sha);
 private:
  friend class LiveSession;friend class BnwPolicyAdoptionPending;
  friend class BnwDisplayedPlotYield;
  friend class BnwLuaCalculatedPlotYieldOwner;
  friend class BnwPlotYieldRefreshPending;
  friend class BnwCityYieldRefreshPending;
  // Actual immutable DB/thread guard, before any source/world borrow.
  void require_current()const;
  void require_current(std::string_view rules_sha)const;
  std::int32_t calculate_current_normal(const BnwPlotYieldRefreshIssuer&,
   const LiveSessionState&,const saves::SavedSession&,const SessionCheckpointIdentity&,
   std::int32_t plot,std::int32_t yield)const;
  struct Impl;std::shared_ptr<const Impl> impl_;
  explicit BnwNormalPlotYieldRules(std::shared_ptr<const Impl>)noexcept;
  friend std::int32_t calculate_bnw_normal_plot_yield(const LiveSessionState&,
   const saves::SavedSession&,const SessionCheckpointIdentity&,
   const BnwNormalPlotYieldRules&,std::int32_t,std::int32_t);
};
// Actual CvPlot::calculateYield(false), bounded first nonzero branch:
// owned/unowned, ordinary non-NaturalWonder Features (or NO_FEATURE),
// normally visible Resources (or NO_RESOURCE); present/pillaged improvements
// support all six yields. Owned unpillaged improvements include CULTURE via
// SAME native-bounded helper/current registry, six-slot same-type adjacency
// and actual active/unblocked policy source facts.
// Unowned unpillaged improvements reuse all native Tech slots and generic
// Policy Info in source order; per-era and owning caches are skipped.
// Current routes
// use SAME activated Routes/Route_Yields and actual source pillage state.
// Reuses current raw/owned city registry and native-bounded nature helper.
// Includes source-bound resource/religion, sea-resource, city resource-extra
// and strategic trait gates. Visible resources must have the same result
// under either debug mode; hidden/debug-dependent cases explicitly refuse.
// Includes home-city, water/lake, river and feature/terrain extra yields, player city/
// coastal/capital/threshold/golden-age effects. Returns int32 BEFORE the
// updateYield short cast. Unsupported reached branches explicitly refuse.
// Caller holds the genuine synchronous owner-thread/exclusive read scope.
// No cache writes, UI acknowledgements, freshness or execution permission.
std::int32_t calculate_bnw_normal_plot_yield(const LiveSessionState&,
 const saves::SavedSession&,const SessionCheckpointIdentity&,
 const BnwNormalPlotYieldRules&,std::int32_t plot,std::int32_t yield);
}
