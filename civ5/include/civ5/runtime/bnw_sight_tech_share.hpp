#pragma once
#include "civ5/runtime/bnw_sight_meet_detached.hpp"

namespace civ5::runtime {

struct BnwSightTechShareCheck {
    std::int64_t recipient_team=-1;
    std::int64_t best_share=-1;
    std::uint64_t technologies_examined=0;
    std::optional<std::int64_t> first_grant_tech;
    std::int64_t first_grant_contacts=0;
};

struct BnwSightTechShareResult {
    bool calculated=false;
    std::string reason;
    std::optional<LiveSessionState> detached;
    std::vector<BnwSightMeetTransition> contacts;
    std::vector<BnwSightTechShareCheck> checks;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvTeam.cpp:3349-3354,5803-5859. For one newly met team pair,
// evaluate A's updateTechShare after A's flag and B's after B's flag. A grant
// would enter CvTeam::setHasTech and its unported world/Lua effects, so this
// boundary refuses it and discards the partial detached copy. Multiple new
// pairs need interleaved flag writes and are likewise refused. No gameplay
// state is published, even if no technology would be shared.
BnwSightTechShareResult apply_bnw_sight_tech_share_detached(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
