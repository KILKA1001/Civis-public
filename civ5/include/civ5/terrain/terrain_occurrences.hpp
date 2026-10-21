#pragma once
#include "terrain_placement.hpp"
#include <functional>

namespace civ5::terrain {
struct TerrainGenerationSetup {
    std::array<IntMatrix4, 4> orientations;
    std::vector<std::uint32_t> cooked_to_primary;
    std::int32_t maximum_pass;
};
// Original owner construction and normal FXSB post-load selection. The global
// factory descriptor remains explicit; no captured matrices/mapping are inputs.
// Active records require native positional primary ordinals. Custom explicit
// cooked mappings remain available through generate_terrain_occurrences.
TerrainGenerationSetup prepare_terrain_generation(const TerrainPieceLibrary &library,
                                                   const FactoryDescriptor &factory);
// Explicit entering thread state, never implicitly seeded from a save/time.
// Same stream can continue into subsequent terrain producers.
class TerrainRandom {
  public:
    explicit TerrainRandom(std::uint32_t holdrand) : state_(holdrand) {}
    std::uint16_t next();
    std::uint32_t index(std::uint32_t bound);
    std::uint32_t state() const { return state_; }
    std::uint64_t calls() const { return calls_; }
  private:
    std::uint32_t state_;
    std::uint64_t calls_ = 0;
};
using TerrainPassObserver = std::function<void(std::int32_t, std::span<const TerrainOccurrence>,
                                              const TerrainLogicalClaims &, const TerrainRandom &)>;
struct TerrainGeneration {
    std::vector<TerrainOccurrence> occurrences;
    TerrainLogicalClaims claims;
};
// Original descending pass/candidate/record/orientation loop. Cooked mapping
// selects records without sorting/filtering the shuffled index space. All four
// orientations and every pass consume random values, even with no matches.
TerrainGeneration generate_terrain_occurrences(const TerrainPieceLibrary &library,
    const TerrainPlacement &placement, std::span<const std::uint32_t> cooked_to_primary,
    std::int32_t width, std::int32_t height, std::span<const std::uint8_t> cells,
    std::int32_t maximum_pass, TerrainRandom &random, const TerrainPassObserver &observer = {});
} // namespace civ5::terrain
