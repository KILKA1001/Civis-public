#include "civ5/runtime/bnw_fresh_diplo_votes.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
void block(BnwFreshDiploVotesDraft &draft,
           BnwFreshDiploVotesBlockerCode code, std::string message) {
    draft.blockers.push_back({code, std::move(message)});
}

std::optional<float> source_define(const content::RulesDatabase &rules,
                                   std::string_view name) {
    const auto found = rules.global_define(name, content::GlobalDefineKind::source_float);
    if (!found.found || !found.error.empty() ||
        !std::holds_alternative<double>(found.value)) return std::nullopt;
    const auto value = static_cast<float>(std::get<double>(found.value));
    if (!std::isfinite(value)) return std::nullopt;
    return value;
}
} // namespace

bool BnwFreshDiploVotesDraft::has_blocker(
    BnwFreshDiploVotesBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshDiploVotesDraft::rollback() noexcept {
    votes_needed = 0;
    host_delegates = civ_delegates = city_state_delegates = 1;
    visited_special_session_ids.clear();
    zero_ever_alive_votes_computed = false;
    league_callback_complete = false;
}

BnwFreshDiploVotesDraft draft_bnw_fresh_zero_ever_alive_diplo_votes(
    const BnwFreshGameContentResult &selected,
    std::string_view selected_rules_sha256) {
    BnwFreshDiploVotesDraft result;
    if (!selected.content_ready() || !selected.activation || !selected.rules ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        selected_rules_sha256.empty()) {
        block(result, BnwFreshDiploVotesBlockerCode::selected_rules_unverified,
              "First BNW diplomatic vote update requires activated selected rules");
        return result;
    }
    result.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    if (result.selected_rules_sha256 != selected_rules_sha256) {
        block(result, BnwFreshDiploVotesBlockerCode::selected_rules_unverified,
              "Diplomatic vote rules differ from fresh game rules");
        return result;
    }

    try {
        std::int64_t next_id = 0;
        const auto rows = selected.rules->query_rows(
            {"SELECT ID, UnitedNations, HostDelegates, CivDelegates, "
             "CityStateDelegates FROM LeagueSpecialSessions WHERE ID > -1 ORDER BY ID", {}},
            [&](const content::RulesRowView &row) {
                const auto id = row.integer("ID");
                if (id != next_id++ || id > std::numeric_limits<std::int32_t>::max())
                    throw std::invalid_argument("League special session Info ID gap");
                result.visited_special_session_ids.push_back(static_cast<std::int32_t>(id));
                if (row.integer("UnitedNations") == 0) return;
                const auto host = row.integer("HostDelegates");
                const auto civ = row.integer("CivDelegates");
                const auto minor = row.integer("CityStateDelegates");
                if (host < 0 || civ < 0 || minor < 0 ||
                    host > std::numeric_limits<std::int32_t>::max() ||
                    civ > std::numeric_limits<std::int32_t>::max() ||
                    minor > std::numeric_limits<std::int32_t>::max())
                    throw std::invalid_argument("Invalid United Nations delegates");
                // Source scans every Info row and the last UnitedNations row wins.
                result.host_delegates = static_cast<std::int32_t>(host);
                result.civ_delegates = static_cast<std::int32_t>(civ);
                result.city_state_delegates = static_cast<std::int32_t>(minor);
            }, 4096);
        if (!rows.all_succeeded() || rows.rows != result.visited_special_session_ids.size())
            throw std::invalid_argument("Selected league special sessions are incomplete");
    } catch (const std::exception &) {
        result.rollback();
        block(result, BnwFreshDiploVotesBlockerCode::special_session_info_unverified,
              "Selected LeagueSpecialSessions Info is incomplete");
        return result;
    }

    const auto civ_coefficient = source_define(
        *selected.rules, "DIPLO_VICTORY_CIV_DELEGATES_COEFFICIENT");
    const auto civ_constant = source_define(
        *selected.rules, "DIPLO_VICTORY_CIV_DELEGATES_CONSTANT");
    const auto minor_coefficient = source_define(
        *selected.rules, "DIPLO_VICTORY_CS_DELEGATES_COEFFICIENT");
    const auto minor_constant = source_define(
        *selected.rules, "DIPLO_VICTORY_CS_DELEGATES_CONSTANT");
    // At zero ever-alive players, positive coefficients multiply log(0) to
    // negative infinity; both portions are then clamped to zero in the SDK.
    // Zero/negative coefficients make this x86 float/cast path undefined or
    // different, so no vote result is published for altered unproven rules.
    if (!civ_coefficient || !civ_constant || !minor_coefficient ||
        !minor_constant || *civ_coefficient <= 0.0f ||
        *minor_coefficient <= 0.0f ||
        result.host_delegates > std::numeric_limits<std::int32_t>::max() -
                                    result.civ_delegates - 1) {
        result.rollback();
        block(result, BnwFreshDiploVotesBlockerCode::diplomatic_defines_unverified,
              "Selected diplomatic float defines do not prove the zero-count SDK path");
        return result;
    }
    const auto lower_bound = result.host_delegates + result.civ_delegates + 1;
    result.votes_needed = std::min(result.host_delegates,
                                   std::max(lower_bound, 0));
    result.zero_ever_alive_votes_computed = true;
    block(result, BnwFreshDiploVotesBlockerCode::league_callback_and_ui_pending,
          "GameLeagues membership, vote-field publication and LeagueScreen dirty callback remain pending");
    return result;
}
} // namespace civ5::runtime
