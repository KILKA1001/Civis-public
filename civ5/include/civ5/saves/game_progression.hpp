#pragma once
#include "civ5/saves/game_state.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::saves {
struct ProgressionWireDimensions {
    std::uint64_t vote_team_slots = 0, victory_award_slots = 0;
    runtime::ProfileExtensions profile_extensions{};
};
struct ProgressionRow {
    // Vanilla hash 0 means a missing entry with no following value. VP arrays
    // have no hashes on the wire. Preserve unknown hashes without resolving IDs.
    std::optional<std::uint64_t> type_hash;
    std::vector<std::int64_t> values;
};
struct ProgressionArray {
    const char *name;
    std::uint64_t start_offset = 0, end_offset = 0, columns = 1;
    std::vector<ProgressionRow> rows;
};
struct NameHistory {
    std::vector<std::uint64_t> hashes;
    std::vector<std::vector<std::uint8_t>> strings;
};
struct VoteOptionObservation {
    std::int64_t vote = 0, player = 0, city = 0, other_player = 0;
    std::vector<std::uint8_t> text;
};
struct VoteEntryObservation {
    std::int64_t id = 0, source = 0;
    std::vector<VoteOptionObservation> options;
};
struct VoteFreeListObservation {
    // Source allocation metadata is retained as data; no native free list is
    // instantiated and no packed ID is used as a host pointer or capacity.
    std::int64_t last_index = 0, free_head = 0, free_count = 0, current_id = 0;
    std::vector<std::int64_t> next_free;
    std::vector<VoteEntryObservation> entries;
};
struct RandomStateObservation {
    std::uint64_t seed = 0, calls = 0, resets = 0;
};
struct GameProgressionObservation {
    GameStateLayout layout;
    ProgressionWireDimensions dimensions;
    std::uint64_t start_offset = 0, histories_offset = 0, random_offset = 0, end_offset = 0;
    std::vector<ProgressionArray> arrays;
    NameHistory destroyed_cities, great_people;
    std::optional<std::array<VoteFreeListObservation, 2>> vote_free_lists;
    std::array<RandomStateObservation, 2> random;
};
// Starts immediately after RosterObservation::end_offset, stops before replay
// messages. Dimensions describe bytes, not a verified saved content profile.
// No RNG advance, load side effects, or continuation capability is implied.
GameProgressionObservation observe_game_progression(runtime::BinaryReader &input, GameStateLayout layout,
                                                    ProgressionWireDimensions dimensions,
                                                    GameStateBudget budget = {});
} // namespace civ5::saves
