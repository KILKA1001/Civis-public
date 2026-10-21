#pragma once
#include <cstdint>

namespace civ5::content { struct TechnologyInfo; struct TechnologyRules; }
namespace civ5::runtime {
class TeamTechnologies;
struct TeamResearchFactors;

// Called by the shared dispatcher after rule/team/technology validation.
// The common dispatcher applies the final minimum cost for all profiles.
std::int64_t vp_team_research_cost_unclamped(const content::TechnologyInfo &info,
    const TeamTechnologies &team, std::int64_t tech, const TeamResearchFactors &factors);
std::int64_t vp_research_left_times100(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech, const TeamResearchFactors &factors);
// Source GetEurekaDiscount returns the remaining percentage and does not
// divide by team size. This getter belongs to the VP GameCore profile.
std::int64_t vp_eureka_discount(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech);
} // namespace civ5::runtime
