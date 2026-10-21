#pragma once
#include "civ5/runtime/bnw_sight_first_contact.hpp"
#include <array>

namespace civ5::runtime {

// Reserved common CivisSaveSectionId::live_bnw_player_proximity (integration
// owns the enum and importer). This section alone is never a contact bundle.
inline constexpr std::uint32_t bnw_player_proximity_checkpoint_section_id=20;
struct BnwPlayerProximityCompanions {
    // A setXY contact changes these existing checkpoint families as well.
    static constexpr std::array<std::uint32_t,3> existing_sections{17,18,19};
    // Team has_met/turn_team_met and first-contact DiplomacyAI still have no
    // complete checkpoint/handler; the section cannot enable publication.
    static constexpr bool team_meet_available=false;
    static constexpr bool first_contact_ai_available=false;
};

// Version 1 records exactly the two reciprocal SDK proximity calls for one
// player pair. All other player-core fields and proximity slots must match
// the source. This is an isolated detached diagnostic, not full persistence.
saves::CivisSaveSection make_bnw_player_proximity_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const std::vector<BnwSightProximityChange> &changes);

// Validates identity, source roster, both old values and complete section
// before touching either slot. A failure leaves world unchanged. Callers must
// restore all companion sections and load hooks before any gameplay use.
void restore_bnw_player_proximity_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
