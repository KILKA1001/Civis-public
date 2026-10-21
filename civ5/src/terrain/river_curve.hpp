#pragma once
#include "civ5/terrain/river_geometry.hpp"

namespace civ5::terrain::detail {
// Shared original 69F850/432190 evaluator. Callers validate corresponding,
// finite controls and bound the parameter before entering this internal API.
RiverPoint river_curve(std::span<const RiverPoint> points, float parameter, bool derivative,
                       const RiverCurveObserver &observer);
} // namespace civ5::terrain::detail
