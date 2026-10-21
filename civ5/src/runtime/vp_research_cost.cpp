#include "civ5/runtime/vp_research_cost.hpp"
#include "civ5/runtime/vp_research_cost_profile.hpp"
#include "civ5/runtime/vp_team_technologies_profile.hpp"
#include "civ5/runtime/research.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
std::int64_t era_factor(std::int64_t per_era_modifier, std::int64_t era) {
    return std::max<std::int64_t>(0, wide::add(wide::multiply_divide(per_era_modifier, era, 1), 100));
}
} // namespace

std::int64_t vp_team_research_cost_unclamped(const content::TechnologyInfo &info,
    const TeamTechnologies &team, std::int64_t tech, const TeamResearchFactors &f) {
    const auto &vp=vp_research_cost_profile(f);
    const auto base=info.integers.at("Cost");
    std::int64_t modifier = 100;
    if (f.team_handicap_percent) {
        modifier = wide::multiply_divide(modifier, *f.team_handicap_percent, 100);
        modifier = wide::multiply_divide(modifier, era_factor(vp.team_handicap_per_era_modifier, vp.current_era), 100);
        if (!vp.human_for_handicap) {
            modifier = wide::multiply_divide(modifier, vp.game_ai_handicap_percent, 100);
            modifier = wide::multiply_divide(modifier, era_factor(vp.game_ai_per_era_modifier, vp.current_era), 100);
        }
    }
    modifier = wide::multiply_divide(modifier, f.world_percent, 100);
    modifier = wide::multiply_divide(modifier, f.speed_percent, 100);
    modifier = wide::multiply_divide(modifier, f.start_era_percent, 100);
    const auto member_delta = wide::subtract(f.members, 1);
    const auto factor = wide::add(100, std::max<std::int64_t>(0,
        wide::multiply_divide(f.extra_team_member_modifier, member_delta, 1)));
    modifier = wide::multiply_divide(modifier, factor, 100);
    if (vp.eurekas_enabled) {
        const auto earned = wide::multiply_divide(info.integers.at("EurekaPerMillion"), vp_team_eureka(team, tech),
                                                   std::max<std::int64_t>(1, f.members));
        const auto remaining = std::max<std::int64_t>(0, wide::subtract(1000000, earned) / 10000);
        modifier = wide::add(modifier, wide::subtract(remaining, 100));
    }
    // Original switches to (base/100)*modifier at base>=10000 to avoid
    // x86 overflow. A wide product removes that precision-losing workaround.
    return wide::multiply_divide(base, modifier, 100);
}
std::int64_t vp_research_left_times100(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech, const TeamResearchFactors &factors) {
    if (team.profile() != content::TechnologyProfile::vp542)
        throw std::invalid_argument("Research operation has a different source profile");
    return std::max<std::int64_t>(0, wide::subtract(
        wide::multiply_divide(team_research_cost(rules, team, tech, factors), 100, 1), team.progress_times100(tech)));
}
std::int64_t vp_eureka_discount(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech) {
    if (rules.profile != team.profile() || rules.collection.slots != team.slots() || !rules.all_succeeded())
        throw std::invalid_argument("Research requires matching complete technology rules and team state");
    if (team.profile() != content::TechnologyProfile::vp542)
        throw std::invalid_argument("Research operation has a different source profile");
    if (tech < 0) throw std::out_of_range("Eureka discount requires an assigned technology");
    const auto *info = rules.find(static_cast<std::uint64_t>(tech));
    if (!info) return 0;
    return std::max<std::int64_t>(0, wide::subtract(1000000,
        wide::multiply_divide(info->integers.at("EurekaPerMillion"), vp_team_eureka(team, tech), 1)) / 10000);
}
} // namespace civ5::runtime
