#include "civ5/runtime/research.hpp"
#include "civ5/runtime/vp_research_cost.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
void validate(const content::TechnologyRules &rules, const TeamTechnologies &team) {
    if (rules.profile != team.profile() || rules.collection.slots != team.slots() || !rules.all_succeeded())
        throw std::invalid_argument("Research requires matching complete technology rules and team state");
}
void profile(const TeamTechnologies &team, TechnologyProfile expected) {
    if (team.profile() != expected) throw std::invalid_argument("Research operation has a different source profile");
}
const content::TechnologyInfo *entry(const content::TechnologyRules &rules, std::int64_t tech) {
    return tech < 0 ? nullptr : rules.find(static_cast<std::uint64_t>(tech));
}
// Skip default-filled spans without walking MAX(ID). Respect early exit before
// an out-of-array read; otherwise diagnose the source's invalid array access.
template<class F> void prerequisites(const content::RuleValues &list, std::int64_t limit, F visit) {
    if (limit <= 0) return;
    if (list.default_value != -1) throw std::invalid_argument("Research prerequisite default must be NO_TECH");
    const auto count = static_cast<std::uint64_t>(limit);
    for (const auto &[index, tech] : list.values) {
        if (index >= count || index >= list.extent) break;
        if (tech != -1 && !visit(tech)) return;
    }
    if (count > list.extent) throw std::out_of_range("Research prerequisite limit exceeds source array extent");
}
} // namespace
std::int64_t team_research_cost(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech, const TeamResearchFactors &f) {
    validate(rules, team);
    if (tech < 0) throw std::out_of_range("Research cost requires an assigned technology");
    const auto *info = entry(rules, tech);
    if (!info) return 0;
    const auto base = info->integers.at("Cost");
    auto cost = base;
    if (rules.profile == TechnologyProfile::bnw) {
        const auto member_delta = wide::subtract(f.members, 1);
        if (f.team_handicap_percent) cost = wide::multiply_divide(cost, *f.team_handicap_percent, 100);
        cost = wide::multiply_divide(cost, f.world_percent, 100);
        cost = wide::multiply_divide(cost, f.speed_percent, 100);
        cost = wide::multiply_divide(cost, f.start_era_percent, 100);
        const auto factor = std::max<std::int64_t>(0,
            wide::add(wide::multiply_divide(f.extra_team_member_modifier, member_delta, 1), 100));
        cost = wide::multiply_divide(cost, factor, 100);
    } else if (rules.profile == TechnologyProfile::vp542) {
        cost = vp_team_research_cost_unclamped(*info, team, tech, f);
    } else {
        throw std::invalid_argument("Unsupported research cost GameCore profile");
    }
    return std::max<std::int64_t>(1, cost);
}
std::int64_t bnw_research_left(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech, const TeamResearchFactors &factors) {
    profile(team, TechnologyProfile::bnw);
    return std::max<std::int64_t>(0, wide::subtract(team_research_cost(rules, team, tech, factors), team.progress(tech)));
}
ResearchPrerequisiteLimits research_prerequisite_limits(const content::GlobalDefineCache &defines) {
    const auto get = [&](const char *name) {
        const auto *value = defines.find(name);
        if (!value || !value->lookup.error.empty() || !std::holds_alternative<std::int64_t>(value->lookup.value))
            throw std::invalid_argument("Missing research prerequisite define cache entry");
        return std::get<std::int64_t>(value->lookup.value);
    };
    return {get("NUM_AND_TECH_PREREQS"), get("NUM_OR_TECH_PREREQS")};
}
bool research_prerequisites_met(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech, ResearchPrerequisiteLimits limits, bool trading) {
    validate(rules, team);
    const auto *info = entry(rules, tech);
    if (!info) return false;
    bool possible = false, valid = false;
    prerequisites(info->or_prerequisites, limits.or_count, [&](std::int64_t prereq) {
        possible = true;
        valid = team.has_tech(prereq) && (!trading || !team.at(prereq).no_trade);
        return !valid;
    });
    if (possible && !valid) return false;
    bool all = true;
    prerequisites(info->and_prerequisites, limits.and_count, [&](std::int64_t prereq) {
        all = team.has_tech(prereq) && (!trading || !team.at(prereq).no_trade);
        return all;
    });
    return all;
}
} // namespace civ5::runtime
