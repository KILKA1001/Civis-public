#include "civ5/runtime/bnw_player_conversion_timer_turn.hpp"
#include "civ5/runtime/live_session.hpp"
#include <limits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
const saves::PlayerCoreField &unique_field(const saves::PlayerCoreObservation &core,
                                            std::string_view name) {
    const saves::PlayerCoreField *result = nullptr;
    for (const auto &candidate : core.fields) {
        if (candidate.name && name == candidate.name) {
            require(result == nullptr, "duplicate BNW player conversion field");
            result = &candidate;
        }
    }
    require(result != nullptr, "missing BNW player conversion field");
    require(result->values.size() == 1, "invalid BNW player conversion field width");
    return *result;
}
std::int32_t i32(const saves::PlayerCoreField &field) {
    require(field.values[0] >= std::numeric_limits<std::int32_t>::min() &&
            field.values[0] <= std::numeric_limits<std::int32_t>::max(),
            "BNW player conversion field exceeds i32");
    return static_cast<std::int32_t>(field.values[0]);
}
} // namespace

BnwConversionTimerTurnStep apply_bnw_conversion_timer_turn(
    LiveSessionState &state, std::int64_t player_id, std::int64_t active_player_id) {
    require(state.setup.profile == content::TechnologyProfile::bnw &&
            state.world.game_state.layout == saves::GameStateLayout::expansion2_version1 &&
            state.world.all_players.has_value() &&
            state.world.all_players->layout == saves::GameStateLayout::expansion2_version1,
            "BNW conversion turn requires decoded Expansion2 players");
    require(active_player_id >= -1 && active_player_id <
            static_cast<std::int64_t>(state.world.all_players->context.player_count),
            "BNW conversion turn requires a resolved current active player");
    const auto setup = state.setup.players.find(player_id);
    require(setup != state.setup.players.end() && setup->second.alive,
            "BNW conversion turn requires an alive restored player");

    saves::SavedPlayerObservation *selected = nullptr;
    for (auto &candidate : state.world.all_players->players) {
        if (candidate.saved_player_id == player_id) {
            require(selected == nullptr, "duplicate saved BNW player ID");
            selected = &candidate;
        }
    }
    require(selected != nullptr &&
            selected->core.layout == saves::GameStateLayout::expansion2_version1,
            "BNW conversion turn requires a matching saved player core");
    const auto &id = unique_field(selected->core, "m_eID");
    require(id.encoding == saves::PlayerCoreEncoding::i32 && i32(id) == player_id,
            "BNW conversion turn player ID mismatch");
    const auto &alive = unique_field(selected->core, "m_bAlive");
    require(alive.encoding == saves::PlayerCoreEncoding::boolean && i32(alive) == 1,
            "BNW conversion turn requires an alive saved player");
    const auto &timer = unique_field(selected->core, "m_iConversionTimer");
    require(timer.encoding == saves::PlayerCoreEncoding::i32,
            "invalid BNW conversion timer encoding");
    const auto before = i32(timer);
    require(before >= 0, "negative BNW conversion timer");

    // Expansion2 CvPlayer.cpp:4052-4053, 14484-14502. The setter emits the
    // engine dirty bit after the scalar changes, and only for active player.
    const auto after = before > 0 ? before - 1 : before;
    BnwConversionTimerTurnStep result{player_id, before, after, std::nullopt};
    if (after != before && player_id == active_player_id)
        result.post_commit_dirty = BnwConversionTimerDirtyIntent{player_id};
    for (auto &candidate : selected->core.fields) {
        if (candidate.name && std::string_view(candidate.name) == "m_iConversionTimer") {
            candidate.values[0] = after;
            break;
        }
    }
    return result;
}
} // namespace civ5::runtime
