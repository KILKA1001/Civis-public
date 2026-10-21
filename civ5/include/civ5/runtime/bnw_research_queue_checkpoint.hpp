#pragma once
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/research_checkpoint.hpp"
#include "civ5/runtime/research_turn_checkpoint.hpp"

namespace civ5::runtime {

// Proposed raw-world companion for BNW research-choice state. This covers
// PlayerAdministration.pre_city.m_researchQueue and
// PlayerKnowledge.player_techs.researching_tech only. Treasury ID30 and the
// transient culture-bomb timer ID32 remain independent domains.
inline constexpr std::uint32_t bnw_research_queue_checkpoint_section_id=33;

struct BnwResearchQueueCheckpointBundle {
    saves::CivisSaveSection raw,research,research_turn;
};
struct BnwResearchQueueRestored {
    saves::SavedSessionSnapshot world;
    std::vector<TeamTechnologies> technologies;
    PlayerResearchContext players;
    ResearchTurnState turn;
};

saves::CivisSaveSection make_bnw_research_queue_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);
void restore_bnw_research_queue_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// This accepts only queue/flag mutations on otherwise unchanged owned
// research contexts. It prepares ID33+ID8+ID11 together and rejects team
// technology/progress changes. The caller must separately establish that the
// Lua checks and post-commit dirty notifications of the action completed;
// this bundle alone does not authorize an action or a full turn.
BnwResearchQueueCheckpointBundle make_bnw_research_queue_checkpoint_bundle(
    const LiveSessionState &state,
    const LiveSessionState &baseline,
    const saves::SavedSessionSnapshot &source,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,
    const SessionCheckpointIdentity &identity);
BnwResearchQueueRestored restore_bnw_research_queue_checkpoint_bundle(
    const BnwResearchQueueCheckpointBundle &bundle,
    const LiveSessionState &baseline,
    const saves::SavedSessionSnapshot &source,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
