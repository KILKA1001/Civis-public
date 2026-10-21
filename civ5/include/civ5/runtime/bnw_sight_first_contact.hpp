#pragma once
#include "civ5/runtime/bnw_sight_diplomacy_preflight.hpp"

namespace civ5::runtime {

enum class BnwSightFirstContactBlocker : std::uint8_t {
    none,
    city_roster_unavailable,
    city_distance_unimplemented,
    first_contact_ai_unimplemented,
    player_core_checkpoint_unimplemented,
    earlier_preflight_rejected,
};
struct BnwSightProximityChange {
    std::int64_t player=-1,other=-1,old_value=-1,new_value=-1;
};
struct BnwSightFirstContactResult {
    bool calculated=false;
    std::string reason;
    std::optional<LiveSessionState> detached;
    std::vector<BnwSightProximityChange> proximity_changes;
    BnwSightFirstContactBlocker blocker=
        BnwSightFirstContactBlocker::earlier_preflight_rejected;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvTeam.cpp:3400-3406 calls both CvPlayer proximity updates
// before CvDiplomacyAI::DoFirstContact. Only the zero city-pair branch of
// CvPlayer.cpp:17069-17200 is applied to a detached raw player field here.
// Its owner city roster is checked against the owned city context. The first
// AI evaluation and the player-core checkpoint are not available, so this
// diagnostic never publishes a contact or a unit move.
BnwSightFirstContactResult apply_bnw_sight_first_contact_detached(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
