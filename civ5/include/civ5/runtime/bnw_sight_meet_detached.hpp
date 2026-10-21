#pragma once
#include "civ5/runtime/bnw_unit_sight_event_order.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

struct BnwSightMeetTransition {
    std::uint64_t plot=0;
    std::int64_t revealing_team=-1,other_team=-1;
    std::int64_t object_owner=-1,object_id=-1;
    bool city=false,new_contact=false;
};

struct BnwSightMeetResult {
    bool calculated=false;
    std::string reason;
    // A diagnostic continuation of the earlier detached sight state. The
    // original LiveSessionState and source payload are never modified.
    std::optional<LiveSessionState> detached;
    std::vector<BnwSightMeetTransition> contacts;
    // CvTeam::makeHasMet has mandatory effects beyond these two stored fields.
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvPlot::changeVisibilityCount invokes CvTeam::meet for each
// eligible unit in roster order, then the city. CvTeam::meet invokes
// makeHasMet(A,B), makeHasMet(B,A), then TeamMeet Lua. This helper applies only
// the reciprocal has_met and turn_team_met writes to a private copy after
// resolving saved unit/city identity and the selected source/rules hashes.
// updateTechShare, diplomacy, minor reveals, gameplay/UI, Lua, and complete
// persistence are still required before any result may be committed.
BnwSightMeetResult apply_bnw_sight_meets_detached(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
