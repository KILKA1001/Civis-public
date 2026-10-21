#pragma once
#include "height_addressing.hpp"
#include "native_math.hpp"
#include <functional>

namespace civ5::terrain {
// The two streams returned by the original river geometry producer both have
// five float32 components. Height is in U16 units, not normalized world units.
struct RiverVertex {
    float x, y, height, profile, opacity;
};
struct RiverTriangle {
    RiverVertex a, b, c;
};
enum class RiverPass { banks, channels };
enum class RiverWriteChannel { height, material, visited };
using RiverWriteObserver =
    std::function<void(RiverPass, RiverWriteChannel, std::int32_t, std::uint32_t)>;
using RiverSampleObserver = std::function<void(RiverPass, std::size_t, std::int32_t, std::int32_t,
                                               const std::array<float, 3> &)>;
struct RiverRasterSettings {
    float level;
    X87Precision precision;
    std::array<std::uint8_t, 4> remap_from, remap_to;
    std::span<const std::uint32_t> material_codes;
    std::uint32_t river_code;
};
struct RiverRasterField {
    std::vector<std::uint16_t> heights;
    std::vector<std::uint8_t> materials, bank_visited;
};
using RiverPassComplete = std::function<void(RiverPass, const RiverRasterField &)>;

// Banks share a fresh first-visit mask; all channels follow all banks. X wraps
// by the original (x+100*width)%width and Y clips. Input fields are copied once.
// This consumes explicit triangles; river topology/jitter/stream generation is
// a separate upstream stage, and no saved-game PRNG state is inferred here.
RiverRasterField apply_river_triangles(HeightDomain domain, std::span<const std::uint16_t> heights,
                                       std::span<const std::uint8_t> materials,
                                       std::span<const RiverTriangle> banks,
                                       std::span<const RiverTriangle> channels,
                                       const RiverRasterSettings &settings,
                                       const RiverWriteObserver &write_observer = {},
                                       const RiverSampleObserver &sample_observer = {},
                                       const RiverPassComplete &pass_complete = {});
} // namespace civ5::terrain
