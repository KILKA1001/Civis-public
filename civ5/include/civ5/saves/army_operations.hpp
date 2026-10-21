#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
struct ArmyOperationsContext {
    std::optional<std::uint64_t> bnw_player_version;
};
struct ArmyOperationsBudget {
    // Caller-adjustable accounting for retained records and numeric elements.
    // This is an import resource budget, not a game/container capacity.
    std::uint64_t elements = 8 * 1024 * 1024;
};
struct ArmyOperationsNode {
    const char *name;
    const char *encoding;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::int64_t> wire_count;
    std::vector<std::int64_t> values;
    std::vector<ArmyOperationsNode> children;
};
struct ArmyCollectionHeader {
    std::uint64_t start_offset = 0, end_offset = 0, source_slots = 0;
    std::vector<NamedSaveInteger> fields;
    std::vector<std::int64_t> next_free_indices;
    std::int64_t wire_army_count = 0;
};
struct ArmyOperationsObservation {
    GameStateLayout layout;
    ArmyOperationsContext context;
    ArmyCollectionHeader header;
    std::uint64_t start_offset = 0, armies_end_offset = 0, operations_end_offset = 0, end_offset = 0;
    std::uint64_t wire_operation_count = 0, retained_elements = 0;
    std::vector<ArmyOperationsNode> armies, operations;
    std::optional<std::int64_t> bnw_unused_popup_count;
};
// Complete army collection and typed AI operations, then BNW's obsolete popup
// count for player versions <=10. Stops before player replay/history data.
// Retains source order, unknown values within known record layouts, duplicate
// IDs and BNW's discarded outer operation ID. Does not invoke factories, Reset,
// unit release/attachment, ID masks or source fixed-capacity containers.
ArmyOperationsObservation observe_army_operations(runtime::BinaryReader &input, GameStateLayout layout,
                                                  ArmyOperationsContext context = {},
                                                  ArmyOperationsBudget budget = {});
} // namespace civ5::saves
