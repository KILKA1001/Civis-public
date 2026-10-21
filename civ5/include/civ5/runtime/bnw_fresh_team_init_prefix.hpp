#pragma once

#include "civ5/runtime/bnw_fresh_pregame_roster.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshTeamInitStep : std::uint8_t {
    batch_free_and_pointer_clear,
    team_techs_uninit,
    scalar_reset,
    fixed_array_reset,
    team_id_write,
    selected_info_batch_pending,
};

struct BnwFreshTeamInitEvent {
    BnwFreshTeamInitStep step;
    std::int32_t team_id;
    bool operator==(const BnwFreshTeamInitEvent &) const = default;
};

enum class BnwFreshTeamInitBlockerCode {
    roster_or_native_constants_unverified,
    selected_info_batch_pending,
    team_techs_init_pending,
};

struct BnwFreshTeamInitBlocker {
    BnwFreshTeamInitBlockerCode code;
    std::string message;
};

struct BnwFreshTeamRelationReset {
    std::int32_t tech_share = 0;
    std::int32_t peace_treaty_turn = -1;
    std::int32_t ignored_warnings = 0;
    bool has_met = false, at_war = false, permanent_war_peace = false;
    bool embassy = false, open_borders = false, defensive_pact = false;
    bool research_agreement = false, trade_agreement = false;
    bool force_peace = false;
    bool operator==(const BnwFreshTeamRelationReset &) const = default;
};

struct BnwFreshTeamFixedState {
    std::int32_t id = -1; // NO_TEAM until reset(eID) writes it
    std::int32_t num_members = 0, alive_count = 0, ever_alive_count = 0;
    // CvTeam::uninit zeros these in source order: cities, population, land,
    // nuke interception, water sight, map/tech/gold/embassy/open-border/
    // defensive-pact/research/trade/permanent-alliance trading, bridges,
    // water work, river trade, border obstacle, victory points, embarked
    // moves, embark/defensive embark/all-water passage, natural wonders,
    // minor civs attacked. The nonzero route sentinel is kept separately.
    std::array<std::int32_t, 25> remaining_zero_counters{};
    std::int32_t best_possible_route = -1; // NO_ROUTE
    std::int32_t current_era = 0;
    std::int32_t liberated_by_team = -1, killed_by_team = -1;
    bool map_centering = false, broken_peace_treaty = false;
    bool home_of_united_nations = false, has_tech_for_world_congress = false;
    bool broken_military_promise = false, broken_expansion_promise = false;
    bool broken_border_promise = false, broken_city_state_promise = false;
    std::array<std::int32_t, 5> extra_moves{}; // SDK DomainTypes
    std::vector<BnwFreshTeamRelationReset> relations;
    std::vector<bool> found_players_territory;
    bool constructor_tech_arrays_empty = true;
    bool dynamic_info_arrays_allocated = false;
};

struct BnwFreshTeamInitPrefix {
    std::vector<BnwFreshTeamFixedState> teams;
    std::vector<BnwFreshTeamInitEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshTeamInitBlocker> blockers;
    bool fixed_reset_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshTeamInitBlockerCode code) const;
    void rollback() noexcept;
};

// CvGame::InitPlayers -> CvTeam::init -> reset: fresh constructor batch and
// TeamTechs arrays are empty. This owns the complete scalar/fixed-array reset
// for each MAX_TEAMS slot in SDK order. It stops before selected Info-sized
// allocations and CvTeamTechs::Init; it cannot publish an initialized team.
BnwFreshTeamInitPrefix draft_bnw_fresh_team_init_fixed_prefix(
    const BnwFreshPregameRosterDraft &roster,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants);

} // namespace civ5::runtime
