#pragma once
#include "civ5/saves/game_state.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::saves {
struct CityTailContext {
    std::optional<std::uint64_t> bnw_city_version;
    runtime::ProfileExtensions profile_extensions;
};
struct CityTailNode {
    const char *name;
    const char *encoding;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::optional<std::int64_t> wire_count;
    std::vector<std::int64_t> values;
    std::vector<CityTailNode> children;
};
struct CityTailObservation {
    GameStateLayout layout;
    CityTailContext context;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<CityTailNode> blocks;
};
// Religion, emphases, espionage, remaining CvCity fields, then CvCityAI.
// Retains serialized order, duplicate keys, raw fractions and source versions.
// Does not apply religion effects, normalize yields, or register a live city.
CityTailObservation observe_city_tail(runtime::BinaryReader &input, GameStateLayout layout,
                                      CityTailContext context, GameStateBudget budget = {});
} // namespace civ5::saves
