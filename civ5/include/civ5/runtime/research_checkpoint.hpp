#pragma once
#include "civ5/runtime/player_research.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/civis_save.hpp"
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace civ5::runtime {
using ResearchCheckpointIdentity=SessionCheckpointIdentity;
struct ResearchCheckpointBudget {
    std::uint64_t teams=1'000'000,players=1'000'000,rows=100'000'000;
    std::uint64_t bytes=1024ULL*1024*1024;
};
struct ResearchCheckpointSections {
    saves::CivisSaveSection common;
    // Owned and interpreted by the selected GameCore module, when present.
    std::optional<saves::CivisSaveSection> profile;
};
// The common checkpoint owns only the shared research rows. A GameCore module
// supplies its wire identity and optional private section through this seam.
struct ResearchCheckpointProfileCodec {
    std::uint32_t wire_id;
    saves::CivisSaveSection (*make)(std::span<const TeamTechnologies>,
        const PlayerResearchContext &,const saves::CivisSaveSection &,
        ResearchCheckpointBudget)=nullptr;
    void (*restore)(const saves::CivisSaveSection &,const saves::CivisSaveSection &,
        std::vector<TeamTechnologies> &,PlayerResearchContext &,
        ResearchCheckpointBudget)=nullptr;
};
ResearchCheckpointProfileCodec research_checkpoint_profile_codec(content::TechnologyProfile);
// Preserve the complete shared team technology rows and live player queue,
// overflow and historical city maximum. Private counters use a profile section.
ResearchCheckpointSections make_research_checkpoint(
    std::span<const TeamTechnologies> teams,const PlayerResearchContext &players,
    const ResearchContext &setup,const content::TechnologyRules &rules,
    const ResearchCheckpointIdentity &identity,ResearchCheckpointBudget budget={});
// Parses and validates both sections before changing either live object.
void restore_research_checkpoint(
    const saves::CivisSaveSection &common,
    const std::optional<saves::CivisSaveSection> &profile,
    std::vector<TeamTechnologies> &teams,PlayerResearchContext &players,
    const ResearchContext &setup,const content::TechnologyRules &rules,
    const ResearchCheckpointIdentity &identity,ResearchCheckpointBudget budget={});
} // namespace civ5::runtime
