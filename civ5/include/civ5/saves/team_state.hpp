#pragma once
#include "civ5/saves/map_state.hpp"

namespace civ5::saves {
// Source wire dimensions only; collections and IDs in the host are dynamic/wide.
struct TeamWireDimensions {
    std::uint64_t teams = 64, players = 64, civ_teams = 63, fixed_players = 80, fixed_teams = 80;
    std::uint64_t domains = 5, major = 22, yields = 6;
    runtime::ProfileExtensions profile_extensions;
};
struct TeamHashedValues {
    std::uint64_t type_hash = 0;
    std::vector<std::int64_t> values;
};
struct TeamHashedArray {
    const char *name;
    std::int64_t wire_count = 0;
    std::vector<TeamHashedValues> rows;
};
struct TeamProjectArt {
    std::int64_t key = 0, source_count = 0;
    // VP: explicit vector length, including UINT32_MAX sentinel. BNW: length
    // comes from the earlier saved project-count hash, without another prefix.
    std::optional<std::uint64_t> wire_count;
    std::vector<std::int64_t> values;
};
struct TeamObservation {
    std::uint64_t index = 0, start_offset = 0, end_offset = 0;
    std::vector<NamedSaveInteger> fields;
    std::vector<PlotArrayObservation> arrays;
    std::vector<TeamHashedArray> hashes;
    std::vector<TeamProjectArt> project_art;
};
struct TeamsObservation {
    GameStateLayout layout;
    TeamWireDimensions dimensions;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<TeamObservation> teams;
};
// Full saved CvTeam and CvTeamTechs data. Does not apply self-war fixes,
// best-route updates, hash remapping, diplomacy effects or technology unlocks.
// VP fixed bool arrays retain raw bytes, including uninitialized source tail
// slots beyond MAX_TEAMS/PLAYERS; these bytes are never C++ bool objects.
TeamsObservation observe_teams(runtime::BinaryReader &input, GameStateLayout layout,
                               TeamWireDimensions dimensions = {}, GameStateBudget budget = {});
} // namespace civ5::saves
