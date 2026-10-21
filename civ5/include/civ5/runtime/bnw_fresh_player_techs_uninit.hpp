#pragma once

#include "civ5/runtime/bnw_fresh_league_ai_uninit.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace civ5::runtime {

// An engaged optional represents an allocated SDK array/pointer. The
// constructor leaves every pointer null; Uninit frees without changing the
// tech/player bindings or the flavor recipient's m_bIsCity.
struct BnwFreshPlayerTechsServiceState {
    std::optional<bool> is_city;
    bool tech_entries_bound = false;
    bool player_bound = false;
    std::optional<std::vector<std::int32_t>> latest_flavor_values;
    std::optional<std::vector<bool>> researching_tech;
    std::optional<std::vector<std::int32_t>> civ_tech_priority;
    std::optional<std::vector<std::int32_t>> locale_tech_priority;
    std::optional<std::vector<std::int32_t>> locale_tech_resources;
    std::optional<std::vector<std::int32_t>> civ_tech_unique_buildings;
    std::optional<std::vector<std::int32_t>> civ_tech_unique_units;
    std::optional<std::vector<std::int32_t>> civ_tech_unique_improvements;
    // CvTechAI has a binding back to this CvPlayerTechs and owned weights.
    std::optional<std::vector<std::pair<std::int32_t,std::int32_t>>> tech_ai_weights;
    bool operator==(const BnwFreshPlayerTechsServiceState &) const = default;
};

struct BnwFreshPlayerTechsUninitDraft {
    BnwFreshPlayerTechsServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

BnwFreshPlayerTechsServiceState make_bnw_fresh_player_techs_constructor_state();

// Expansion2 CvPlayer.cpp:681, CvTechClasses.cpp:641-654 and
// CvFlavorManager.cpp:37-40. Only detached member effects are projected;
// FAutoArchive traversal and the following FlavorManager Uninit remain blocked.
BnwFreshPlayerTechsUninitDraft project_bnw_fresh_player_techs_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshLeagueAiUninitDraft &league_ai,
    std::string_view selected_rules_sha256,
    BnwFreshPlayerTechsServiceState initial);

} // namespace civ5::runtime
