#include "civ5/runtime/bnw_player_conscript_turn.hpp"
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
runtime::LiveSessionState fixture(std::int64_t count) {
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
        scalar("m_iTurnsSinceSettledLastCity", saves::PlayerCoreEncoding::i32, 5),
        scalar("m_iConscriptCount", saves::PlayerCoreEncoding::i32, count),
        scalar("m_bCramped", saves::PlayerCoreEncoding::boolean, 1),
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
        // Expansion2 CvPlayer.cpp:4058 and 14195-14207: unconditional
        // assignment of zero, with no UI, Lua, or other GameCore callback.
        for (const auto before : {-1, 0, 3, INT_MAX}) {
            auto state = fixture(before);
            const auto step = runtime::apply_bnw_conscript_count_turn(state, 7);
            check(step.player_id == 7 && step.before == before && step.after == 0 &&
                  value(state, "m_iConscriptCount") == 0,
                  "conscript reset differs from Expansion2 setter");
            check(value(state, "m_iTurnsSinceSettledLastCity") == 5 &&
                  value(state, "m_bCramped") == 1,
                  "conscript step changed adjacent source-order effects");
        }
        {
            auto committed = fixture(3);
            auto detached = committed;
            (void)runtime::apply_bnw_conscript_count_turn(detached, 7);
            check(value(committed, "m_iConscriptCount") == 3 &&
                  value(detached, "m_iConscriptCount") == 0,
                  "discarded detached reset changed committed world");
        }
        {
            auto state = fixture(3);
            state.world.all_players->players[0].core.fields.push_back(
                scalar("m_iConscriptCount", saves::PlayerCoreEncoding::i32, 7));
            rejects([&] { (void)runtime::apply_bnw_conscript_count_turn(state, 7); },
                    "duplicate count accepted");
            check(value(state, "m_iConscriptCount") == 3,
                  "invalid count changed world");
        }
        std::cout << "BNW conscript turn scalar parity PASS\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
