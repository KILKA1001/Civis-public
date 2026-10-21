#pragma once
#include "civ5/saves/map_state.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::saves {
struct CityProductionContext {
    std::optional<std::uint64_t> bnw_city_version;
    runtime::ProfileExtensions profile_extensions;
};
struct CityProductionBlob {
    const char *name;
    std::vector<std::uint8_t> bytes;
};
struct CityProductionRecord {
    const char *name;
    std::uint64_t index = 0, start_offset = 0, end_offset = 0;
    std::vector<NamedSaveInteger> fields;
    std::vector<PlotArrayObservation> arrays;
    std::vector<CityProductionBlob> blobs;
    std::vector<CityProductionRecord> children;
};
struct CityProductionObservation {
    GameStateLayout layout;
    CityProductionContext context;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<CityProductionRecord> blocks;
};
// Reads CityBuildings and city production data, stopping before CityStrategyAI.
// VP spans use saved-profile building slots, never the city's building count.
// Source hashes, unknown orders, duplicate records and raw name bytes survive.
// Does not resolve content, filter the queue, rebuild caches or apply load effects.
CityProductionObservation observe_city_production(runtime::BinaryReader &input, GameStateLayout layout,
                                                  CityProductionContext context, GameStateBudget budget = {});
} // namespace civ5::saves
