#pragma once
#include "civ5/runtime/bnw_sight_meet_detached.hpp"
#include <array>

namespace civ5::runtime {

// Reserved common CivisSaveSectionId::live_bnw_team_meet. The common enum,
// importer, and LiveSession restore are integrated separately.
inline constexpr std::uint32_t bnw_team_meet_checkpoint_section_id=21;
struct BnwTeamMeetCompanions {
    static constexpr std::array<std::uint32_t,4> existing_sections{
        17,18,19,20}; // map, unit, area accounting, player proximity
    static constexpr bool first_contact_ai_available=false;
    static constexpr bool team_meet_lua_available=false;
};

// Expansion2 CvTeam::meet calls makeHasMet(A,B), then makeHasMet(B,A),
// then TeamMeet Lua. Version 1 persists only the two has_met flags and two
// turn_team_met entries for one new contact, in that source order.
saves::CivisSaveSection make_bnw_team_meet_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const BnwSightMeetTransition &contact);

// Checks source/rules identity, 64-slot team/player roster, complete team
// shape, and both baseline relationships before changing any field. This
// isolated restore does not mark the contact or a unit move publishable.
void restore_bnw_team_meet_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
