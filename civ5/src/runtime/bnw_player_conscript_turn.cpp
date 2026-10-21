#include "civ5/runtime/bnw_player_conscript_turn.hpp"
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
            require(result == nullptr, "duplicate BNW conscript field");
            result = &candidate;
        }
    }
    require(result != nullptr, "missing BNW conscript field");
    require(result->values.size() == 1, "invalid BNW conscript field width");
    return *result;
}
std::int32_t i32(const saves::PlayerCoreField &field) {
    require(field.values[0] >= std::numeric_limits<std::int32_t>::min() &&
            field.values[0] <= std::numeric_limits<std::int32_t>::max(),
            "BNW conscript field exceeds i32");
    return static_cast<std::int32_t>(field.values[0]);
}
} // namespace

BnwConscriptTurnStep apply_bnw_conscript_count_turn(LiveSessionState &state,
                                                    std::int64_t player_id) {
    require(state.setup.profile == content::TechnologyProfile::bnw &&
            state.world.game_state.layout == saves::GameStateLayout::expansion2_version1 &&
            state.world.all_players.has_value() &&
            state.world.all_players->layout == saves::GameStateLayout::expansion2_version1,
            "BNW conscript turn requires decoded Expansion2 players");
    const auto setup = state.setup.players.find(player_id);
    require(setup != state.setup.players.end() && setup->second.alive,
            "BNW conscript turn requires an alive restored player");

    saves::SavedPlayerObservation *selected = nullptr;
    for (auto &candidate : state.world.all_players->players) {
        if (candidate.saved_player_id == player_id) {
            require(selected == nullptr, "duplicate saved BNW player ID");
            selected = &candidate;
        }
    }
    require(selected != nullptr &&
            selected->core.layout == saves::GameStateLayout::expansion2_version1,
            "BNW conscript turn requires a matching saved player core");
    const auto &id = unique_field(selected->core, "m_eID");
    require(id.encoding == saves::PlayerCoreEncoding::i32 && i32(id) == player_id,
            "BNW conscript turn player ID mismatch");
    const auto &alive = unique_field(selected->core, "m_bAlive");
    require(alive.encoding == saves::PlayerCoreEncoding::boolean && i32(alive) == 1,
            "BNW conscript turn requires an alive saved player");
    const auto &count = unique_field(selected->core, "m_iConscriptCount");
    require(count.encoding == saves::PlayerCoreEncoding::i32,
            "invalid BNW conscript count encoding");
    const auto before = i32(count);

    // Expansion2 CvPlayer.cpp:4058, 14203-14207: setter unconditionally
    // assigns zero and only asserts that the new value is nonnegative.
    for (auto &candidate : selected->core.fields) {
        if (candidate.name && std::string_view(candidate.name) == "m_iConscriptCount") {
            candidate.values[0] = 0;
            break;
        }
    }
    return {player_id, before, 0};
}
} // namespace civ5::runtime
