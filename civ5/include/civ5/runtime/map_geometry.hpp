#pragma once
#include "civ5/saves/map_state.hpp"

namespace civ5::runtime {
// Source gameplay geometry over the same row-major grid used by the shared
// saved map. No allocation or original wire-coordinate width is required.
struct MapGeometry {
    saves::MapGrid grid;
    bool wrap_x=false,wrap_y=false;
    saves::GameStateLayout layout=saves::GameStateLayout::expansion2_version1;
    std::optional<std::int64_t> plot(std::int64_t x,std::int64_t y) const;
    std::optional<std::int64_t> plot_by_index(std::int64_t index) const;
    std::optional<std::int64_t> direction(std::int64_t x,std::int64_t y,std::int64_t direction) const;
    std::int64_t distance(std::int64_t x1,std::int64_t y1,std::int64_t x2,std::int64_t y2) const;
    void validate() const;
};
} // namespace civ5::runtime
