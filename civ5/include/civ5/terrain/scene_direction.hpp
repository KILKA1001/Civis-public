#pragma once
#include "native_math.hpp"

namespace civ5::terrain {
// Original 9AE6C0. SSE terms retain their Float32 rounding; the final x87
// sum returns at the requested precision without an extra Float32 spill.
// Only bounded coordinates with nonnegative lattice indices are admitted:
// original signed remainder would otherwise read before the permutation table.
double scene_direction_noise(Point2 point, X87Precision precision);
// Angle-producing tail of original 5D5710. The preceding owner's allocation
// and 100 random draws are separate; this function does not advance an RNG.
float initial_scene_direction_angle(X87Precision precision);

struct SceneDirectionField {
    // Original owner +4068/+406C/+4070. These are scene inputs, not seeds
    // inferred from the independent terrain-generation random cursors.
    float angle;
    Point2 offset;
    // Original 685120: two spatial noise scales, the first weighted value
    // spilled to Float32, then the second, owner angle, FCOS/FSIN and radius 64.
    Point2 direction(Point2 point, X87Precision precision) const;
};
} // namespace civ5::terrain
