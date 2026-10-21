#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
enum class PlayerTraitsEncoding {
    i32,
    boolean,
    // Three VP cache flags have no initialization in the source. Preserve
    // their bytes, including noncanonical values, without creating bools.
    raw_bool8,
    yield_array,
    terrain_array,
    vector_i32,
    vector_bool,
    set_i32,
    pairs_i32,
    yield_rows,
    map_ii,
    map_iib,
    map_bii,
    map_iii,
    nested_map_ibi,
    nested_map_iii,
    map_ivec,
    counted_i32,
    counted_bool,
    counted_pairs,
    combat_i32,
    shared_combat_i32,
    hashed_i32
};
const char *player_traits_encoding_name(PlayerTraitsEncoding encoding);
struct PlayerTraitsWireDimensions {
    std::uint64_t yields = 6, terrains = 9;
};
struct PlayerTraitsRow {
    std::int64_t key = 0, wire_count = 0;
    std::vector<std::int64_t> values;
};
struct PlayerTraitsField {
    const char *name;
    PlayerTraitsEncoding encoding;
    std::uint64_t start_offset = 0, end_offset = 0, columns = 1;
    std::optional<std::int64_t> wire_count;
    std::vector<std::int64_t> values;
    // Nested maps keep source key/count/order; duplicate keys are not merged.
    std::vector<PlayerTraitsRow> rows;
    // Hash zero has no associated value. Unknown nonzero hashes retain it.
    std::vector<std::uint64_t> hashes;
};
struct PlayerTraitsObservation {
    GameStateLayout layout;
    PlayerTraitsWireDimensions dimensions;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> version;
    std::vector<PlayerTraitsField> fields;
};
// Complete saved CvPlayerTraits, including BNW versions 1..19. Next is the
// same player's Espionage. Does not derive active leader traits or bonuses,
// remap IDs/hashes, recalculate cached improvements or supply absent defaults.
// Fixed source enum spans are explicit for VP; the runtime has no such caps.
PlayerTraitsObservation observe_player_traits(runtime::BinaryReader &input, GameStateLayout layout,
                                              std::optional<PlayerTraitsWireDimensions> vp_dimensions = {},
                                              GameStateBudget budget = {});
} // namespace civ5::saves
