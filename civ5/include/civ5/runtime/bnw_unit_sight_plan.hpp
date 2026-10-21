#pragma once
#include "civ5/runtime/bnw_unit_sight_preflight.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/content/rules_database.hpp"
#include <memory>

namespace civ5::runtime {

struct BnwSightPlotDelta {
    std::uint64_t plot=0;
    std::int64_t before=0,after_old=0,after_new=0;
    std::uint8_t old_calls=0,new_calls=0;
    // CvPlot.h defaults bBasedOnUnit=true. Ring 0/1 additionally changes
    // all invisible types even when CvUnit::getSeeInvisibleType()==NO_INVISIBLE.
    std::int64_t invisible_before=0,invisible_after_old=0,invisible_after_new=0;
    std::uint8_t invisible_old_calls=0,invisible_new_calls=0;
};

struct BnwSightCall {
    std::uint64_t plot=0;
    std::int64_t ring=0;
};

struct BnwUnitSightPlan {
    bool calculated=false;
    std::string reason;
    std::int64_t team=-1,visibility_range=-1;
    // SDK old-plot decrement then new-plot increment, with CvPlot's zero clamp.
    // These are counter predictions only; no state is changed or published.
    std::vector<BnwSightPlotDelta> plots;
    // Expansion2 CvPlot::changeAdjacentSight ring visitation order for the
    // new center. A zero-to-positive edge invokes setRevealed, exploration,
    // adjacent-owner refresh, then unit and city meet in this sequence.
    std::vector<BnwSightCall> new_call_order;
    std::vector<BnwSightBlocker> blockers;
    bool can_commit() const noexcept {return blockers.empty();}
};

// The database must be the activated rules for the same session and match
// identity.rules_sha256. Uses the exact Expansion2 ring/height algorithm for
// a nonembarked unit with NO_INVISIBLE, including the default based-on-unit
// ring-0/1 invisible counter. Reveal, meetings, AI, Lua and UI are outside
// this read-only counter plan and remain hard blockers.
BnwUnitSightPlan plan_bnw_unit_sight(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime

namespace civ5::runtime {
class BnwNativeUnitAllocatorCore;
// Core-only effect owner, minted from the actual unpublished node/control.
// No constructor, supplied-world setter, completion flag or public commit.
class BnwInitialUnitSightOwner final {
 public: ~BnwInitialUnitSightOwner();
 BnwInitialUnitSightOwner(const BnwInitialUnitSightOwner&)=delete;
 BnwInitialUnitSightOwner&operator=(const BnwInitialUnitSightOwner&)=delete;
 private:
 friend class BnwNativeUnitAllocatorCore;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwInitialUnitSightOwner(std::unique_ptr<Impl>)noexcept;
 static std::shared_ptr<BnwInitialUnitSightOwner> prepare(const BnwNativeUnitAllocatorCore&);
 void require_before_without_callbacks(const BnwNativeUnitAllocatorCore&)const;
 void require_after_without_callbacks(const BnwNativeUnitAllocatorCore&)const;
 void apply_without_callbacks()noexcept;
};
}
