#include "civ5/runtime/bnw_fresh_player_reset_prefix.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
std::optional<std::int32_t> native_constant(
    std::span<const content::GameplayDefine> constants, std::string_view name) {
    std::optional<std::int32_t> result;
    for (const auto &constant : constants) {
        if (constant.name != name) continue;
        if (result || constant.value <= 0 ||
            constant.value > std::numeric_limits<std::int32_t>::max())
            return std::nullopt;
        result = static_cast<std::int32_t>(constant.value);
    }
    return result;
}
void block(BnwFreshPlayerResetPrefix &result,
           BnwFreshPlayerResetBlockerCode code, std::string message) {
    result.blockers.push_back({code, std::move(message)});
}
void append(BnwFreshPlayerResetPrefix &result, BnwFreshPlayerResetStep step,
            std::int32_t player) {
    result.source_order.push_back({step,player});
}
} // namespace

bool BnwFreshPlayerResetPrefix::has_blocker(
    BnwFreshPlayerResetBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshPlayerResetPrefix::rollback() noexcept {
    players.clear();
    source_order.clear();
    early_arrays_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshPlayerResetPrefix draft_bnw_fresh_player_early_reset(
    const BnwFreshGameContentResult &selected,
    const BnwFreshPregameRosterDraft &roster,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants,
    std::span<const std::int32_t> pregame_leader_ids) {
    BnwFreshPlayerResetPrefix result;
    const auto max_players = native_constant(native_constants,"MAX_PLAYERS");
    const auto max_major = native_constant(native_constants,"MAX_MAJOR_CIVS");
    const auto num_yields = native_constant(native_constants,"NUM_YIELD_TYPES");
    if (!selected.content_ready() || !selected.activation || !selected.rules ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        !roster.slot_prefix_complete || roster.initialized_by_gamecore ||
        selected_rules_sha256.empty() ||
        roster.selected_rules_sha256 != selected_rules_sha256 ||
        !max_players || !max_major || !num_yields ||
        *max_players > 4096 || *max_major > *max_players ||
        *num_yields != 6 ||
        roster.slots.size() != static_cast<std::size_t>(*max_players) ||
        pregame_leader_ids.size() != roster.slots.size()) {
        block(result,
              BnwFreshPlayerResetBlockerCode::roster_rules_or_native_constants_unverified,
              "Selected BNW roster, MAX_PLAYERS and compiled six YieldTypes are required");
        return result;
    }
    result.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    if (result.selected_rules_sha256 != selected_rules_sha256) {
        block(result,
              BnwFreshPlayerResetBlockerCode::roster_rules_or_native_constants_unverified,
              "Player reset and pregame roster must use the same selected rules");
        return result;
    }
    try {
        std::vector<std::int32_t> available_leaders;
        const auto leaders = selected.rules->prefetch_info("Leaders",
            [&](std::uint64_t, std::int64_t id, const content::RulesRowView &) {
                if (id < 0 || id > std::numeric_limits<std::int32_t>::max())
                    throw std::invalid_argument("Selected Leader ID is invalid");
                available_leaders.push_back(static_cast<std::int32_t>(id));
                return true;
            }, {1024 * 1024});
        if (!leaders.all_succeeded())
            throw std::invalid_argument("Selected Leaders Info is incomplete");
        std::sort(available_leaders.begin(),available_leaders.end());
        for (const auto leader : pregame_leader_ids) {
            if (leader < -1 ||
                (leader != -1 && !std::binary_search(
                    available_leaders.begin(),available_leaders.end(),leader)))
                throw std::invalid_argument("Pregame leader is not in selected Info");
        }
        result.players.reserve(pregame_leader_ids.size());
        result.source_order.reserve(pregame_leader_ids.size() * 9);
        for (std::size_t index = 0; index < pregame_leader_ids.size(); ++index) {
            const auto id = static_cast<std::int32_t>(index);
            BnwFreshPlayerEarlyReset player;
            if (id < *max_major)
                append(result,BnwFreshPlayerResetStep::major_notifications_pending,id);
            append(result,BnwFreshPlayerResetStep::sync_archive_reset,id);
            append(result,BnwFreshPlayerResetStep::player_uninit_pending,id);
            player.id = id;
            player.personality = pregame_leader_ids[index];
            append(result,BnwFreshPlayerResetStep::id_and_personality_write,id);
            player.ever_popped_goody = false;
            append(result,BnwFreshPlayerResetStep::tutorial_reset,id);
            for (auto &yields : player.yield_vectors) yields.assign(6,0);
            append(result,BnwFreshPlayerResetStep::yield_vectors_reset,id);
            player.proximity_to_player.assign(*max_players,0);
            player.research_agreement_counter.assign(*max_players,0);
            player.incoming_unit_types.assign(*max_players,-1);
            player.incoming_unit_countdowns.assign(*max_players,-1);
            player.minor_friendship_anchors.assign(*max_players,0);
            player.siphon_luxury_count.assign(*max_players,0);
            append(result,BnwFreshPlayerResetStep::player_vectors_reset,id);
            player.options.clear();
            player.religion_key.clear();
            player.script_data.clear();
            player.embarked_graphic_override.clear();
            append(result,BnwFreshPlayerResetStep::options_and_strings_clear,id);
            append(result,BnwFreshPlayerResetStep::resource_info_arrays_pending,id);
            result.players.push_back(std::move(player));
        }
    } catch (const std::exception &) {
        result.rollback();
        block(result,BnwFreshPlayerResetBlockerCode::pregame_leader_info_unverified,
              "Complete CvPreGame leaderHead IDs and selected Leaders Info are required");
        return result;
    }
    result.early_arrays_complete = true;
    block(result,BnwFreshPlayerResetBlockerCode::notifications_and_uninit_pending,
          "CvNotifications/CvDiplomacyRequests and CvPlayer::uninit precede these detached reset arrays");
    block(result,BnwFreshPlayerResetBlockerCode::resource_info_and_remaining_reset_pending,
          "Selected Resource/Improvement and remaining CvPlayer reset arrays, traits and AI effects remain");
    return result;
}
} // namespace civ5::runtime
