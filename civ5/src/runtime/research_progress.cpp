#include "civ5/runtime/research_progress.hpp"
#include "civ5/runtime/vp_research_cost.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
bool vp(const ResearchProgressContext &c) { return c.setup.profile == content::TechnologyProfile::vp542; }
TeamTechnologies &team_state(ResearchProgressContext &c, std::int64_t id) {
    TeamTechnologies *found = nullptr;
    for (auto &t : c.teams) if (t.team() == id) {
        if (found) throw std::runtime_error("Duplicate research progress team");
        found = &t;
    }
    if (!found || found->profile() != c.setup.profile || found->slots() != c.technologies.collection.slots)
        throw std::runtime_error("Research progress requires matching team/profile/span");
    return *found;
}
void validate(ResearchProgressContext &c, std::int64_t player) {
    if (c.players.profile != c.setup.profile || c.rules.profile != c.setup.profile ||
        c.technologies.profile != c.setup.profile || !c.rules.all_succeeded() || !c.technologies.all_succeeded() ||
        c.defines.profile() != (vp(c) ? content::GlobalDefineProfile::vp542 : content::GlobalDefineProfile::bnw) ||
        (vp(c) && (!c.options || !c.options->all_succeeded())) ||
        !c.setup.players.contains(player) || !c.players.players.contains(player))
        throw std::runtime_error("Research progress requires complete matching rules and player state");
}
} // namespace
void set_player_research_overflow_times100(ResearchProgressContext &c, std::int64_t player, std::int64_t value) {
    if (vp(c) && c.no_science) return;
    c.players.players.at(player).overflow_times100 = value;
}
void change_player_research_overflow_times100(ResearchProgressContext &c, std::int64_t player, std::int64_t change) {
    if (vp(c) && c.no_science) return;
    set_player_research_overflow_times100(c,player,wide::add(c.players.players.at(player).overflow_times100,change));
}
std::int64_t max_research_overflow(ResearchProgressContext &c, std::int64_t tech, std::int64_t player) {
    const auto five_turns = wide::multiply_divide(c.world.science_times100(player),5,1);
    const auto *info = tech < 0 ? nullptr : c.technologies.find(static_cast<std::uint64_t>(tech));
    if (!info) return 0;
    return std::max(five_turns,wide::multiply_divide(info->integers.at("Cost"),100,1));
}
ResearchProgressChange set_research_progress_times100(ResearchProgressContext &c,
    std::int64_t team_id, std::int64_t tech, std::int64_t value, std::int64_t player,
    std::int64_t player_overflow, std::int64_t overflow_divisor) {
    ResearchProgressChange result;
    // VP's failsafe runs even on the direct setter; NO_SCIENCE does not.
    if (vp(c) && (player == -1 || tech == -1)) { result.skipped=true; return result; }
    validate(c,player);
    auto &team = team_state(c,team_id);
    result.previous = result.assigned = team.at(tech).progress_times100;
    if (value == result.previous) return result;
    result.changed = true;
    // Retain VP's lower clamp, remove its x86 INT_MAX ceiling. BNW only
    // asserts nonnegative in debug; its release setter stores the raw value.
    result.assigned = vp(c) ? std::max<std::int64_t>(0,value) : value;
    team.set_progress_times100(tech,result.assigned);
    if (team_id == c.active_team) c.world.game_data_and_score_dirty(team_id);
    // BNW computes the threshold using THIS CvTeamTechs' cost even if the
    // credited player belongs to another team. VP asks the player's own cost.
    if (!vp(c)) {
        const auto binding = bind_team_research(c.setup,c.rules,c.defines,c.options,team);
        const auto cost = team_research_cost(c.technologies,team,tech,binding.factors);
        const auto modifier = player_research_modifier(c.players,c.setup,c.rules,c.technologies,c.defines,c.options,c.teams,player,tech);
        auto threshold = wide::multiply_divide(cost,10000,std::max<std::int64_t>(1,modifier.value));
        const auto cities = bnw_max_effective_cities(c.players.players.at(player),true);
        const auto city_modifier = wide::multiply_divide(c.players.world_city_modifier,cities,1);
        result.threshold = wide::multiply_divide(threshold,wide::add(100,city_modifier),100);
    } else {
        result.threshold = player_research_cost(c.players,c.setup,c.rules,c.technologies,c.defines,c.options,c.teams,player,tech).completion_threshold_times100;
    }
    auto overflow = wide::subtract(team.progress_times100(tech),result.threshold);
    if (!vp(c)) overflow = std::min(overflow,max_research_overflow(c,tech,player));
    if (overflow < 0) return result;
    if (vp(c)) {
        // Separate branch rounding is observable: do not scale the whole sum.
        overflow = overflow > player_overflow
            ? wide::add(wide::subtract(overflow,player_overflow),wide::multiply_divide(player_overflow,100,overflow_divisor))
            : wide::multiply_divide(overflow,100,overflow_divisor);
    }
    result.overflow_granted = vp(c) && c.no_science ? 0 : overflow;
    change_player_research_overflow_times100(c,player,overflow);
    c.world.acquire_technology(team_id,tech,player);
    team.set_no_trade(tech,true);
    c.world.city_specializations_dirty(player);
    for (auto id : c.players.major_players) if (c.setup.players.at(id).team == team_id) {
        const auto culture = c.world.culture_per_tech(id);
        if (culture != 0) c.world.add_culture(id,culture);
    }
    result.completed = true;
    return result;
}
ResearchProgressChange change_research_progress_times100(ResearchProgressContext &c,
    std::int64_t team, std::int64_t tech, std::int64_t change, std::int64_t player,
    std::int64_t player_overflow, std::int64_t overflow_divisor) {
    if (vp(c) && c.no_science) { ResearchProgressChange r; r.skipped=true; return r; }
    const auto value = wide::add(team_state(c,team).progress_times100(tech),change);
    return set_research_progress_times100(c,team,tech,value,player,player_overflow,overflow_divisor);
}
ResearchProgressChange set_research_progress(ResearchProgressContext &c,
    std::int64_t team, std::int64_t tech, std::int64_t value, std::int64_t player) {
    return set_research_progress_times100(c,team,tech,wide::multiply_divide(value,100,1),player);
}
ResearchProgressChange change_research_progress(ResearchProgressContext &c,
    std::int64_t team, std::int64_t tech, std::int64_t change, std::int64_t player) {
    return change_research_progress_times100(c,team,tech,wide::multiply_divide(change,100,1),player);
}
std::int64_t change_research_progress_percent(ResearchProgressContext &c,
    std::int64_t team_id, std::int64_t tech, std::int64_t percent, std::int64_t player) {
    if (percent == 0) return 0;
    auto &team = team_state(c,team_id);
    if (team.has_tech(tech)) return 0;
    validate(c,player);
    const auto binding = bind_team_research(c.setup,c.rules,c.defines,c.options,team);
    const auto cost = team_research_cost(c.technologies,team,tech,binding.factors);
    const auto remaining = vp(c) ? vp_research_left_times100(c.technologies,team,tech,binding.factors)
                                : bnw_research_left(c.technologies,team,tech,binding.factors);
    const auto cost_units = vp(c) ? wide::multiply_divide(cost,100,1) : cost;
    const auto fraction = wide::multiply_divide(cost,percent,vp(c) ? 1 : 100);
    const auto beakers = percent > 0 ? std::min(remaining,fraction) : std::max(wide::subtract(remaining,cost_units),fraction);
    change_research_progress_times100(c,team_id,tech,vp(c) ? beakers : wide::multiply_divide(beakers,100,1),player);
    return vp(c) ? beakers/100 : beakers;
}
} // namespace civ5::runtime
