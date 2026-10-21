#include "civ5/terrain/terrain_auxiliary.hpp"
#include <bit>
#include <stdexcept>

namespace civ5::terrain {
TerrainAuxiliarySources build_terrain_auxiliary_sources(const WorldGrid &grid,
    std::span<const TerrainAuxiliaryPlot> plots, std::uint64_t maximum_cells) {
    if (grid.width <= 0 || grid.height <= 0 ||
        std::uint64_t(grid.width) * grid.height != plots.size() || plots.size() > maximum_cells)
        throw std::invalid_argument("Invalid auxiliary source dimensions or cell budget");
    TerrainAuxiliarySources result;
    const auto &m = grid.second_matrix;
    for (std::int32_t x = 0; x < grid.width; ++x)
        for (std::int32_t y = 0; y < grid.height; ++y) {
            const auto &plot = plots[std::size_t(x) * grid.height + y];
            const auto a = wrap_add(x, wrap_neg(y >> 1)), c = wrap_neg(wrap_add(a, y));
            const auto coordinate = [&](unsigned i) {
                return wrap_add(wrap_add(wrap_add(wrap_mul(c, m[8+i]), wrap_mul(a, m[i])),
                                         wrap_mul(y, m[4+i])), m[12+i]);
            };
            LogicalHex position{coordinate(0), coordinate(1), 0};
            position.z = wrap_neg(wrap_add(position.x, position.y));
            if (plot.terrain == 5 || plot.terrain == 6 || plot.plot == 0)
                result.relief.push_back({
                    std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(position.x)),
                    std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(position.y)),
                    static_cast<std::uint8_t>(plot.terrain == 5 || plot.terrain == 6 ? 0 : 2)});
            if (plot.feature == 2)
                result.feature2.push_back({position, plot.terrain,
                    std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(plot.continent))});
            if (plot.feature == 0) result.feature0.push_back(position);
            if (plot.feature == 4) result.feature4.push_back(position);
        }
    return result;
}
} // namespace civ5::terrain
