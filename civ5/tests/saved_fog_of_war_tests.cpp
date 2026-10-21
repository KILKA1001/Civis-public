#include "civ5/scene/saved_fog_of_war.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace civ5;
namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
template <class Action> void rejects_without_mutation(materials::FogOfWarState &state, Action action) {
    const auto before = state.visibility();
    bool rejected = false;
    try { action(); }
    catch (const std::invalid_argument &) { rejected = true; }
    check(rejected && state.visibility() == before, "Invalid saved map partially mutated FOW");
}
} // namespace
int main() {
    const saves::MapGrid grid{4, 3};
    saves::SavedMapTeamArtObservation art;
    for (std::uint64_t i = 0; i < grid.size(); ++i)
        art.plots.push_back({i, 0, false, {}, {}, -1});
    art.plots[0].revealed = true;
    art.plots[0].visibility_count = 2; // northwest visible
    art.plots[3].revealed = true;      // northeast explored
    art.plots[8].revealed = true;
    art.plots[8].visibility_count = 1; // southwest visible; catches Y reversal
    art.plots[5].visibility_count = 1; // reveal wins over contradictory visibility
    materials::FogOfWarState state(4, 3, {2, -1, -1}, 0.f, 1000.f);
    const auto counts = scene::apply_saved_map_fog(grid, art, state);
    check(counts == scene::SavedFogOfWarCounts{2, 1, 9, 1}, "Saved FOW modes differ");
    const auto &cells = state.visibility(); // native x*height+y storage
    check(cells.size() == 12 && cells[0] == 0 && cells[9] == 1 && cells[2] == 0 &&
              cells[1] == 2 && cells[4] == 2 && cells[11] == 2,
          "Saved logical grid was flipped or transposed in FOW owner");
    state.tick(0.f);
    check(!state.dirty(), "Saved FOW replay was not published on tick");

    auto invalid = art;
    invalid.plots[11].plot_index = 10;
    rejects_without_mutation(state, [&] { scene::apply_saved_map_fog(grid, invalid, state); });
    invalid = art;
    invalid.plots[11].revealed.reset();
    rejects_without_mutation(state, [&] { scene::apply_saved_map_fog(grid, invalid, state); });
    invalid = art;
    invalid.plots[11].visibility_count = -1;
    rejects_without_mutation(state, [&] { scene::apply_saved_map_fog(grid, invalid, state); });
    rejects_without_mutation(state, [&] { scene::apply_saved_map_fog({3, 4}, art, state); });
    materials::FogOfWarState outside(4, 3, {32767, 0, -32767}, 0.f, 1000.f);
    rejects_without_mutation(outside, [&] { scene::apply_saved_map_fog(grid, art, outside); });
    std::cout << "PASS saved FOW owner orientation and validation\n";
    return 0;
}
