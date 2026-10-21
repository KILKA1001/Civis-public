#pragma once
#include "civ5/runtime/team_technologies.hpp"
#include "civ5/runtime/profile_extensions.hpp"
#include "civ5/content/global_defines.hpp"

namespace civ5::runtime {
// Inputs supplied by the owning game/team and their active Info collections.
// These are live getter values, not substitutes inferred from save filenames.
struct TeamResearchFactors {
    // Shared cost inputs only. GameCore-specific cost factors live in extensions.
    std::optional<std::int64_t> team_handicap_percent;
    std::int64_t members = 1;
    std::int64_t world_percent = 100, speed_percent = 100, start_era_percent = 100;
    std::int64_t extra_team_member_modifier = 0;
    ProfileExtensions profile_extensions;
};
std::int64_t team_research_cost(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech, const TeamResearchFactors &factors);
std::int64_t bnw_research_left(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech, const TeamResearchFactors &factors);

struct ResearchPrerequisiteLimits { std::int64_t and_count = 0, or_count = 0; };
ResearchPrerequisiteLimits research_prerequisite_limits(const content::GlobalDefineCache &defines);
// The OR/AND part of CvPlayerTechs::CanResearch. Other gates (city founded,
// civilization restrictions, NO_SCIENCE, era, Lua) belong to the player layer.
bool research_prerequisites_met(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech, ResearchPrerequisiteLimits limits, bool trading = false);
} // namespace civ5::runtime
