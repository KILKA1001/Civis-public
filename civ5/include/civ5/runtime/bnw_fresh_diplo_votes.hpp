#pragma once

#include "civ5/runtime/bnw_fresh_game_content.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshDiploVotesBlockerCode {
    selected_rules_unverified,
    special_session_info_unverified,
    diplomatic_defines_unverified,
    league_callback_and_ui_pending,
};

struct BnwFreshDiploVotesBlocker {
    BnwFreshDiploVotesBlockerCode code;
    std::string message;
};

struct BnwFreshDiploVotesDraft {
    std::int32_t votes_needed = 0;
    std::int32_t host_delegates = 1;
    std::int32_t civ_delegates = 1;
    std::int32_t city_state_delegates = 1;
    std::vector<std::int32_t> visited_special_session_ids;
    std::string selected_rules_sha256;
    std::vector<BnwFreshDiploVotesBlocker> blockers;
    bool zero_ever_alive_votes_computed = false;
    bool league_callback_complete = false;
    bool has_blocker(BnwFreshDiploVotesBlockerCode code) const;
    void rollback() noexcept;
};

// CvGame::DoUpdateDiploVictory at the first fresh setAlive boundary. SDK
// setAlive calls the league callback before writing m_bEverAlive, so no player
// contributes to the logarithmic counts yet. This computes only the selected
// rule-dependent vote field; GameLeagues membership and UI dirty remain owned
// by their actual callback services.
BnwFreshDiploVotesDraft draft_bnw_fresh_zero_ever_alive_diplo_votes(
    const BnwFreshGameContentResult &selected,
    std::string_view selected_rules_sha256);

} // namespace civ5::runtime
