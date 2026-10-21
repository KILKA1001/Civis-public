#pragma once
#include "civ5/saves/map_state.hpp"

namespace civ5::saves {
struct PlayerEspionageBlob {
    const char *name;
    std::vector<std::uint8_t> bytes;
};
struct PlayerEspionageRecord {
    const char *name;
    // Position within the saved container, not an invented game ID.
    std::uint64_t index = 0, start_offset = 0, end_offset = 0;
    std::vector<NamedSaveInteger> fields;
    std::vector<PlotArrayObservation> arrays;
    std::vector<PlayerEspionageBlob> blobs;
    std::vector<PlayerEspionageRecord> children;
};
struct PlayerEspionageObservation {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<PlayerEspionageRecord> blocks;
};
// Complete saved PlayerEspionage and EspionageAI. Next is PlayerTrade of
// the same player. Names stay raw bytes, signed loop counts remain visible,
// and collection sizes come from the stream rather than original array caps.
// No spy reassignment, name/tech resolution, notifications or gameplay effects.
PlayerEspionageObservation observe_player_espionage(runtime::BinaryReader &input, GameStateLayout layout,
                                                    GameStateBudget budget = {});
} // namespace civ5::saves
