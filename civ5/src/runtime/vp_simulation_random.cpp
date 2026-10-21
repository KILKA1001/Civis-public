#include "civ5/runtime/vp_simulation_random.hpp"
#include <limits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require_increment(std::uint64_t value) {
    if (value == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Simulation RNG counter exceeds host range");
}
// floor(sample * bound / 2^32) without a 128-bit product, rounding or
// narrowing the bound. The two partial products and their sum fit in u64.
std::uint64_t scale(std::uint64_t sample, std::uint64_t bound) {
    constexpr unsigned bits = 32;
    constexpr auto mask = (std::uint64_t{1} << bits) - 1;
    return sample * (bound >> bits) + ((sample * (bound & mask)) >> bits);
}
} // namespace

std::uint64_t vp_random_draw(SimulationRandomState &state, std::uint64_t exclusive_bound) {
    // VP skips both the state transition and call counter for these ranges.
    if (exclusive_bound < 2) return 0;
    require_increment(state.calls);
    const auto next_seed = state.seed * 1103515245ULL + 12345ULL;
    const auto sample = (next_seed >> 16) & 0xffffffffULL;
    const auto result = scale(sample, exclusive_bound);
    state.seed = next_seed;
    ++state.calls;
    return result;
}

float vp_random_draw_float(SimulationRandomState &state) {
    constexpr auto bound = 0xffffffffULL;
    return static_cast<float>(vp_random_draw(state, bound)) / static_cast<float>(bound);
}

void vp_random_reseed(SimulationRandomState &state, std::uint64_t seed) {
    require_increment(state.resets);
    state.seed = seed;
    ++state.resets;
}

void vp_random_reset(SimulationRandomState &state, std::uint64_t seed) {
    if (state.seed != seed) vp_random_reseed(state, seed);
    state.calls = 0;
}
} // namespace civ5::runtime
