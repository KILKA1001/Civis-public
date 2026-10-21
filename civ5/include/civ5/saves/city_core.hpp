#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
enum class CityCoreEncoding {
    i32,
    hash32,
    boolean,
    fraction,
    string32,
    vector_i32,
    vector_bool,
    set_i32,
    vector_fraction,
    map_ii,
    nested_map_i_iii,
    hashed_i32
};
const char *city_core_encoding_name(CityCoreEncoding encoding);
struct CityCoreField {
    const char *name;
    CityCoreEncoding encoding;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> wire_count;
    // Fractions keep numerator/denominator; map entries stay in source order.
    std::vector<std::int64_t> values, row_keys;
    std::vector<std::uint64_t> row_counts, hashes;
    std::vector<std::uint8_t> bytes;
};
struct CityCoreObservation {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> version;
    std::vector<CityCoreField> fields;
};
struct CityCollectionHeader {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, end_offset = 0, source_slots = 0;
    std::vector<NamedSaveInteger> fields;
    std::vector<std::int64_t> next_free_indices;
    std::int64_t wire_city_count = 0;
};
// BNW free-list metadata is retained without its original ID/capacity limits.
// VP stores a signed object count only. Does not create or index live cities.
CityCollectionHeader observe_city_collection_header(runtime::BinaryReader &input, GameStateLayout layout,
                                                    GameStateBudget budget = {});
// One city's fields up to CityBuildings. Caller must finish its buildings,
// queue, subsystems and CityAI before advancing to another city.
// No reset, hash resolution, fraction normalization or load-time recomputation.
CityCoreObservation observe_city_core(runtime::BinaryReader &input, GameStateLayout layout,
                                      GameStateBudget budget = {});
} // namespace civ5::saves
