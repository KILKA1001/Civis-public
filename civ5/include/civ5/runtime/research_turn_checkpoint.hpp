#pragma once
#include "civ5/runtime/research_turn.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/civis_save.hpp"

namespace civ5::runtime {
struct ResearchTurnCheckpointBudget {
    std::uint64_t players=1'000'000,entries=100'000'000;
    std::uint64_t bytes=1024ULL*1024*1024;
};
// Common live state changed by research turns: founded-city count, sparse
// agreement counters and researching flags. Profile-specific fields stay in
// the imported source until their owner has a separate profile section.
saves::CivisSaveSection make_research_turn_checkpoint(
    const ResearchTurnState&,const ResearchContext&,
    const content::TechnologyRules&,const SessionCheckpointIdentity&,
    ResearchTurnCheckpointBudget={});
// The complete section is parsed and checked before any live player changes.
void restore_research_turn_checkpoint(
    const saves::CivisSaveSection&,ResearchTurnState&,const ResearchContext&,
    const content::TechnologyRules&,const SessionCheckpointIdentity&,
    ResearchTurnCheckpointBudget={});
} // namespace civ5::runtime
