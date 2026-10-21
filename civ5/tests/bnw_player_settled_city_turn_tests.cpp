#include "civ5/runtime/bnw_player_settled_city_turn.hpp"
#include "civ5/runtime/live_session.hpp"
#include <climits>
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
runtime::LiveSessionState fixture(std::int64_t counter) {
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
        scalar("m_iCultureBombTimer", saves::PlayerCoreEncoding::i32, 2),
        scalar("m_iConversionTimer", saves::PlayerCoreEncoding::i32, 2),
        scalar("m_iTurnsSinceSettledLastCity", saves::PlayerCoreEncoding::i32, counter),
        scalar("m_iConscriptCount", saves::PlayerCoreEncoding::i32, 4),
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
std::int64_t value(const runtime::LiveSessionState &state, std::string_view name) {
    for (const auto &field : state.world.all_players->players[0].core.fields)
        if (field.name && std::string_view(field.name) == name)
            return field.values.at(0);
    throw std::runtime_error("missing fixture field");
}
template<class F> void rejects(F action, const char *message) {
    bool rejected = false;
    try { action(); } catch (const std::runtime_error &) { rejected = true; }
    check(rejected, message);
}
} // namespace

int main() {
    try {
        // Expansion2 CvPlayer.cpp:4055-4056, 23596-23614: only a
        // nonnegative counter increments. The setter has no external effects.
        for (const auto before : {-2, -1, 0, 1, 9}) {
            auto state = fixture(before);
            const auto step = runtime::apply_bnw_turns_since_settled_city_turn(state, 7);
            const auto expected = before >= 0 ? before + 1 : before;
            check(step.before == before && step.after == expected &&
                  value(state, "m_iTurnsSinceSettledLastCity") == expected,
                  "settled-city counter differs from Expansion2 branch");
            check(value(state, "m_iCultureBombTimer") == 2 &&
                  value(state, "m_iConversionTimer") == 2 &&
                  value(state, "m_iConscriptCount") == 4,
                  "settled-city step changed another source-order field");
        }
        {
            auto committed = fixture(5);
            auto detached = committed;
            (void)runtime::apply_bnw_turns_since_settled_city_turn(detached, 7);
            check(value(committed, "m_iTurnsSinceSettledLastCity") == 5 &&
                  value(detached, "m_iTurnsSinceSettledLastCity") == 6,
                  "discarded detached turn changed committed world");
        }
        {
            auto state = fixture(INT_MAX);
            rejects([&] { (void)runtime::apply_bnw_turns_since_settled_city_turn(state, 7); },
                    "overflowing counter accepted");
            check(value(state, "m_iTurnsSinceSettledLastCity") == INT_MAX,
                  "overflow rejection changed world");
        }
        {
            auto state = fixture(5);
            state.world.all_players->players[0].core.fields.push_back(
                scalar("m_iTurnsSinceSettledLastCity", saves::PlayerCoreEncoding::i32, 99));
            rejects([&] { (void)runtime::apply_bnw_turns_since_settled_city_turn(state, 7); },
                    "duplicate counter accepted");
            check(value(state, "m_iTurnsSinceSettledLastCity") == 5,
                  "duplicate rejection changed world");
        }
        std::cout << "BNW settled-city turn counter parity PASS\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
