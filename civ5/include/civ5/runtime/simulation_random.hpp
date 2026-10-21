#pragma once
#include "civ5/saves/game_progression.hpp"

namespace civ5::runtime {
enum class RandomAlgorithm { bnw, vp_reference };
struct SimulationRandomState {
    std::uint64_t seed = 0, calls = 0, resets = 0;
    bool operator==(const SimulationRandomState &) const = default;
};

// A session-owned deterministic generator, restored without an implicit call
// or reset. Its algorithm preserves source RNG arithmetic; counts and draw
// ranges are x64, independent of the source GameCore's container capacities.
// Mutation belongs to the simulation thread, never a renderer or UI reader.
class SimulationRandom {
  public:
    SimulationRandom(RandomAlgorithm algorithm, SimulationRandomState state);
    RandomAlgorithm algorithm() const noexcept { return algorithm_; }
    const SimulationRandomState &state() const noexcept { return state_; }
    std::uint64_t draw(std::uint64_t exclusive_bound);
    float draw_float();
    void reseed(std::uint64_t seed);
    void reset(std::uint64_t seed = 0);

  private:
    void validate_seed(std::uint64_t seed) const;
    RandomAlgorithm algorithm_;
    SimulationRandomState state_;
};

struct SessionRandomStreams {
    SimulationRandom map;
    SimulationRandom gameplay;
};
// CvGame stores map RNG first, gameplay RNG second. Algorithm selection here
// describes the source reference only; it does not certify an arbitrary mod
// DLL or make a decoded save ready for game continuation.
SessionRandomStreams restore_session_random(const saves::GameProgressionObservation &progression);
} // namespace civ5::runtime
