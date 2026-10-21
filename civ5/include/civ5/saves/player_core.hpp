#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
// These encodings describe source bytes, not host integer widths/capacities.
enum class PlayerCoreEncoding {
    i32,
    u32,
    i64,
    boolean,
    fraction,
    string32,
    vector_i32,
    vector_bool,
    vector_u8,
    set_i32,
    nested_i32,
    options,
    hashed_i32,
    yield_rows
};
const char *player_core_encoding_name(PlayerCoreEncoding encoding);
struct PlayerCoreField {
    // Exact source member name keeps distinct counters and units distinguishable.
    const char *name;
    PlayerCoreEncoding encoding;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> wire_count;
    // Scalars have one value, fractions have numerator/denominator, options
    // alternate unsigned option key/signed value. No fraction normalization.
    std::vector<std::int64_t> values;
    // Nested vector lengths retain UINT32_MAX empty sentinels. Values are
    // concatenated in source row order; fixed yield rows have no row prefix.
    std::vector<std::uint64_t> row_counts;
    // BNW: hash 0 has no value; all nonzero hashes have one value, even unknowns.
    std::vector<std::uint64_t> hashes;
    std::vector<std::uint8_t> bytes;
};
struct PlayerCoreObservation {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, end_offset = 0, yield_slots = 0;
    std::optional<std::uint64_t> version;
    std::vector<PlayerCoreField> fields;
};
// Reads one player's core through the field immediately before PlayerPolicies.
// Caller must finish that player's subsystems/entities before reading the next
// core. Does not apply reset(), cached-value recomputation or loading effects.
// VP fixed yield rows require the selected source NUM_YIELD_TYPES explicitly.
PlayerCoreObservation observe_player_core(runtime::BinaryReader &input, GameStateLayout layout,
                                          std::optional<std::uint64_t> vp_yield_slots = {},
                                          GameStateBudget budget = {});
} // namespace civ5::saves
