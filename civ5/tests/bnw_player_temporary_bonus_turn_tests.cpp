#include "civ5/runtime/bnw_player_temporary_bonus_turn.hpp"
#include "civ5/runtime/live_session.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
saves::PlayerCoreField scalar(const char *name,
                              saves::PlayerCoreEncoding encoding,
                              std::int64_t value) {
    saves::PlayerCoreField result{};
    result.name = name;
    result.encoding = encoding;
    result.values = {value};
    return result;
}
runtime::LiveSessionState fixture(std::array<std::int64_t, 3> timers) {
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
        scalar("m_iAttackBonusTurns", saves::PlayerCoreEncoding::i32, timers[0]),
        scalar("m_iCultureBonusTurns", saves::PlayerCoreEncoding::i32, timers[1]),
        scalar("m_iTourismBonusTurns", saves::PlayerCoreEncoding::i32, timers[2]),
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
std::array<std::int64_t, 3> durations(const runtime::LiveSessionState &state) {
    std::array<std::int64_t, 3> result{};
    constexpr std::array<std::string_view, 3> names{
        "m_iAttackBonusTurns", "m_iCultureBonusTurns", "m_iTourismBonusTurns"};
    for (const auto &field : state.world.all_players->players[0].core.fields)
        for (std::size_t i = 0; i < names.size(); ++i)
            if (field.name && std::string_view(field.name) == names[i])
                result[i] = field.values.at(0);
    return result;
}
template<class F> void rejects(F action, const char *message) {
    bool rejected = false;
    try { action(); } catch (const std::runtime_error &) { rejected = true; }
    check(rejected, message);
}
} // namespace

int main() {
    try {
        // Expansion2 CvPlayer.cpp:4153-4164, then DoProcessGoldenAge.
        auto state = fixture({3, 1, 0});
        const auto step = runtime::apply_bnw_temporary_bonus_turn(state, 7);
        check(step.player_id == 7 && step.before == std::array<std::int32_t, 3>{3, 1, 0} &&
              step.after == std::array<std::int32_t, 3>{2, 0, 0} &&
              durations(state) == std::array<std::int64_t, 3>{2, 0, 0},
              "BNW post-diplomacy duration order differs");
        auto committed = fixture({3, 1, 0});
        auto draft = committed;
        (void)runtime::apply_bnw_temporary_bonus_turn(draft, 7);
        check(durations(committed) == std::array<std::int64_t, 3>{3, 1, 0} &&
              durations(draft) == std::array<std::int64_t, 3>{2, 0, 0},
              "discarded BNW turn changed committed durations");
        for (std::size_t i = 0; i < 3; ++i) {
            auto invalid = fixture({3, 1, 2});
            invalid.world.all_players->players[0].core.fields[i + 2].values[0] = -1;
            const auto before = durations(invalid);
            rejects([&] { (void)runtime::apply_bnw_temporary_bonus_turn(invalid, 7); },
                    "negative BNW boost duration accepted");
            check(durations(invalid) == before, "failed boost validation mutated draft");
        }
        auto duplicate = fixture({3, 1, 2});
        duplicate.world.all_players->players[0].core.fields.push_back(
            scalar("m_iTourismBonusTurns", saves::PlayerCoreEncoding::i32, 5));
        rejects([&] { (void)runtime::apply_bnw_temporary_bonus_turn(duplicate, 7); },
                "duplicate BNW boost field accepted");
        check(durations(duplicate)[0] == 3, "duplicate field rejection mutated draft");
        std::cout << "BNW post-diplomacy temporary durations PASS\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
