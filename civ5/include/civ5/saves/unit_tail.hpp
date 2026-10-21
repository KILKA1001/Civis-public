#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
struct UnitTailContext {
    std::optional<std::uint64_t> bnw_unit_version;
};
struct UnitTailNode {
    const char *name;
    const char *encoding;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::int64_t> wire_count;
    // Keep raw hashes, unknown IDs, duplicate rows, and original order.
    std::vector<std::int64_t> values;
    // Bitfields and strings are opaque bytes, including unused bits and NULs.
    std::vector<std::uint8_t> bytes;
    std::vector<UnitTailNode> children;
};
struct UnitTailObservation {
    GameStateLayout layout;
    UnitTailContext context;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<UnitTailNode> blocks;
};
// Reads promotions through the end of one CvUnit. The source path count is
// u32 on disk even though Civis uses 64-bit counts/offsets. BNW uses the version
// from this unit's core. No promotion cache rebuild, hash resolution or map effects.
// GameStateBudget string byte limits also cover the opaque promotion bitfields.
UnitTailObservation observe_unit_tail(runtime::BinaryReader &input, GameStateLayout layout,
                                      UnitTailContext context = {}, GameStateBudget budget = {});
} // namespace civ5::saves
