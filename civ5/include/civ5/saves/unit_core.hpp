#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
enum class UnitCoreEncoding {
    i32,
    hash32,
    boolean,
    string32,
    vector_i32,
    vector_bool,
    set_i32,
    vector_pair,
    map_pair,
    map_triple,
    plagues
};
const char *unit_core_encoding_name(UnitCoreEncoding encoding);
struct UnitCoreField {
    const char *name;
    UnitCoreEncoding encoding;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> wire_count;
    // Rows retain source order, duplicate keys and unknown enum/hash values.
    // Plague rows contain promotion, domain, attack flag, defense flag, chance.
    std::vector<std::int64_t> values;
    std::vector<std::uint8_t> bytes;
};
struct UnitCoreObservation {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> version;
    std::vector<UnitCoreField> fields;
};
struct UnitCollectionHeader {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, end_offset = 0, source_slots = 0;
    std::vector<NamedSaveInteger> fields;
    std::vector<std::int64_t> next_free_indices;
    std::int64_t wire_unit_count = 0;
};
UnitCollectionHeader observe_unit_collection_header(runtime::BinaryReader &input, GameStateLayout layout,
                                                    GameStateBudget budget = {});
// Reads one unit up to promotions. BNW archive fields follow registration order,
// followed by the raw unit-type hash and last-move turn. No hash resolution,
// object registration, reset, cache rebuilding or 32-bit source container limits.
UnitCoreObservation observe_unit_core(runtime::BinaryReader &input, GameStateLayout layout,
                                      GameStateBudget budget = {});
} // namespace civ5::saves
