#include "civ5/runtime/bnw_player_settled_city_turn.hpp"
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
            require(result == nullptr, "duplicate BNW settled-city turn field");
            result = &candidate;
        }
    }
    require(result != nullptr, "missing BNW settled-city turn field");
    require(result->values.size() == 1, "invalid BNW settled-city turn field width");
    return *result;
}
std::int32_t checked_i32(const saves::PlayerCoreField &field) {
    require(field.values[0] >= std::numeric_limits<std::int32_t>::min() &&
            field.values[0] <= std::numeric_limits<std::int32_t>::max(),
            "BNW settled-city turn field exceeds i32");
    return static_cast<std::int32_t>(field.values[0]);
}
} // namespace

BnwSettledCityTurnStep apply_bnw_turns_since_settled_city_turn(
    LiveSessionState &state, std::int64_t player_id) {
    require(state.setup.profile == content::TechnologyProfile::bnw &&
            state.world.game_state.layout == saves::GameStateLayout::expansion2_version1 &&
            state.world.all_players.has_value() &&
            state.world.all_players->layout == saves::GameStateLayout::expansion2_version1,
            "BNW settled-city turn requires decoded Expansion2 players");
    const auto setup = state.setup.players.find(player_id);
    require(setup != state.setup.players.end() && setup->second.alive,
            "BNW settled-city turn requires an alive restored player");

    saves::SavedPlayerObservation *selected = nullptr;
    for (auto &candidate : state.world.all_players->players) {
        if (candidate.saved_player_id == player_id) {
            require(selected == nullptr, "duplicate saved BNW player ID");
            selected = &candidate;
        }
    }
    require(selected != nullptr &&
            selected->core.layout == saves::GameStateLayout::expansion2_version1,
            "BNW settled-city turn requires a matching saved player core");
    const auto &id = unique_field(selected->core, "m_eID");
    require(id.encoding == saves::PlayerCoreEncoding::i32 && checked_i32(id) == player_id,
            "BNW settled-city turn player ID mismatch");
    const auto &alive = unique_field(selected->core, "m_bAlive");
    require(alive.encoding == saves::PlayerCoreEncoding::boolean && checked_i32(alive) == 1,
            "BNW settled-city turn requires an alive saved player");
    const auto &counter = unique_field(selected->core, "m_iTurnsSinceSettledLastCity");
    require(counter.encoding == saves::PlayerCoreEncoding::i32,
            "invalid BNW settled-city turn encoding");
    const auto before = checked_i32(counter);
    // The SDK adds one to a signed int only for nonnegative values. Refuse
    // INT_MAX because an overflowing source add has no defined C++ result.
    require(before < std::numeric_limits<std::int32_t>::max(),
            "BNW settled-city turn counter would overflow");
    const auto after = before >= 0 ? before + 1 : before;
    for (auto &candidate : selected->core.fields) {
        if (candidate.name && std::string_view(candidate.name) ==
                                  "m_iTurnsSinceSettledLastCity") {
            candidate.values[0] = after;
            break;
        }
    }
    return {player_id, before, after};
}
} // namespace civ5::runtime
