#pragma once
#include "civ5/saves/expansion2_prefix.hpp"
#include "civ5/saves/vp_prefix.hpp"
#include "civ5/runtime/profile_extensions.hpp"
#include <optional>
#include <variant>
#include <vector>

namespace civ5::saves {
enum class GameStateLayout { expansion2_version1, vp542_corpus };
struct NamedSaveFlag {
    const char *name;
    bool value;
};
struct NamedSaveInteger {
    const char *name;
    std::int64_t value;
};
// Counts describe the source GameCore's wire layout, not Civis capacities.
// VP's resource array has neither a count nor type hashes in the stream.
// Its dimension must come from the selected rules, never a byte-search guess.
struct RosterWireDimensions {
    std::uint64_t player_slots = 0, team_slots = 0;
    runtime::ProfileExtensions profile_extensions{};
};
struct GameStateBudget {
    std::uint64_t elements = 1024 * 1024;
    std::uint64_t string_bytes = 16 * 1024 * 1024;
    std::uint64_t total_string_bytes = 64 * 1024 * 1024;
};
struct RosterObservation {
    RosterWireDimensions dimensions;
    std::uint64_t start_offset = 0, rankings_offset = 0, end_offset = 0;
    std::vector<std::int64_t> resource_monopoly_players;
    // Script data is opaque: preserve embedded NULs and non-UTF-8 bytes.
    std::vector<std::uint8_t> script_data;
    std::array<std::vector<std::int64_t>, 7> arrays;
    static constexpr std::array<const char *, 7> array_names{"end_turn_messages_received",
                                                             "rank_player",
                                                             "player_rank",
                                                             "player_score",
                                                             "rank_team",
                                                             "team_rank",
                                                             "team_score"};
};
// Partial serialized state only. No CvGame::Read side effects (reseed, session
// increment, normalization of game-over or autoplay, replay messages) run here.
struct GameStateObservation {
    GameStateLayout layout;
    std::variant<Expansion2PrefixObservation, VpPrefixObservation> prefix;
    std::vector<NamedSaveFlag> flags;
    std::vector<NamedSaveInteger> controls;
    std::vector<std::vector<std::uint8_t>> advisor_messages;
    std::vector<std::uint64_t> advisor_hashes;
    std::uint64_t start_offset = 0, controls_end_offset = 0, end_offset = 0;
    std::optional<RosterObservation> roster;
};
// Without dimensions, stop before content-dependent/slot-dependent arrays.
// Providing dimensions permits observation; it does not verify a content profile.
GameStateObservation observe_game_state(runtime::BinaryReader &input, GameStateLayout layout,
                                        std::optional<RosterWireDimensions> dimensions = {},
                                        GameStateBudget budget = {});
} // namespace civ5::saves
