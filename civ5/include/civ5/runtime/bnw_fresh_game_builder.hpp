#pragma once

#include "civ5/runtime/bnw_fresh_city_specialization_init.hpp"
#include "civ5/runtime/bnw_fresh_league_state.hpp"
#include "civ5/runtime/bnw_fresh_slot_info.hpp"
#include "civ5/runtime/bnw_fresh_player_objects.hpp"
#include "civ5/runtime/bnw_fresh_player_uninit_plan.hpp"
#include "civ5/runtime/bnw_fresh_archive_vector.hpp"
#include "civ5/runtime/bnw_fresh_builder_player_diagnostic.hpp"
#include "civ5/runtime/bnw_fresh_team_info_init.hpp"
#include "civ5/runtime/bnw_fresh_first_player_state.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace civ5::runtime {

struct BnwFreshGameBuilderInputs {
    std::span<const content::GameplayDefine> native_constants;
    std::span<const BnwFreshPregameSlot> pregame_slots;
    std::span<const BnwFreshMajorColorSlot> major_color_slots;
    std::span<const std::int32_t> pregame_leader_ids;
    std::span<const std::int32_t> active_minor_civ_ids;
    std::int32_t selected_minor_count = -1;
    std::int32_t observer_team = -1;
    std::int32_t barbarian_civilization_id = -1;
};

enum class BnwFreshGameBuilderStep : std::uint8_t {
    team_init,
    major_color_pass,
    pregame_slot_write,
    notifications_allocated,
    notifications_constructor_uninit,
    diplomacy_requests_allocated,
    diplomacy_requests_constructor_uninit,
    reset_sync_archive_pending,
    owned_player_reset_through_city_specialization,
    owned_player_reset_through_wonder_production,
    owned_player_reset_through_grand_strategy,
    owned_player_reset_through_diplomacy,
    owned_player_reset_through_player_religions,
    owned_player_reset_through_religion_ai,
    owned_player_reset_through_player_techs,
    owned_player_reset_through_player_policies,
    owned_player_reset_through_tactical_reset,
};
struct BnwFreshGameBuilderEvent {
    BnwFreshGameBuilderStep step;
    std::int32_t index;
    bool operator==(const BnwFreshGameBuilderEvent &) const = default;
};
enum class BnwFreshGameBuilderBlockerCode {
    selected_content,
    pregame_roster,
    team_init,
    major_colors,
    selected_slot_info_pending,
    player_reset,
    economic_ai,
    military_ai,
    city_specialization_ai,
    player_init,
    first_alive,
    diplomatic_votes,
    league_callback,
    reset_sync_archive_pending,
    archive_reset_semantics_unavailable,
    diagnostic_exception,
    wonder_production_init_pending,
    grand_strategy_init_pending,
    diplomacy_ai_init_pending,
    player_religions_init_pending,
    religion_ai_init_pending,
    player_techs_init_pending,
    player_policies_init_pending,
    tactical_ai_init_pending,
    tactical_game_analysis_map_source_pending,
};
struct BnwFreshGameBuilderBlocker {
    BnwFreshGameBuilderBlockerCode code;
    std::string message;
};

// Legacy held projections remain diagnostics. first_player_state owns the
// executable first-player gameplay prefix through TacticalAI binding/Reset.
// Actual Game pointer/handicap and remaining Tactical/Homeland Init are pending.
// Remaining Reset/InitPlayers/world effects remain mandatory; no playable world.
struct BnwFreshGameBuilderResult {
    BnwFreshPregameRosterDraft roster;
    BnwFreshTeamInitPrefix team_fixed;
    BnwFreshTeamInfoInit team_info;
    BnwFreshPlayerColorsDraft colors;
    BnwFreshSlotInfoDraft slot_info;
    BnwFreshFirstPlayerObjects first_player_objects;
    BnwFreshPlayerUninitPlan first_player_uninit;
    BnwFreshArchiveVectorProjection first_resource_vector;
    BnwFreshBuilderPlayerDiagnostic first_player_diagnostic;
    BnwFreshPlayerResetPrefix player_early;
    BnwFreshPlayerInfoReset player_info;
    BnwFreshEconomicAIInit economic;
    BnwFreshMilitaryAIInit military;
    BnwFreshCitySpecializationInit city_specialization;
    BnwFreshPlayerInitPrefix player_init;
    BnwFreshAliveBeforeLeague first_alive;
    BnwFreshDiploVotesDraft diplomatic_votes;
    BnwFreshLeagueState league;
    std::vector<BnwFreshGameBuilderEvent> source_order_prefix;
    std::string selected_rules_sha256;
    std::optional<BnwFreshGameBuilderBlocker> frontier;
    bool detached_diagnostics_complete = false;
    bool initialized_by_gamecore = false;
    std::shared_ptr<const BnwFreshFirstPlayerState> first_player_state;
    void rollback() noexcept;
};

BnwFreshGameBuilderResult draft_bnw_fresh_gamecore(
    const BnwFreshGameContentResult &selected,
    const BnwFreshGameBuilderInputs &inputs);

} // namespace civ5::runtime
