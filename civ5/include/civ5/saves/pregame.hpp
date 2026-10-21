#pragma once
#include "civ5/saves/game_state.hpp"
#include "civ5/saves/save_header.hpp"
#include <map>

namespace civ5::saves {
// Serialized observations, not a live CvPreGame singleton. Strings preserve
// their wire bytes, including passwords and script data; do not log them.
struct PregameRecord {
    std::map<std::string, std::int64_t> integers;
    std::map<std::string, bool> flags;
    // Preserve float32 bits, including NaNs, without host FP normalization.
    std::map<std::string, std::uint32_t> float_bits;
    std::map<std::string, std::string> strings;
    std::map<std::string, std::vector<std::int64_t>> integer_arrays;
    std::map<std::string, std::vector<bool>> flag_arrays;
    std::map<std::string, std::vector<std::string>> string_arrays;
};
struct PregameInfo {
    std::int64_t id = -1;
    // Civilopedia, description, help, disabled help, strategy, type, text key, text.
    std::array<std::string, 8> text;
    PregameRecord values;
    std::optional<std::uint64_t> wire_version;
};
struct PregameOption {
    std::string name;
    std::int64_t value = 0;
};
struct PregameObservation {
    GameStateLayout layout;
    std::uint64_t header_tail_offset = 0, slots_offset = 0, archive_offset = 0,
                  archive_end_offset = 0, end_offset = 0;
    std::uint64_t slots_version = 0, archive_version = 0;
    // The native header reader has three more FString fields after the mod list.
    // Their offsets/encoding are verified; only the third's PlayerColor use is
    // identified. Retain the others without guessing their meaning.
    std::array<std::string, 3> header_tail_strings;
    std::array<std::uint8_t, 16> gamecore_guid{}, engine_guid{};
    std::string gamecore_version;
    std::int64_t engine_value = 0;
    std::array<std::uint8_t, 4> engine_trailer{};
    PregameRecord slots, archive;
    // Source masks only. This representation is never a host player limit.
    std::optional<std::vector<std::uint64_t>> wire_known_player_masks;
    PregameInfo calendar, climate, sea_level, turn_timer, world;
    // Ordered entries retain duplicate option names and unknown mod options.
    std::vector<PregameOption> game_options, map_options;
};
struct PregameBudget {
    std::uint64_t total_bytes = 128ULL * 1024 * 1024;
    std::uint64_t elements = 1024 * 1024;
    std::uint64_t string_bytes = 16ULL * 1024 * 1024;
    std::uint64_t total_string_bytes = 64ULL * 1024 * 1024;
};
// Explicit BNW / pinned VP 5.4.2 wire contracts: slot hints v3, archive v6.
// Starts at the already parsed header boundary; never searches for a version
// marker. No load hooks, reseating, key remapping or default options are applied.
PregameObservation observe_pregame(std::span<const std::uint8_t> source_prefix,
                                  const SaveHeader &header, GameStateLayout layout,
                                  PregameBudget budget = {});
} // namespace civ5::saves
