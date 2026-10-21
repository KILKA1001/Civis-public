#include "civ5/runtime/simulation_random.hpp"
#include "civ5/runtime/vp_simulation_random.hpp"
#include <limits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr std::uint64_t source_mask = 0xffffffffULL;
void require_increment(std::uint64_t value) {
    if (value == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Simulation RNG counter exceeds host range");
}
// floor(sample * bound / 2^bits), without a 128-bit product, floating-point
// rounding or narrowing bound. Both partial products and their sum fit u64.
std::uint64_t scale(std::uint64_t sample, std::uint64_t bound, unsigned bits) {
    const auto mask = (std::uint64_t{1} << bits) - 1;
    return sample * (bound >> bits) + ((sample * (bound & mask)) >> bits);
}
} // namespace

SimulationRandom::SimulationRandom(RandomAlgorithm algorithm, SimulationRandomState state)
    : algorithm_(algorithm), state_(state) {
    if (algorithm != RandomAlgorithm::bnw && algorithm != RandomAlgorithm::vp_reference)
        throw std::invalid_argument("Unknown simulation RNG algorithm");
    validate_seed(state.seed);
}
void SimulationRandom::validate_seed(std::uint64_t seed) const {
    if (algorithm_ == RandomAlgorithm::bnw && seed > source_mask)
        throw std::invalid_argument("BNW RNG seed is outside its algorithm's residue range");
}
std::uint64_t SimulationRandom::draw(std::uint64_t exclusive_bound) {
    if (algorithm_ == RandomAlgorithm::vp_reference)
        return vp_random_draw(state_, exclusive_bound);
    require_increment(state_.calls);
    auto next_seed = state_.seed * 1103515245ULL + 12345ULL;
    next_seed &= source_mask;
    const auto sample = (next_seed >> 16) & 0xffffULL;
    const auto result = scale(sample, exclusive_bound, 16);
    state_.seed = next_seed;
    ++state_.calls;
    return result;
}
float SimulationRandom::draw_float() {
    if (algorithm_ == RandomAlgorithm::vp_reference)
        return vp_random_draw_float(state_);
    return static_cast<float>(draw(65535ULL)) / 65535.0f;
}
void SimulationRandom::reseed(std::uint64_t seed) {
    if (algorithm_ == RandomAlgorithm::vp_reference) {
        vp_random_reseed(state_, seed);
        return;
    }
    validate_seed(seed);
    require_increment(state_.resets);
    state_.seed = seed;
    ++state_.resets;
}
void SimulationRandom::reset(std::uint64_t seed) {
    if (algorithm_ == RandomAlgorithm::vp_reference) {
        vp_random_reset(state_, seed);
        return;
    }
    validate_seed(seed);
    reseed(seed);
}
SessionRandomStreams restore_session_random(const saves::GameProgressionObservation &progression) {
    RandomAlgorithm algorithm;
    switch (progression.layout) {
    case saves::GameStateLayout::expansion2_version1:
        algorithm = RandomAlgorithm::bnw;
        break;
    case saves::GameStateLayout::vp542_corpus:
        algorithm = RandomAlgorithm::vp_reference;
        break;
    default:
        throw std::invalid_argument("Unknown saved-session RNG layout");
    }
    const auto restore = [&](const saves::RandomStateObservation &state) {
        return SimulationRandom(algorithm, {state.seed, state.calls, state.resets});
    };
    return {restore(progression.random[0]), restore(progression.random[1])};
}
} // namespace civ5::runtime
