#pragma once
#include "civ5/saves/map_state.hpp"

namespace civ5::saves {
struct PlayerAdministrationBlob {
    const char *name;
    std::vector<std::uint8_t> bytes;
};
struct PlayerAdministrationRecord {
    const char *name;
    std::uint64_t index = 0, start_offset = 0, end_offset = 0;
    std::vector<NamedSaveInteger> fields;
    std::vector<PlotArrayObservation> arrays;
    std::vector<PlayerAdministrationBlob> blobs;
    std::vector<PlayerAdministrationRecord> children;
};
struct PlayerAdministrationObservation {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<PlayerAdministrationRecord> blocks;
};
// Reads the has-notifications flag through the data immediately before m_cities.
// Fixed notification slots are source format spans, not native runtime limits.
// Notification type bits (including legacy BNW indices), inactive slots and raw
// strings remain intact. No broadcasts, type remapping, treasury recalculation,
// flavor hookups, queue reordering or live entity resolution occurs here.
PlayerAdministrationObservation observe_player_administration(runtime::BinaryReader &input,
                                                              GameStateLayout layout,
                                                              GameStateBudget budget = {});
} // namespace civ5::saves
