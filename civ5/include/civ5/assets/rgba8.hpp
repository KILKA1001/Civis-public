#pragma once
#include "asset_source.hpp"
#include <array>

namespace civ5::assets {
// Explicit producer pixels, not a DDS decoder or a request to generate mips.
struct Rgba8Level {
    std::uint32_t width, height;
    Bytes rgba;
};
// Explicit square cube mip, faces +X,-X,+Y,-Y,+Z,-Z; no generated mip chain.
struct Rgba8CubeLevel {
    std::uint32_t size;
    std::array<Bytes, 6> faces;
};
} // namespace civ5::assets
