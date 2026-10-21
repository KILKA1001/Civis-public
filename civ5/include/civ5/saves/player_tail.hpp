#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
struct PlayerTailVpContext {
    // Source dimensions and saved ID determine fixed rows and the request branch.
    // These are wire context, not Civis player or yield limits.
    std::uint64_t yields = 0, players = 0, major = 0;
    std::int64_t player_id = 0;
};
struct PlayerTailNode {
    const char *name;
    const char *encoding;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::int64_t> wire_count;
    std::vector<std::int64_t> values;
    std::vector<std::uint8_t> bytes;
    std::vector<PlayerTailNode> children;
};
struct PlayerTailObservation {
    GameStateLayout layout;
    std::optional<PlayerTailVpContext> context;
    std::uint64_t start_offset = 0, base_player_end_offset = 0, end_offset = 0;
    std::uint64_t retained_elements = 0, retained_bytes = 0;
    std::vector<PlayerTailNode> fields;
};
// Reads the remaining CvPlayer history/state and CvPlayerAI suffix, stopping
// at the next player. Preserves ordering, duplicate keys and opaque strings.
// Does not truncate history, rebuild unit cycles, activate requests, recompute
// cached values, register achievements with Steam, or apply load side effects.
PlayerTailObservation observe_player_tail(runtime::BinaryReader &input, GameStateLayout layout,
                                          std::optional<PlayerTailVpContext> context = {},
                                          GameStateBudget budget = {});
} // namespace civ5::saves
