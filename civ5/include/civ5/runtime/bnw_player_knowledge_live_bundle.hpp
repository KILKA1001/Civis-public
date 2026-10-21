#pragma once
#include "civ5/runtime/bnw_player_knowledge_checkpoint.hpp"
#include "civ5/runtime/live_session.hpp"

namespace civ5::runtime {

struct BnwPlayerKnowledgeProphetBundle {
    LiveSessionState state;
    std::size_t changed_players=0;
};

// Detached post-context-restore boundary. ID35 may change only BNW
// CvPlayerReligions::m_iNumProphetsSpawned; all TechAI, research flags,
// ReligionAI, founding and FlavorManager values must equal the source.
// The owned ResearchTurnState must still match its raw inputs. This does not
// execute prophet creation, faith spending, religious AI or a full turn.
// Call before publishing a LiveSession, after all other owned contexts have
// been restored, and rebuild any dependent read caches afterward.
BnwPlayerKnowledgeProphetBundle restore_bnw_player_knowledge_prophet_bundle(
    const saves::CivisSaveSection &section,
    const LiveSessionState &baseline,
    const saves::SavedSession &source,
    const content::TechnologyRules &technologies,
    const content::InfoTypeCatalog &types,
    const content::RulesDatabase &database,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
