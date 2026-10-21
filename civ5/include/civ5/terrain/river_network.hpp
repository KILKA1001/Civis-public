#pragma once
#include "native_math.hpp"
#include <compare>
#include <optional>
#include <span>
#include <vector>

namespace civ5::terrain {
struct RiverEdge {
    std::int32_t x, y, z, direction;
    auto operator<=>(const RiverEdge &) const = default;
};
struct RiverChain {
    // Original traversal order and unwrapped cube coordinates.
    std::vector<RiverEdge> edges;
    bool terminal;
};
// Original 200x200 direction owner and 0x760E90 traversal. Dimensions belong
// to the source map, before the worker field's border expansion. The supported
// global factory is the same verified zero descriptor as build_world_layout.
class RiverNetwork {
  public:
    static RiverNetwork build(std::int32_t map_width, std::int32_t map_height, std::span<const RiverEdge> edges);
    std::int32_t width() const { return width_; }
    std::int32_t height() const { return height_; }
    std::span<const std::uint32_t> direction_grid() const { return directions_; }
    const std::vector<RiverChain> &chains() const { return chains_; }
    RiverEdge normalized(RiverEdge edge) const;
    RiverEdge canonical(RiverEdge edge) const;
    RiverEdge opposite(RiverEdge edge) const;
    bool contains(RiverEdge edge) const;
    std::optional<RiverEdge> next(RiverEdge edge) const;

  private:
    RiverNetwork(std::int32_t width, std::int32_t height);
    std::optional<RiverEdge> previous(RiverEdge edge) const;
    std::int32_t width_, height_;
    std::vector<std::uint32_t> directions_;
    std::vector<RiverChain> chains_;
};
} // namespace civ5::terrain
