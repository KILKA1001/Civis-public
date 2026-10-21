#pragma once
#include "civ5/runtime/binary_reader.hpp"
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
struct BarbSpawnerObservation {
    std::optional<bool> attacked;
    std::int64_t counter = 0;
    std::int64_t units_spawned = 0;
};

struct SupportingClassesObservation {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::uint64_t plot_count = 0, major_players = 0;
    std::optional<std::uint64_t> game_version;
    std::optional<std::uint64_t> barbarians_version;
    std::optional<std::uint64_t> goody_huts_version;
    std::vector<BarbSpawnerObservation> barbarians;
    // CvGoodyHuts uses NUM_GOODIES_REMEMBERED == 3 in the source SDK.
    std::vector<std::array<std::int64_t, 3>> goody_hut_results;
};

// Reads CvGame::ReadSupportingClassData after the serialized player array.
// BNW carries CvGame/CvBarbarians/CvGoodyHuts version words and no attacked
// flag; VP carries the newer bool span followed by the two short spans.
// Source widths are retained on the wire; host offsets, counts and budgets
// remain 64-bit.
SupportingClassesObservation observe_supporting_classes(runtime::BinaryReader &input, GameStateLayout layout,
                                                        std::uint64_t plot_count, std::uint64_t major_players,
                                                        GameStateBudget budget = {});
} // namespace civ5::saves
