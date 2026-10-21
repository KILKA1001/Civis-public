#include "civ5/runtime/bnw_player_scalar_prefix.hpp"
#include "civ5/runtime/live_session.hpp"
#include <climits>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
saves::PlayerCoreField scalar(const char *name, saves::PlayerCoreEncoding encoding,
                              std::int64_t value) {
    saves::PlayerCoreField field{};
    field.name = name;
    field.encoding = encoding;
    field.values.push_back(value);
    return field;
}
runtime::LiveSessionState fixture(std::int64_t settled = 5) {
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
        scalar("m_iConversionTimer", saves::PlayerCoreEncoding::i32, 2),
        scalar("m_iTurnsSinceSettledLastCity", saves::PlayerCoreEncoding::i32, settled),
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
        if (field.name && name == field.name) return field.values.at(0);
    throw std::runtime_error("missing fixture field");
}
} // namespace

int main() {
    try {
        // Expansion2 CvPlayer.cpp:4049-4060. The four scalar operations
        // precede DoUpdateCramped and must retain this exact order.
        auto state = fixture();
        const auto result = runtime::apply_bnw_player_scalar_prefix(state, 7, 7);
        check(result.culture_bomb.before == 3 && result.culture_bomb.after == 2 &&
              result.conversion.before == 2 && result.conversion.after == 1 &&
              result.settled_city.before == 5 && result.settled_city.after == 6 &&
              result.conscript.before == 4 && result.conscript.after == 0,
              "BNW scalar prefix result differs from source order");
        check(value(state, "m_iCultureBombTimer") == 2 &&
              value(state, "m_iConversionTimer") == 1 &&
              value(state, "m_iTurnsSinceSettledLastCity") == 6 &&
              value(state, "m_iConscriptCount") == 0 &&
              result.conversion.post_commit_dirty.has_value(),
              "BNW scalar prefix state or deferred intent incorrect");
        {
            auto invalid = fixture(INT_MAX);
            bool rejected = false;
            try { (void)runtime::apply_bnw_player_scalar_prefix(invalid, 7, 7); }
            catch (const std::runtime_error &) { rejected = true; }
            check(rejected && value(invalid, "m_iCultureBombTimer") == 3 &&
                  value(invalid, "m_iConversionTimer") == 2 &&
                  value(invalid, "m_iTurnsSinceSettledLastCity") == INT_MAX &&
                  value(invalid, "m_iConscriptCount") == 4,
                  "later rejection leaked an earlier scalar mutation");
        }
        {
            auto inactive = fixture();
            const auto step = runtime::apply_bnw_player_scalar_prefix(inactive, 7, 8);
            check(!step.conversion.post_commit_dirty,
                  "inactive player emitted engine UI intent");
        }
        std::cout << "BNW player scalar prefix parity PASS\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
