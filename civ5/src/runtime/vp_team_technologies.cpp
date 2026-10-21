#include "civ5/runtime/vp_team_technologies_profile.hpp"
#include "civ5/runtime/team_technologies.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
VpTeamTechnologiesProfileState &profile(TeamTechnologies &team) {
    if (team.profile() != content::TechnologyProfile::vp542)
        throw std::logic_error("Team technology operation requires VP");
    auto *state = team.profile_extensions().find<VpTeamTechnologiesProfileState>();
    if (!state) throw std::logic_error("Unresolved VP team technology profile state");
    return *state;
}
const VpTeamTechnologiesProfileState &profile(const TeamTechnologies &team) {
    if (team.profile() != content::TechnologyProfile::vp542)
        throw std::logic_error("Team technology operation requires VP");
    const auto *state = team.profile_extensions().find<VpTeamTechnologiesProfileState>();
    if (!state) throw std::logic_error("Unresolved VP team technology profile state");
    return *state;
}
std::uint64_t index(const TeamTechnologies &team, std::int64_t tech) {
    if (tech < 0 || static_cast<std::uint64_t>(tech) >= team.slots())
        throw std::out_of_range("Team technology index");
    return static_cast<std::uint64_t>(tech);
}
std::int64_t add(std::int64_t a, std::int64_t b) {
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b))
        throw std::overflow_error("Team technology counter exceeds int64");
    return a + b;
}
} // namespace

void initialize_vp_team_technologies(ProfileExtensions &extensions) {
    extensions.emplace<VpTeamTechnologiesProfileState>();
}
std::uint64_t vp_team_occupied_slots(const TeamTechnologies &team) {
    const auto &state = profile(team);
    std::uint64_t count = team.nondefault_values().size();
    for (const auto &[slot, value] : state.eureka_counters)
        if (value != 0 && !team.nondefault_values().contains(slot)) ++count;
    return count;
}
std::vector<std::uint64_t> vp_team_occupied_indices(const TeamTechnologies &team) {
    const auto &state = profile(team);
    std::vector<std::uint64_t> result;
    result.reserve(team.nondefault_values().size());
    for (const auto &[slot, value] : team.nondefault_values()) result.push_back(slot);
    for (const auto &[slot, value] : state.eureka_counters)
        if (value != 0 && !team.nondefault_values().contains(slot)) result.push_back(slot);
    std::sort(result.begin(), result.end());
    return result;
}
std::int64_t vp_team_eureka(const TeamTechnologies &team, std::int64_t tech) {
    const auto &state = profile(team);
    if (tech == -1) return 0;
    const auto found = state.eureka_counters.find(index(team, tech));
    return found == state.eureka_counters.end() ? 0 : found->second;
}
void vp_team_set_eureka(TeamTechnologies &team, std::int64_t tech, std::int64_t value) {
    auto &state = profile(team);
    if (tech == -1) return;
    const auto slot = index(team, tech);
    if (value == 0) state.eureka_counters.erase(slot);
    else state.eureka_counters.insert_or_assign(slot, value);
}
std::int64_t vp_team_known_count(const TeamTechnologies &team) { return profile(team).known_count; }
void vp_team_change_known_count(TeamTechnologies &team, std::int64_t change) {
    auto &known = profile(team).known_count;
    known = add(known, change);
}
bool vp_team_all_researched(const TeamTechnologies &team) {
    const auto known = profile(team).known_count;
    return known >= 0 && static_cast<std::uint64_t>(known) >= team.slots();
}
} // namespace civ5::runtime
