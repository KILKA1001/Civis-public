#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <span>

namespace civ5::runtime {

// Proposed ID35: all decoded BNW PlayerKnowledge scalar, array, and hashed
// row values for the complete 64-player roster. Structure, versions, array
// dimensions and hash IDs remain fixed. This raw diagnostic section conflicts
// with ID33 (which owns player_techs.researching_tech). Until a composed owned
// research/AI contract exists, a normal loader must reject ID35+ID33 and must
// not treat ID35 by itself as an authorized player action or full persistence.
inline constexpr std::uint32_t bnw_player_knowledge_checkpoint_section_id=35;

// The common importer must call this before applying either section. The
// full knowledge section and ID33 cannot share researching_tech ownership.
void validate_bnw_player_knowledge_composition(
    std::span<const saves::CivisSaveSection> sections);

saves::CivisSaveSection make_bnw_player_knowledge_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Decode and validate completely, then return a detached replacement. The
// input world is never mutated on rejection. Other already restored domains
// may coexist only when they preserve PlayerKnowledge schema and source values.
saves::SavedSessionSnapshot restore_bnw_player_knowledge_checkpoint(
    const saves::CivisSaveSection &section,
    const saves::SavedSessionSnapshot &world_before,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
