#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
struct BeliefBuildingCount {
    std::uint64_t type_hash = 0;
    std::optional<std::int64_t> value;
};
struct ReligionBeliefsObservation {
    std::optional<std::uint64_t> version;
    std::optional<std::int64_t> religion;
    std::vector<NamedSaveInteger> cached_values;
    std::vector<std::uint64_t> belief_hashes;
    std::vector<std::int64_t> belief_ids, belief_lookup;
    std::vector<BeliefBuildingCount> building_class_enabled;
};
struct ReligionObservation {
    std::optional<std::uint64_t> version;
    std::int64_t religion = 0, founder = 0, holy_city_x = 0, holy_city_y = 0, turn_founded = 0;
    bool pantheon = false, enhanced = false;
    std::optional<bool> reformed;
    // Original field: 128 raw bytes, without a length or required terminator.
    // Empty for legacy versions which did not serialize a custom name. This
    // vector does not limit the length of names in a future Civis session.
    std::vector<std::uint8_t> custom_name;
    ReligionBeliefsObservation beliefs;
};
struct GreatWorkObservation {
    std::optional<std::uint64_t> version;
    std::vector<std::uint8_t> creator;
    std::optional<std::vector<std::uint8_t>> legacy_name;
    std::int64_t type = 0, turn_founded = 0, era = 0, player = 0;
    // BNW versions 1/2 derive this through the selected rules after reading.
    // Missing is distinct from a serialized NO_GREAT_WORK_CLASS (-1).
    std::optional<std::int64_t> class_type;
};
struct GameReligionCultureObservation {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, culture_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> religions_version, culture_version;
    std::int64_t minimum_faith_next_pantheon = 0;
    std::optional<std::int64_t> obsolete_minimum_faith_next_prophet;
    std::vector<ReligionObservation> religions;
    std::vector<GreatWorkObservation> great_works;
    bool reported_someone_influential = false;
};
// Starts after CvGameDeals; stops before CvGameLeagues. Preserves all source
// IDs, lookup entries, cached effects and opaque names. Does not apply beliefs,
// resolve hashes/classes, sanitize pantheon coordinates, or compute tourism.
GameReligionCultureObservation observe_game_religion_culture(runtime::BinaryReader &input,
                                                             GameStateLayout layout,
                                                             GameStateBudget budget = {});
} // namespace civ5::saves
