#include "civ5/runtime/bnw_player_culture_bomb_turn.hpp"
#include "civ5/runtime/live_session.hpp"
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
saves::PlayerCoreField scalar(const char *name, saves::PlayerCoreEncoding encoding,
                              std::int64_t value) {
    saves::PlayerCoreField result{};
    result.name = name;
    result.encoding = encoding;
    result.values.push_back(value);
    return result;
}
runtime::LiveSessionState state_with_timer(std::int64_t timer) {
    saves::SavedSessionSnapshot snapshot;
    snapshot.game_state.layout = saves::GameStateLayout::expansion2_version1;
    snapshot.all_players.emplace();
    snapshot.all_players->layout = saves::GameStateLayout::expansion2_version1;
    saves::SavedPlayerObservation player;
    player.saved_player_id = 7;
    player.core.layout = saves::GameStateLayout::expansion2_version1;
    player.core.fields = {
        scalar("m_eID", saves::PlayerCoreEncoding::i32, 7),
        scalar("m_bAlive", saves::PlayerCoreEncoding::boolean, 1),
        scalar("m_iCultureBombTimer", saves::PlayerCoreEncoding::i32, timer),
        scalar("m_iConversionTimer", saves::PlayerCoreEncoding::i32, 2),
    };
    snapshot.all_players->players.push_back(std::move(player));
    runtime::LiveSessionState state{std::move(snapshot), runtime::ResearchContext{}, {},
        runtime::PlayerResearchContext{}, runtime::CityYieldContext{},
        runtime::BnwUnhappinessContext{}, runtime::MaintenanceContext{},
        runtime::TreasuryContext{}, runtime::ScienceContext{},
        runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})}};
    state.setup.profile = content::TechnologyProfile::bnw;
    state.setup.players[7].alive = true;
    return state;
}
std::int64_t timer(const runtime::LiveSessionState &state) {
    for (const auto &field : state.world.all_players->players[0].core.fields)
        if (field.name && std::string_view(field.name) == "m_iCultureBombTimer")
            return field.values.at(0);
    throw std::runtime_error("missing fixture timer");
}
template<class F> void rejects(F action, const char *message) {
    bool rejected = false;
    try { action(); } catch (const std::runtime_error &) { rejected = true; }
    check(rejected, message);
}
} // namespace

int main() {
    try {
        // Expansion2 CvPlayer::doTurn: positive culture-bomb timer changes
        // before conversion timer and after AI_doTurnPre. Setter has no
        // external callback; this fixture intentionally does not run a turn.
        for (const std::int64_t before : {0, 1, 3}) {
            auto state = state_with_timer(before);
            const auto result = runtime::apply_bnw_culture_bomb_timer_turn(state, 7);
            const auto expected = before > 0 ? before - 1 : before;
            check(result.player_id == 7 && result.before == before &&
                  result.after == expected && timer(state) == expected,
                  "culture-bomb timer differs from Expansion2 source branch");
            check(state.world.all_players->players[0].core.fields[3].values[0] == 2,
                  "later conversion-timer step ran prematurely");
        }
        {
            auto state = state_with_timer(3);
            rejects([&] { (void)runtime::apply_bnw_culture_bomb_timer_turn(state, 8); },
                    "unknown player accepted");
            check(timer(state) == 3, "unknown player changed timer");
            state.setup.players[7].alive = false;
            rejects([&] { (void)runtime::apply_bnw_culture_bomb_timer_turn(state, 7); },
                    "dead player accepted");
            check(timer(state) == 3, "dead player changed timer");
        }
        {
            auto state = state_with_timer(-1);
            rejects([&] { (void)runtime::apply_bnw_culture_bomb_timer_turn(state, 7); },
                    "negative timer accepted");
            check(timer(state) == -1, "invalid timer changed");
        }
        {
            auto state = state_with_timer(3);
            state.world.all_players->players[0].core.fields.push_back(
                scalar("m_iCultureBombTimer", saves::PlayerCoreEncoding::i32, 99));
            rejects([&] { (void)runtime::apply_bnw_culture_bomb_timer_turn(state, 7); },
                    "duplicate timer accepted");
            check(timer(state) == 3, "duplicate timer changed");
        }
        {
            auto state = state_with_timer(3);
            state.world.game_state.layout = saves::GameStateLayout::vp542_corpus;
            rejects([&] { (void)runtime::apply_bnw_culture_bomb_timer_turn(state, 7); },
                    "VP source accepted by BNW step");
            check(timer(state) == 3, "cross-profile rejection changed timer");
        }
        std::cout << "BNW culture-bomb turn scalar parity PASS\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
