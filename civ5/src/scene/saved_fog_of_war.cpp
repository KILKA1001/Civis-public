#include "civ5/scene/saved_fog_of_war.hpp"
#include <array>
#include <stdexcept>
#include <vector>

namespace civ5::scene {
namespace {
struct VisibilityEvent {
    std::array<std::int32_t, 3> cube;
    std::uint8_t type;
};
} // namespace
SavedFogOfWarCounts apply_saved_map_fog(const saves::MapGrid &grid,
                                       const saves::SavedMapTeamArtObservation &art,
                                       materials::FogOfWarState &state) {
    const auto count = grid.size();
    if (grid.width != static_cast<std::uint64_t>(state.width()) ||
        grid.height != static_cast<std::uint64_t>(state.height()) || art.plots.size() != count)
        throw std::invalid_argument("Saved map and FOW grid dimensions differ");
    const auto offset = state.grid_offset();
    SavedFogOfWarCounts result;
    std::vector<VisibilityEvent> events;
    events.reserve(art.plots.size());
    for (std::uint64_t i = 0; i < count; ++i) {
        const auto &plot = art.plots[static_cast<std::size_t>(i)];
        if (plot.plot_index != i || plot.visibility_count < 0 || !plot.revealed)
            throw std::invalid_argument("Saved plot FOW input is missing or out of order");
        const auto [x, y] = grid.coordinates(i);
        const auto cube_x = static_cast<std::int64_t>(offset[0]) + static_cast<std::int64_t>(x) -
                            static_cast<std::int64_t>(y / 2);
        const auto cube_y = static_cast<std::int64_t>(offset[1]) + static_cast<std::int64_t>(y);
        if (cube_x < -32768 || cube_x > 32767 || cube_y < -32768 || cube_y > 32767)
            throw std::invalid_argument("Saved plot exceeds FOW cube-coordinate domain");
        // CvPlot::GetActiveFogOfWarMode checks reveal before visibility.
        const auto type = !*plot.revealed ? std::uint8_t{0}
                          : plot.visibility_count > 0 ? std::uint8_t{2}
                                                      : std::uint8_t{1};
        result.unexplored += type == 0;
        result.explored += type == 1;
        result.visible += type == 2;
        result.visible_unrevealed += plot.visibility_count > 0 && !*plot.revealed;
        events.push_back({{static_cast<std::int32_t>(cube_x), static_cast<std::int32_t>(cube_y),
                           static_cast<std::int32_t>(-cube_x - cube_y)}, type});
    }
    // All entries are checked before the first event mutates the retained owner.
    for (const auto &event : events)
        state.visibility_event(event.cube, event.type);
    return result;
}
} // namespace civ5::scene
