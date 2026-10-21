#pragma once
#include <cstdint>
#include <set>
#include <vector>

namespace civ5::content { struct TechnologyRules; }
namespace civ5::runtime {
class TeamTechnologies;
struct ResearchPrerequisiteLimits;

// VP GameCore counts distinct prerequisite technologies within its bounded
// look-ahead. The shared selection dispatcher supplies the selected rules and
// team, but does not own this profile's traversal semantics.
std::int64_t vp_research_path_length(const content::TechnologyRules &rules,
    const TeamTechnologies &team, const ResearchPrerequisiteLimits &limits,
    std::int64_t technology);
// VP's AI helper stops at the first NO_TECH and combines AND/OR conditions
// with OR. It is not the common CanResearch gate.
bool vp_ai_prerequisites_met(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech, ResearchPrerequisiteLimits limits, const std::set<std::int64_t> &extra_techs = {});
std::vector<std::int64_t> vp_tech_frontier(const content::TechnologyRules &rules, const TeamTechnologies &team,
    ResearchPrerequisiteLimits limits);
} // namespace civ5::runtime
