#pragma once

#include "civ5/runtime/research_selection.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/civis_save.hpp"

namespace civ5::runtime {

struct ResearchSelectionCheckpointBudget {
    std::uint64_t players=1'000'000, entries=100'000'000;
    std::uint64_t string_bytes=64ULL*1024*1024;
    std::uint64_t bytes=1024ULL*1024*1024;
};

// BNW selection inputs and the queue/researching flags changed by push_research
// are one logical checkpoint. Civilization restrictions come from the active
// rules database and are bound by rules_sha256 rather than duplicated here.
saves::CivisSaveSection make_research_selection_checkpoint(
    const ResearchSelectionState&, const PlayerResearchContext&,
    const ResearchTurnState&, const ResearchContext&,
    const content::TechnologyRules&, const SessionCheckpointIdentity&,
    ResearchSelectionCheckpointBudget={});

// Parse and validate the complete section before changing any of the three
// owned contexts. If companion overlays were present, compare their queue or
// flag values instead of silently accepting contradictory checkpoint sections.
void restore_research_selection_checkpoint(
    const saves::CivisSaveSection&, ResearchSelectionState&,
    PlayerResearchContext&, ResearchTurnState&, const ResearchContext&,
    const content::TechnologyRules&, const SessionCheckpointIdentity&,
    bool require_queue_match=false, bool require_flags_match=false,
    ResearchSelectionCheckpointBudget={});

} // namespace civ5::runtime
