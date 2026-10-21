#pragma once
#include "civ5/runtime/bnw_sight_diplomacy_preflight.hpp"
#include <span>

namespace civ5::runtime {

enum class BnwMeetEffectKind : std::uint8_t {
    has_met,
    turn_team_met,
    update_tech_share,
    proximity,
    first_contact_ai,
    third_party_ai,
    clear_research_queue,
    score_ui_dirty,
    gameplay_met_team,
    lua_team_meet,
};
struct BnwMeetEffectEvent {
    BnwMeetEffectKind kind=BnwMeetEffectKind::has_met;
    std::int64_t team=-1,other_team=-1;
    std::int64_t player=-1,other_player=-1,third_player=-1;
    // A capability check is only a preflight. This plan never invokes the
    // callback or grants permission to publish the detached world.
    bool external=false;
};
struct BnwSightMeetEffectPlan {
    bool calculated=false;
    std::string reason;
    std::vector<BnwMeetEffectEvent> events;
    std::vector<BnwSightMeetTransition> contacts;
    bool can_commit() const noexcept {return false;}
};
struct BnwMeetEffectCapabilityCheck {
    bool complete=false;
    std::string reason;
    std::vector<std::size_t> missing_event_indices;
    bool can_commit() const noexcept {return false;}
};

// Source-order trace for one no-grant, non-minor CvTeam::meet. Each side runs
// makeHasMet fully before the other; TeamMeet Lua follows both. Unavailable
// minor contact, always-war, tech grant, or player roster fails closed.
BnwSightMeetEffectPlan plan_bnw_sight_meet_effects(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);

// Checks that every external event has a declared owner. Even a complete
// capability list is not an execution receipt or a setXY commit permit.
BnwMeetEffectCapabilityCheck check_bnw_meet_effect_capabilities(
    const BnwSightMeetEffectPlan &plan,
    std::span<const BnwMeetEffectKind> available);
} // namespace civ5::runtime
