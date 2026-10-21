#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
// Source format widths. Every imported numeric value is held in int64_t.
enum class DiplomacyEncoding { i8, u8, i16, u16, i32, boolean, diplo_log, declaration };
const char *diplomacy_encoding_name(DiplomacyEncoding encoding);
struct PlayerDiplomacyWireDimensions {
    // Standard BNW source dimensions; VP requires the selected DLL context.
    std::uint64_t major = 22, minor = 41, civ_players = 63, players = 64;
    std::uint64_t approaches = 7, minor_approaches = 5, statements = 0, log_entries = 60;
};
struct DiplomacyWireSpan {
    std::uint64_t start = 0, end = 0;
};
struct PlayerDiplomacyField {
    const char *name;
    DiplomacyEncoding encoding;
    std::uint64_t rows = 0, columns = 0;
    // Source row order. diplo_log has [statement, turn]; declaration has
    // [declaration, data1, data2, must_have_met_player, active, turn].
    std::vector<std::int64_t> values;
    // BNW interleaves rows from several matrices. Preserve each consumed span.
    std::vector<DiplomacyWireSpan> spans;
};
struct PlayerDiplomacyObservation {
    GameStateLayout layout;
    PlayerDiplomacyWireDimensions dimensions;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> version;
    std::vector<PlayerDiplomacyField> fields;
};
// Reads one complete CvDiplomacyAI, immediately after GrandStrategyAI and
// before PlayerReligions. Does not evaluate diplomacy or apply reset/load effects.
PlayerDiplomacyObservation observe_player_diplomacy(runtime::BinaryReader &input, GameStateLayout layout,
                                                    PlayerDiplomacyWireDimensions dimensions,
                                                    GameStateBudget budget = {});
} // namespace civ5::saves
