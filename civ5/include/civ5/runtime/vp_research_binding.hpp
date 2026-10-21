#pragma once
#include "civ5/runtime/research_context.hpp"

namespace civ5::runtime {
// VP GameCore owns its handicap, era and eureka binding. The shared dispatcher
// calls this only for a VP ruleset; other profiles never need VP cost state.
BoundTeamResearch bind_vp_team_research(const ResearchContext &context,
    const content::ResearchRules &rules,const content::GlobalDefineCache &defines,
    const content::CustomModOptions &options,const TeamTechnologies &team);
} // namespace civ5::runtime
