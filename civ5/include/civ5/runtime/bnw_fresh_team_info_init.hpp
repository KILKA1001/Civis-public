#pragma once

#include "civ5/runtime/bnw_fresh_team_init_prefix.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshTeamInfoStep : std::uint8_t {
    fixed_batch_clear, fixed_techs_uninit, fixed_scalars,
    fixed_arrays, team_id_write, info_sizes_collected,
    info_batch_allocated, info_batch_values_reset,
    turn_relation_arrays_reset, improvement_yields_reset,
    team_techs_init_and_reset, project_art_allocated,
    revealed_resources_cleared,
};

struct BnwFreshTeamInfoEvent {
    BnwFreshTeamInfoStep step;
    std::int32_t team_id;
    bool operator==(const BnwFreshTeamInfoEvent &) const = default;
};

enum class BnwFreshTeamInfoBlockerCode {
    fixed_prefix_or_selected_rules_unverified,
    selected_info_collections_unverified,
    allocation_budget_exceeded,
    remaining_gamecore_init_pending,
};

struct BnwFreshTeamInfoBlocker {
    BnwFreshTeamInfoBlockerCode code;
    std::string message;
};

struct BnwFreshTeamInfoSizes {
    std::uint32_t vote_sources = 0, victories = 0, small_awards = 0;
    std::uint32_t routes = 0, builds = 0, projects = 0;
    std::uint32_t unit_classes = 0, building_classes = 0;
    std::uint32_t buildings = 0, terrains = 0, improvements = 0;
    std::uint32_t technologies = 0;
};

struct BnwFreshTeamInfoArrays {
    BnwFreshTeamFixedState fixed;
    std::vector<std::int32_t> vote_eligibility;
    std::vector<bool> can_launch, victory_achieved, small_award_achieved;
    std::vector<std::int32_t> victory_countdown;
    std::vector<std::int32_t> route_change, build_time_change;
    std::vector<std::int32_t> project_count, project_default_art, project_making;
    std::vector<std::int32_t> unit_class_count, building_class_count;
    std::vector<std::int32_t> obsolete_building_count, terrain_trade_count;
    std::vector<std::int32_t> turn_team_met, turns_at_war, turns_locked_into_war;
    std::vector<std::vector<std::int32_t>> improvement_yield;
    std::vector<std::vector<std::int32_t>> improvement_no_fresh_water_yield;
    std::vector<std::vector<std::int32_t>> improvement_fresh_water_yield;
    std::vector<bool> has_tech, no_trade_tech;
    std::vector<std::int32_t> research_progress, tech_count;
    std::int32_t last_tech_acquired = -1; // NO_TECH
    std::vector<std::vector<std::int32_t>> project_art_types;
    std::vector<std::int32_t> revealed_resources;
};

struct BnwFreshTeamInfoInit {
    BnwFreshTeamInfoSizes sizes;
    std::vector<BnwFreshTeamInfoArrays> teams;
    std::vector<BnwFreshTeamInfoEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshTeamInfoBlocker> blockers;
    bool info_and_tech_arrays_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshTeamInfoBlockerCode code) const;
    void rollback() noexcept;
};

// Continues the detached fresh CvTeam::reset(eID) for every selected team.
// Info collection slots, not SQL row counts, size the arrays. Source-order
// events interleave each team's fixed reset with its Info and TeamTechs work.
BnwFreshTeamInfoInit draft_bnw_fresh_team_info_init(
    const BnwFreshGameContentResult &selected,
    const BnwFreshTeamInitPrefix &fixed,
    std::string_view selected_rules_sha256,
    std::uint64_t maximum_total_cells = 32ULL * 1024 * 1024);

} // namespace civ5::runtime
