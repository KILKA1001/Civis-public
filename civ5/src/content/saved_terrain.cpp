#include "civ5/content/saved_terrain.hpp"
#include <map>
#include <stdexcept>

namespace civ5::content {
SavedTerrainFeatureCatalog saved_terrain_features(const RulesDatabase &rules,
                                                  std::uint64_t maximum_definitions) {
    return saved_terrain_features(rules, rules.cache_info_types(), maximum_definitions);
}
SavedTerrainFeatureCatalog saved_terrain_features(const RulesDatabase &rules, const InfoTypeCatalog &types,
                                                  std::uint64_t maximum_definitions) {
    if (!types.report().all_succeeded())
        throw std::runtime_error("Saved terrain requires a complete global type cache");
    struct Definitions {
        std::map<std::uint64_t, saves::SavedTerrainFeature> slots;
        std::uint64_t span = 0;
        InfoTypeCatalog types;
    };
    auto definitions = std::make_shared<Definitions>();
    definitions->types = types;
    const auto collection = rules.prefetch_info("Features", [&](std::uint64_t slot, std::int64_t,
                                                                  const RulesRowView &row) {
        // The plot stores the collection index, not CvBaseInfo::m_iID. After
        // overlapping source IDs, the appended object's index can differ.
        const auto projected_id = slot <= INT64_MAX ? std::optional(static_cast<std::int64_t>(slot))
                                                    : std::nullopt;
        definitions->slots.emplace(slot, saves::SavedTerrainFeature{
            row.boolean("NaturalWonder"), row.text("Type").value_or(""), projected_id});
        return true;
    }, {maximum_definitions});
    if (!collection.all_succeeded())
        throw std::runtime_error("Cannot load saved terrain Features: " + collection.error);
    definitions->span = collection.slots;
    const auto count = collection.rows;
    return {count, [definitions = std::move(definitions)](const saves::SavedMapCellProjection &cell)
        -> std::optional<saves::SavedTerrainFeature> {
        // A decoded source plot carries one wire identity, never two. Do not
        // silently select a side if a caller constructs a contradictory cell.
        if (cell.feature_type && cell.feature_type_hash)
            return {};
        std::optional<std::int64_t> id = cell.feature_type;
        if (cell.feature_type_hash) {
            if (*cell.feature_type_hash > UINT32_MAX)
                return {};
            const auto *found = definitions->types.find_hash(*cell.feature_type_hash);
            if (!found)
                return {};
            id = found->id;
        }
        if (!id || *id < 0)
            return {};
        const auto found = definitions->slots.find(static_cast<std::uint64_t>(*id));
        if (found != definitions->slots.end()) return found->second;
        // A known null slot follows the original null-info classification.
        // An ID outside the loaded collection remains unresolved.
        if (static_cast<std::uint64_t>(*id) < definitions->span)
            return saves::SavedTerrainFeature{false, {}, *id};
        return {};
    }};
}
} // namespace civ5::content
