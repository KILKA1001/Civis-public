#pragma once
#include "civ5/runtime/profile_extensions.hpp"
#include "civ5/runtime/research_context.hpp"
#include <set>

namespace civ5::runtime {
struct ResearchCityState {
    bool puppet = false, ignore_happiness = false, razing = false;
    ProfileExtensions profile_extensions;
};
struct PlayerResearchState {
    std::vector<std::int64_t> queue; // raw saved IDs, not inferred from researching flags
    std::int64_t overflow_times100 = 0, max_effective_cities = 1;
    std::int64_t capital = -1;
    bool pregame_minor = false;
    std::map<std::int64_t, ResearchCityState> cities;
    ProfileExtensions profile_extensions;
    std::int64_t current_research() const { return queue.empty() ? -1 : queue.front(); }
};
struct ResearchTeamState {
    std::int64_t alive_count = 0, master = -1, liberated_by = -1;
    std::vector<std::int64_t> members;
    std::set<std::int64_t> met, embassy, at_war;
    struct WarPeaceState {
        std::map<std::int64_t,std::int64_t> locked_turns;
        std::set<std::int64_t> permanent;
    };
    std::optional<WarPeaceState> war_peace;
};
struct ResearchLeagueState {
    std::vector<std::int64_t> members;
    std::vector<std::int64_t> discovered_tech_modifiers;
};
// Research-relevant owned state; the base ResearchContext remains the single
// authority for player/team assignment, alive/human status and game settings.
struct PlayerResearchContext {
    content::TechnologyProfile profile = content::TechnologyProfile::bnw;
    std::int64_t world_city_modifier = 0, barbarian_team = -1;
    std::map<std::int64_t, PlayerResearchState> players;
    std::map<std::int64_t, ResearchTeamState> teams;
    std::vector<ResearchLeagueState> leagues;
    // Membership in source loop domains, not host capacities. New/wider worlds
    // can populate these sets with arbitrary wide IDs, independently of order.
    std::set<std::int64_t> civ_teams, major_players, bnw_minor_scan_players;
    bool team_is_minor(const ResearchContext &base, std::int64_t team) const;
    bool team_is_major(const ResearchContext &base, std::int64_t team) const;
    bool embassy_at(std::int64_t from, std::int64_t to) const;
    bool spy_at(const ResearchContext &base, std::int64_t from, std::int64_t to) const;
};
PlayerResearchContext restore_player_research_context(const saves::SavedSessionSnapshot &snapshot,
    const ResearchContext &base, const content::GlobalDefineCache &defines);
struct PlayerResearchModifier {
    std::int64_t value = 100, known_teams = 0, possible_teams = 0, known_or_prerequisites = 0;
    std::int64_t catch_up = 0, league = 0;
    std::vector<std::string> missing_infos;
};
PlayerResearchModifier player_research_modifier(PlayerResearchContext &state, const ResearchContext &base,
    const content::ResearchRules &rules, const content::TechnologyRules &technologies,
    const content::GlobalDefineCache &defines, const content::CustomModOptions *options,
    std::span<const TeamTechnologies> teams, std::int64_t player, std::int64_t tech);
// BNW's getter updates the saved historical maximum.
std::int64_t bnw_max_effective_cities(PlayerResearchState &player, bool include_puppets);
struct PlayerResearchCost {
    PlayerResearchModifier modifier;
    BoundTeamResearch team;
    std::int64_t team_cost = 0, cost = 0, city_modifier_times100 = 0;
    // The completion threshold is not always cost*100: BNW tests its
    // unrounded hundredths, whereas the player-facing cost rounds upward.
    std::int64_t completion_threshold_times100 = 0;
};
struct PlayerResearchQuote : PlayerResearchCost { std::int64_t progress = 0; };
PlayerResearchCost player_research_cost(PlayerResearchContext &state, const ResearchContext &base,
    const content::ResearchRules &rules, const content::TechnologyRules &technologies,
    const content::GlobalDefineCache &defines, const content::CustomModOptions *options,
    std::span<const TeamTechnologies> teams, std::int64_t player, std::int64_t tech,
    bool ignore_cities = false, std::int64_t city_offset = 0);
// Progress is whole beakers in BNW (team and overflow separately truncated),
// hundredths in VP. No turn advancement or technology completion occurs here.
// Source query side effects are retained in owned state: BNW max city count;
// VP science-from-league cache only when its source branch actually runs.
PlayerResearchQuote player_research_quote(PlayerResearchContext &state, const ResearchContext &base,
    const content::ResearchRules &rules, const content::TechnologyRules &technologies,
    const content::GlobalDefineCache &defines, const content::CustomModOptions *options,
    std::span<const TeamTechnologies> teams, std::int64_t player, std::int64_t tech,
    bool ignore_cities = false, std::int64_t city_offset = 0);
} // namespace civ5::runtime
