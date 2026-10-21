#pragma once
#include <cstdint>
#include <map>
#include <vector>

namespace civ5::runtime {
class TeamTechnologies;
struct ProfileExtensions;
// The GameCore profile owns state absent from ordinary BNW team technologies.
// ProfileExtensions stores this type only for an explicitly selected VP team.
struct VpTeamTechnologiesProfileState {
    std::int64_t known_count = 0;
    std::map<std::uint64_t, std::int64_t> eureka_counters;
};
void initialize_vp_team_technologies(ProfileExtensions &extensions);
std::uint64_t vp_team_occupied_slots(const TeamTechnologies &team);
std::vector<std::uint64_t> vp_team_occupied_indices(const TeamTechnologies &team);
std::int64_t vp_team_eureka(const TeamTechnologies &team, std::int64_t tech);
void vp_team_set_eureka(TeamTechnologies &team, std::int64_t tech, std::int64_t value);
std::int64_t vp_team_known_count(const TeamTechnologies &team);
void vp_team_change_known_count(TeamTechnologies &team, std::int64_t change);
bool vp_team_all_researched(const TeamTechnologies &team);
} // namespace civ5::runtime
