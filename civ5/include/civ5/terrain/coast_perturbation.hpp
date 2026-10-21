#pragma once
#include "coast_curves.hpp"

namespace civ5::terrain {
class TerrainRandom;
enum class CoastPowPath { legacy_x87, sse2 };
struct CoastPerturbation {
    std::vector<CoastControlPoint> points;
    unsigned levels, padded_point_count, random_calls;
    float initial_amplitude;
};
// Explicit native rand() values. No guessed seed, reset per contour or sharing
// with material PRNG state. Only the consumed prefix is validated/read.
CoastPerturbation perturb_coast(std::span<const CoastControlPoint> points, bool closed, Point2 wrap_offset,
                                std::span<const std::uint32_t> random_values, X87Precision precision,
                                CoastPowPath pow_path);
// Continues the explicitly supplied CRT state across contours. Entry state and
// any intervening consumers remain caller-owned; no automatic seed/reset.
CoastPerturbation perturb_coast(std::span<const CoastControlPoint> points, bool closed, Point2 wrap_offset,
                                TerrainRandom &random, X87Precision precision, CoastPowPath pow_path);
} // namespace civ5::terrain
