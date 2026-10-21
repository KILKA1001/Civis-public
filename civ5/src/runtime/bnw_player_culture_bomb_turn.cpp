#include "civ5/runtime/bnw_player_culture_bomb_turn.hpp"
#include "civ5/runtime/live_session.hpp"
#include <limits>
#include <utility>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
const saves::PlayerCoreField &field(const saves::PlayerCoreObservation &core,
                                    std::string_view name) {
    const saves::PlayerCoreField *found = nullptr;
    for (const auto &candidate : core.fields) {
        if (candidate.name && name == candidate.name) {
            require(found == nullptr, "duplicate BNW player turn scalar");
            found = &candidate;
        }
    }
    require(found != nullptr, "missing BNW player turn scalar");
    require(found->values.size() == 1, "invalid BNW player turn scalar width");
    return *found;
}
std::int32_t checked_i32(const saves::PlayerCoreField &value, const char *message) {
    require(value.encoding == saves::PlayerCoreEncoding::i32 ||
            value.encoding == saves::PlayerCoreEncoding::boolean, message);
    require(value.values[0] >= std::numeric_limits<std::int32_t>::min() &&
            value.values[0] <= std::numeric_limits<std::int32_t>::max(), message);
    return static_cast<std::int32_t>(value.values[0]);
}
} // namespace

void initialize_bnw_player_read_transients(LiveSessionState &state) {
    require(state.setup.profile == content::TechnologyProfile::bnw &&
            state.world.game_state.layout == saves::GameStateLayout::expansion2_version1 &&
            state.world.all_players &&
            state.world.all_players->layout == saves::GameStateLayout::expansion2_version1,
            "BNW player read transients require decoded Expansion2 players");
    require(state.player_culture_bomb_timers.empty(),
            "BNW player read transients already initialized");
    std::map<std::int64_t,std::int32_t> restored;
    for(const auto &player:state.world.all_players->players) {
        require(player.core.layout==saves::GameStateLayout::expansion2_version1,
                "BNW player read transient layout differs");
        require(player.saved_player_id>=0,
                "BNW player read transient ID invalid");
        for(const auto &entry:player.core.fields)
            require(!entry.name || std::string_view(entry.name)!="m_iCultureBombTimer",
                    "BNW wire unexpectedly contains culture bomb timer");
        require(restored.emplace(player.saved_player_id,0).second,
                "Duplicate BNW player read transient ID");
    }
    state.player_culture_bomb_timers=std::move(restored);
}

BnwCultureBombTurnStep apply_bnw_culture_bomb_timer_turn(LiveSessionState &state,
                                                          std::int64_t player_id) {
    require(state.setup.profile == content::TechnologyProfile::bnw &&
            state.world.game_state.layout == saves::GameStateLayout::expansion2_version1,
            "BNW player turn requires the selected Expansion2 profile");
    require(state.world.all_players.has_value() &&
            state.world.all_players->layout == saves::GameStateLayout::expansion2_version1,
            "BNW player turn requires decoded Expansion2 players");
    const auto setup_it = state.setup.players.find(player_id);
    require(setup_it != state.setup.players.end() && setup_it->second.alive,
            "BNW player turn requires an alive restored player");

    saves::SavedPlayerObservation *selected = nullptr;
    for (auto &candidate : state.world.all_players->players) {
        if (candidate.saved_player_id == player_id) {
            require(selected == nullptr, "duplicate saved BNW player ID");
            selected = &candidate;
        }
    }
    require(selected != nullptr &&
            selected->core.layout == saves::GameStateLayout::expansion2_version1,
            "BNW player turn requires a matching saved player core");
    const auto &alive = field(selected->core, "m_bAlive");
    require(alive.encoding == saves::PlayerCoreEncoding::boolean &&
            checked_i32(alive, "invalid saved BNW player alive flag") == 1,
            "BNW player turn requires an alive saved player");
    const auto &id = field(selected->core, "m_eID");
    require(id.encoding == saves::PlayerCoreEncoding::i32 &&
            checked_i32(id, "invalid saved BNW player ID") == player_id,
            "BNW player turn ID mismatch");
    auto transient=state.player_culture_bomb_timers.find(player_id);
    std::int32_t before=0;
    if(transient!=state.player_culture_bomb_timers.end()) {
        for(const auto &entry:selected->core.fields)
            require(!entry.name || std::string_view(entry.name)!="m_iCultureBombTimer",
                    "BNW culture bomb timer has conflicting wire and reset values");
        before=transient->second;
    } else {
        // Legacy detached test fixtures can still supply an explicit scalar.
        // Normal imports always populate the reset-only value above.
        const auto &timer=field(selected->core,"m_iCultureBombTimer");
        require(timer.encoding==saves::PlayerCoreEncoding::i32,
                "invalid BNW culture bomb timer encoding");
        before=checked_i32(timer,"invalid BNW culture bomb timer value");
    }
    require(before >= 0, "negative BNW culture bomb timer");

    // Expansion2 CvPlayer.cpp:4049-4050, 14459-14473: decrement only when
    // positive; the setter changes this scalar and asserts nonnegative.
    const auto after = before > 0 ? before - 1 : before;
    if(transient!=state.player_culture_bomb_timers.end())
        transient->second=after;
    else
        for (auto &candidate : selected->core.fields) {
            if (candidate.name && std::string_view(candidate.name) == "m_iCultureBombTimer") {
                candidate.values[0] = after;
                break;
            }
        }
    return {player_id, before, after};
}
} // namespace civ5::runtime
