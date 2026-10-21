#include "civ5/terrain/terrain_source_rivers.hpp"
#include <stdexcept>

namespace civ5::terrain {
namespace {
LogicalHex transform(LogicalHex v, const IntMatrix4 &m) {
    std::array<std::int32_t, 3> out{};
    for (unsigned i = 0; i < 3; ++i)
        out[i] = wrap_add(wrap_add(wrap_add(wrap_mul(v.x, m[i]), wrap_mul(v.z, m[8+i])),
                                  wrap_mul(v.y, m[4+i])), m[12+i]);
    return {out[0], out[1], out[2]};
}
LogicalHex cube(std::int32_t x, std::int32_t y) {
    const auto a = wrap_add(x, wrap_neg(y >> 1));
    return {a, y, wrap_neg(wrap_add(a, y))};
}
} // namespace
std::vector<RiverEdge> build_terrain_river_edges(const WorldGrid &grid,
    std::span<const TerrainRiverPlot> plots, std::uint64_t maximum_edges) {
    if (grid.width <= 0 || grid.height <= 0 ||
        std::uint64_t(grid.width) * grid.height != plots.size())
        throw std::invalid_argument("Invalid river source dimensions/count");
    std::vector<RiverEdge> result;
    const auto append = [&](LogicalHex p, std::int32_t direction) {
        if (result.size() >= maximum_edges)
            throw std::invalid_argument("River source exceeds edge budget");
        // Original signed IDIV after adding one period, even on non-wrapped maps.
        const auto offset = transform(p, grid.first_matrix);
        const auto x = wrap_add(wrap_add(offset.x, offset.y >> 1), grid.width) % grid.width;
        const auto y = wrap_add(offset.y, grid.height) % grid.height;
        if (x < 0 || y < 0)
            throw std::invalid_argument("River coordinate outside native normalization range");
        p = transform(cube(x, y), grid.second_matrix);
        result.push_back({p.x, p.y, wrap_neg(wrap_add(p.x, p.y)), direction});
    };
    for (std::int32_t x = 0; x < grid.width; ++x)
        for (std::int32_t y = 0; y < grid.height; ++y) {
            const auto &plot = plots[std::size_t(x) * grid.height + y];
            if (!plot.is_river) continue;
            auto p = transform(cube(x, y), grid.second_matrix);
            p.z = wrap_neg(wrap_add(p.x, p.y));
            if (plot.west)
                append({wrap_add(p.x, 1), p.y, wrap_add(p.z, -1)}, plot.flow_e == 0 ? 1 : 7);
            if (plot.northwest)
                append(p, plot.flow_se == 1 ? 8 : 2);
            if (plot.northeast)
                append({p.x, wrap_add(p.y, -1), wrap_add(p.z, 1)}, plot.flow_sw == 5 ? 6 : 0);
        }
    return result;
}
} // namespace civ5::terrain
