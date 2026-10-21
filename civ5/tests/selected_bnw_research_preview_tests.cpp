#include "civ5/ui/selected_bnw_research_preview.hpp"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
} // namespace

int main(int argc, char **argv) {
    if (argc != 3)
        throw std::runtime_error("Expected selected Gandhi save and installed game root");
    auto selected = civ5::ui::bootstrap_selected_bnw_preview(
        std::filesystem::path(argv[1]), std::filesystem::path(argv[2]), 0);
    if (!selected.preview) {
        const auto reason = selected.preview.blocker
            ? selected.preview.blocker->reason : "unknown bootstrap failure";
        throw std::runtime_error("Selected BNW fixture could not be restored: " + reason);
    }
    auto &host = *selected.preview.host;
    const auto &state = host.session().state();
    const auto queue_before = state.player_research.players.at(0).queue;
    const auto rng_before = state.random.gameplay.state().calls;
    const auto result = civ5::ui::preview_selected_bnw_research(host);
    check(result.player_id == 0 && result.team_id == state.setup.players.at(0).team &&
          result.turn == state.setup.game_turn &&
          result.blocker_code == civ5::ui::SelectedBnwResearchBlocker::stock_ui_contexts_unverified &&
          result.blocker == "Selected BNW observational Lua TestAll is unavailable" &&
          !result.native_candidates.empty(),
          "Selected Gandhi preview must report native research candidates and Lua blocker");
    for (const auto id : result.native_candidates)
        check(id >= 0 && host.technologies().find(
              static_cast<std::uint64_t>(id)) != nullptr,
              "Native candidate must come from selected BNW technology rules");
    check(state.player_research.players.at(0).queue == queue_before &&
          state.random.gameplay.state().calls == rng_before,
          "Read-only preview must not mutate research queue or gameplay RNG");

    host.session().mutable_state().setup.players.at(0).alive = false;
    const auto dead = civ5::ui::preview_selected_bnw_research(host);
    check(dead.native_candidates.empty() && !dead.blocker.empty(),
          "A stale active player must not retain selected research candidates");
}
