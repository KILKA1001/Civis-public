#pragma once
#include "civ5/saves/unit_core.hpp"
#include "civ5/saves/unit_tail.hpp"

namespace civ5::saves {
struct UnitsBudget {
    GameStateBudget per_block;
    // Caller-adjustable resource limits, not original game capacities.
    std::uint64_t retained_elements = 8 * 1024 * 1024;
    std::uint64_t retained_bytes = 64 * 1024 * 1024;
};
struct SavedUnitObservation {
    std::uint64_t index = 0;
    UnitCoreObservation core;
    UnitTailObservation tail;
};
struct UnitsObservation {
    GameStateLayout layout;
    UnitCollectionHeader header;
    std::uint64_t start_offset = 0, end_offset = 0, retained_elements = 0, retained_bytes = 0;
    std::vector<SavedUnitObservation> units;
};
// Header and every complete CvUnit, in source order, stopping before m_armyAIs.
// No ID masking, sorting, deduplication, or source fixed-capacity containers.
// Each BNW unit supplies its own core version to its tail reader.
UnitsObservation observe_units(runtime::BinaryReader &input, GameStateLayout layout, UnitsBudget budget = {});
} // namespace civ5::saves
