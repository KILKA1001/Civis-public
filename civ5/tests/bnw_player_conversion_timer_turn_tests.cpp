#include "civ5/runtime/bnw_player_conversion_timer_turn.hpp"
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
runtime::LiveSessionState fixture(std::int64_t timer) {
    saves::SavedSessionSnapshot snapshot;
    snapshot.game_state.layout = saves::GameStateLayout::expansion2_version1;
    snapshot.all_players.emplace();
    snapshot.all_players->layout = saves::GameStateLayout::expansion2_version1;
    snapshot.all_players->context.player_count = 64;
    saves::SavedPlayerObservation player;
    player.saved_player_id = 7;
    player.core.layout = saves::GameStateLayout::expansion2_version1;
    player.core.fields = {
        scalar("m_eID", saves::PlayerCoreEncoding::i32, 7),
        scalar("m_bAlive", saves::PlayerCoreEncoding::boolean, 1),
        scalar("m_iCultureBombTimer", saves::PlayerCoreEncoding::i32, 3),
        scalar("m_iConversionTimer", saves::PlayerCoreEncoding::i32, timer),
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
        // Source parity: CvPlayer.cpp:4052-4053 and 14484-14502.
        for (const auto before : {0, 1, 3}) {
            for (const auto active : {7, 8, -1}) {
                auto state = fixture(before);
                const auto step = runtime::apply_bnw_conversion_timer_turn(state, 7, active);
                const auto expected = before > 0 ? before - 1 : before;
                check(step.before == before && step.after == expected &&
                      value(state, "m_iConversionTimer") == expected,
                      "conversion timer differs from Expansion2 branch");
                check(step.post_commit_dirty.has_value() == (before > 0 && active == 7),
                      "MiscButtons dirty intent differs from Expansion2 setter");
                if (step.post_commit_dirty)
                    check(step.post_commit_dirty->player_id == 7 &&
                          step.post_commit_dirty->bit ==
                              runtime::BnwConversionTimerDirtyBit::misc_buttons &&
                          step.post_commit_dirty->value,
                          "wrong engine dirty intent");
                check(value(state, "m_iCultureBombTimer") == 3,
                      "conversion step changed preceding timer");
            }
        }
        {
            auto committed = fixture(3);
            auto detached = committed;
            const auto staged = runtime::apply_bnw_conversion_timer_turn(detached, 7, 7);
            check(staged.post_commit_dirty.has_value() &&
                  value(detached, "m_iConversionTimer") == 2 &&
                  value(committed, "m_iConversionTimer") == 3,
                  "discarded detached turn leaked scalar or engine intent");
            // No publish is performed. Destroying the rejected draft also
            // discards its engine intent before it can reach the UI.
        }
        {
            auto state = fixture(-1);
            rejects([&] { (void)runtime::apply_bnw_conversion_timer_turn(state, 7, 7); },
                    "negative timer accepted");
            check(value(state, "m_iConversionTimer") == -1,
                  "rejected timer mutated state");
        }
        {
            auto state = fixture(3);
            rejects([&] { (void)runtime::apply_bnw_conversion_timer_turn(state, 7, 64); },
                    "invalid active-player ID accepted");
            check(value(state, "m_iConversionTimer") == 3,
                  "invalid active-player ID mutated state");
            state.world.all_players->players[0].core.fields.push_back(
                scalar("m_iConversionTimer", saves::PlayerCoreEncoding::i32, 99));
            rejects([&] { (void)runtime::apply_bnw_conversion_timer_turn(state, 7, 7); },
                    "duplicate timer accepted");
            check(value(state, "m_iConversionTimer") == 3,
                  "duplicate timer mutated state");
        }
        std::cout << "BNW conversion turn scalar and UI intent parity PASS\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
