#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
struct CorporationObservation {
    std::int64_t type = 0, founder = 0, headquarters_x = 0, headquarters_y = 0, turn_founded = 0;
};
struct ContractObservation {
    std::int64_t type = 0, holder = 0, turns = 0, maintenance = 0, turn_start = 0;
};
struct VpFinalSystemsObservation {
    std::vector<CorporationObservation> corporations;
    std::vector<ContractObservation> active_contracts, inactive_contracts;
    std::vector<std::uint8_t> game_id;
};
struct GameFinalSystemsObservation {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<VpFinalSystemsObservation> vp;
};
// BNW has no fields in this segment. VP stores corporations, both contract
// lists and its game ID. Does not resolve the ID into the machine's stats.db.
GameFinalSystemsObservation observe_game_final_systems(runtime::BinaryReader &input, GameStateLayout layout,
                                                       GameStateBudget budget = {});
struct SavedDatabaseObservation {
    std::uint64_t start_offset = 0, data_offset = 0, end_offset = 0;
    std::uint64_t declared_image_bytes = 0;
    // Zero length is an absent image, not a new database synthesized on load.
    std::vector<std::uint8_t> image;
    bool quick_check_passed = false;
    // Offsets and declared_image_bytes always describe the retained source
    // payload. A Civis section can replace the current image independently.
    bool image_from_civis_section = false;
    // Explicit map-only diagnostic: non-SQLite bytes were not consumed as a DB.
    bool non_sqlite_image_skipped = false;
};
// Validates a complete nonempty SQLite image without an original u32 length
// prefix. This is shared by the original importer and Civis section reader.
void validate_saved_database_image(std::span<const std::uint8_t> image);
// Reads the declared image, runs SQLite quick_check in an isolated memory
// connection and preserves all bytes. Never writes the original cache or save.
// Strict mode rejects a missing/invalid signature. Explicit map diagnostics
// may leave non-SQLite bytes unread and mark the saved image unverified.
SavedDatabaseObservation observe_saved_database(runtime::BinaryReader &input,
                                                std::uint64_t maximum_bytes = 256 * 1024 * 1024,
                                                bool skip_non_sqlite_image_for_map_diagnostic = false);
} // namespace civ5::saves
