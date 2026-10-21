#pragma once
#include "civ5/saves/map_state.hpp"

namespace civ5::saves {
// Dimensions of the legacy BNW wire arrays, not capacities of the x64 world.
// The SDK writes 80 fixed entries but only 64 target-city/yield rows.
struct RegionWireDimensions {
    std::uint64_t fixed_players = 80, fixed_teams = 80, players = 64, yields = 6;
};
struct MapRegionRecord {
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> version;
    std::vector<NamedSaveInteger> fields;
    std::vector<PlotArrayObservation> arrays;
    std::vector<PlotIndexedValues> yield_modifiers;
    std::vector<MapResourceCount> resources, improvements;
};
struct MapRegionContainer {
    const char *name;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::int64_t wire_count = 0;
    // BNW: slots, last index, free head, free count, current ID; then each
    // next-free index. Retained as source metadata, never used as x64 IDs.
    std::vector<std::int64_t> legacy_metadata, next_free;
    std::vector<MapRegionRecord> records;
};
struct MapRegionsObservation {
    GameStateLayout layout;
    std::optional<RegionWireDimensions> dimensions;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<MapRegionContainer> containers;
    std::int64_t ai_map_hints = 0;
};
// Stops at the end of CvMap serialization. Preserves unknown/duplicate IDs,
// hashes, null river references (-1), ordering and legacy allocation metadata.
// Does not load into native freelists, resolve plot pointers or run map hooks.
MapRegionsObservation observe_map_regions(runtime::BinaryReader &input, GameStateLayout layout,
                                          RegionWireDimensions dimensions = {}, GameStateBudget budget = {});
} // namespace civ5::saves
