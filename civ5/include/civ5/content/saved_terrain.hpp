#pragma once
#include "civ5/content/info_types.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/saves/map_state.hpp"
#include <string_view>

namespace civ5::content {
struct SavedTerrainFeatureCatalog {
    std::uint64_t definitions = 0;
    saves::SavedTerrainFeatureResolver resolve;
};
// Snapshot the selected, already activated Features table. The caller must
// perform any required ID remapping first. This does not establish that the
// database matches the save. Hashes resolve through the global GameCore type
// cache, then index the Features collection (including holes/overlapping IDs).
// Null slots return empty, non-wonder values; outside IDs/hashes are unresolved.
// The explicit cache overload requires
// a cache built from the same rules revision; the default builds one itself.
// The returned resolver owns its data and survives changes/destruction of rules.
SavedTerrainFeatureCatalog saved_terrain_features(const RulesDatabase &rules,
                                                  std::uint64_t maximum_definitions = 1024 * 1024);
SavedTerrainFeatureCatalog saved_terrain_features(const RulesDatabase &rules, const InfoTypeCatalog &types,
                                                  std::uint64_t maximum_definitions = 1024 * 1024);
} // namespace civ5::content
