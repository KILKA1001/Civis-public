#pragma once
#include "civ5/runtime/simulation_random.hpp"

namespace civ5::runtime {
// Vox Populi's RNG semantics belong to its GameCore compatibility module.
// The common session owns only wide seed/counter state and dispatches to this
// module when the selected saved GameCore is VP.
std::uint64_t vp_random_draw(SimulationRandomState &state, std::uint64_t exclusive_bound);
float vp_random_draw_float(SimulationRandomState &state);
void vp_random_reseed(SimulationRandomState &state, std::uint64_t seed);
void vp_random_reset(SimulationRandomState &state, std::uint64_t seed);
} // namespace civ5::runtime
