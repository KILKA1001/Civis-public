#include "civ5/runtime/bnw_fresh_feature_effects.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

namespace civ5::runtime {
namespace {
std::int32_t source_int(std::int64_t value) {
    if (value < std::numeric_limits<std::int32_t>::min() ||
        value > std::numeric_limits<std::int32_t>::max())
        throw std::invalid_argument("Selected feature yield is outside SDK int32");
    return static_cast<std::int32_t>(value);
}
} // namespace

BnwFreshFeatureYieldInfo load_bnw_fresh_feature_yield_info(
    const content::RulesDatabase &rules, const BnwFreshFeatureSetInfo &feature) {
    if (feature.source_rules != &rules || feature.id < 0)
        throw std::invalid_argument("Selected feature yield Info identity differs");
    BnwFreshFeatureYieldInfo result;
    result.source_rules = &rules;
    result.id = feature.id;
    std::optional<std::string> type;
    const auto row = rules.query_rows(
        {"SELECT ROWID AS SourceRow, ID, Type, YieldNotAdditive, "
         "Impassable, Rough, NaturalWonder FROM Features WHERE ID = ? ORDER BY ROWID",
         {static_cast<std::int64_t>(feature.id)}},
        [&](const content::RulesRowView &view) {
            if (type || view.integer("SourceRow") != feature.id ||
                view.integer("ID") != feature.id ||
                view.boolean("Impassable") != feature.impassable ||
                view.boolean("Rough") != feature.rough ||
                view.boolean("NaturalWonder") != feature.natural_wonder)
                throw std::invalid_argument("Selected feature yield Info row differs");
            type = view.text("Type");
            result.yield_not_additive = view.boolean("YieldNotAdditive");
        }, 2);
    if (!row.all_succeeded() || !type || type->empty())
        throw std::invalid_argument("Selected feature yield Info is unresolved: " +
                                    row.error);
    const auto relation = [&](std::string_view table,
                              std::array<std::int32_t, 6> &target) {
        std::array<bool, 6> seen{};
        const auto sql = "SELECT Yields.ID AS YieldID, Relation.Yield AS Amount "
            "FROM " + std::string(table) + " AS Relation JOIN Yields "
            "ON Relation.YieldType = Yields.Type "
            "WHERE Relation.FeatureType = ? ORDER BY Relation.ROWID";
        const auto rows = rules.query_rows({sql, {*type}},
            [&](const content::RulesRowView &view) {
                const auto id = view.integer("YieldID");
                if (id < 0 || id >= static_cast<std::int64_t>(target.size()) ||
                    seen[static_cast<std::size_t>(id)])
                    throw std::invalid_argument("Selected feature yield relation is ambiguous");
                seen[static_cast<std::size_t>(id)] = true;
                target[static_cast<std::size_t>(id)] =
                    source_int(view.integer("Amount"));
            });
        if (!rows.all_succeeded())
            throw std::invalid_argument("Selected feature yield relation failed: " +
                                        rows.error);
    };
    relation("Feature_YieldChanges", result.base);
    relation("Feature_RiverYieldChanges", result.river);
    relation("Feature_HillsYieldChanges", result.hills);
    std::array<bool, 6> seen_yields{};
    constexpr std::array<std::string_view, 6> yield_names{
        "YIELD_FOOD", "YIELD_PRODUCTION", "YIELD_GOLD", "YIELD_SCIENCE",
        "YIELD_CULTURE", "YIELD_FAITH"};
    const auto yields = rules.query_rows(
        {"SELECT ROWID AS SourceRow, ID, Type, MountainChange FROM Yields ORDER BY ROWID", {}},
        [&](const content::RulesRowView &view) {
            const auto id = view.integer("ID");
            if (id < 0 || id >= static_cast<std::int64_t>(result.mountain.size()) ||
                view.integer("SourceRow") != id ||
                view.text("Type") != std::optional<std::string>(
                    std::string(yield_names[static_cast<std::size_t>(id)])) ||
                seen_yields[static_cast<std::size_t>(id)])
                throw std::invalid_argument("Selected mountain yield Info is ambiguous");
            seen_yields[static_cast<std::size_t>(id)] = true;
            result.mountain[static_cast<std::size_t>(id)] =
                source_int(view.integer("MountainChange"));
        }, result.mountain.size() + 1);
    if (!yields.all_succeeded() ||
        !std::all_of(seen_yields.begin(), seen_yields.end(),
                     [](bool present) { return present; }))
        throw std::invalid_argument("Selected mountain yield Info is incomplete: " +
                                    yields.error);
    return result;
}

void BnwFreshFeatureEffectDraft::rollback() noexcept {
    after.reset();
    yield_symbol_after.reset();
}

BnwFreshFeatureEffectDraft draft_bnw_fresh_feature_after_callback(
    const BnwFreshFeatureChangedEvent &event,
    const BnwSetupPlotGrid &grid,
    const BnwFreshAreaStructure &areas,
    const BnwFreshYieldRules &terrain_yields,
    const BnwFreshFeatureSetInfo &feature,
    const BnwFreshFeatureYieldInfo &feature_yields,
    bool empty_world_verified,
    std::int32_t map_natural_wonders_before,
    std::int32_t area_natural_wonders_before,
    std::uint64_t maximum_symbol_events) {
    if (!empty_world_verified || event.plot_index() >= grid.size() ||
        event.before() != grid.at(event.plot_index()) ||
        event.old_feature() != -1 || event.new_feature() != feature.id ||
        event.variety() != 0 || !event.source_rules() ||
        feature.source_rules != event.source_rules() ||
        feature_yields.source_rules != event.source_rules() ||
        feature_yields.id != feature.id ||
        terrain_yields.source_rules != event.source_rules() ||
        identify_bytes(event.source_rules()->gameplay_fingerprint_data()).sha256 !=
            event.rules_sha256() ||
        areas.area_ids.size() != grid.size() ||
        areas.landmass_ids.size() != grid.size() ||
        map_natural_wonders_before < 0 || area_natural_wonders_before < 0)
        throw std::invalid_argument("Fresh feature suffix lacks selected empty-world identity");
    const auto &before = event.before();
    if (before.area_id < 0 || before.landmass_id < 0 ||
        before.area_id != areas.area_ids[event.plot_index()] ||
        before.landmass_id != areas.landmass_ids[event.plot_index()] ||
        before.terrain_type < 0 ||
        !before.terrain_type_assigned_by_script ||
        !before.plot_type_assigned_by_script ||
        !terrain_yields.terrains.contains(before.terrain_type))
        throw std::invalid_argument("Fresh feature suffix plot or terrain is incomplete");
    (void)find_bnw_fresh_area(areas, before.area_id);
    const BnwFreshLandmass *landmass = nullptr;
    for (const auto &candidate : areas.landmasses) {
        if (candidate.id != before.landmass_id) continue;
        if (landmass) throw std::invalid_argument("Fresh feature landmass is ambiguous");
        landmass = &candidate;
    }
    if (!landmass || landmass->tiles <= 0)
        throw std::invalid_argument("Fresh feature landmass is missing");
    if (calculate_bnw_fresh_plot_yields(before, landmass->lake,
                                         terrain_yields) != before.yields)
        throw std::invalid_argument("Fresh feature pre-write yield cache differs");

    BnwFreshFeatureEffectDraft draft;
    draft.event_sequence = event.sequence();
    draft.before = {before, -1, 0, false,
                    map_natural_wonders_before, area_natural_wonders_before};
    auto after = draft.before;
    // CvPlot::setFeatureType writes m_eFeatureType before updateYield.
    after.feature_type = feature.id;
    draft.source_order.push_back(BnwFreshFeatureEffectStep::feature_written);

    const auto &terrain = terrain_yields.terrains.at(before.terrain_type);
    const bool hills = before.plot_type == 1;
    const bool river = before.river_crossing_count > 0;
    for (std::size_t yield = 0; yield < after.plot.yields.size(); ++yield) {
        std::int64_t value = 0;
        // updateYield runs before updateImpassable. The nature-yield guard
        // therefore observes the OLD cached impassability, but the NEW
        // feature ID. Natural wonders are exempt from that guard.
        if (before.potential_city_work &&
            ((!before.impassable && before.plot_type != 0) ||
             feature.natural_wonder)) {
            value = static_cast<std::int64_t>(terrain.base[yield]) +
                (hills ? terrain_yields.yields[yield].hills : 0) +
                (before.plot_type == 0 ? feature_yields.mountain[yield] : 0) +
                (landmass->lake ? terrain_yields.yields[yield].lake : 0);
            if (feature_yields.yield_not_additive)
                value = feature_yields.base[yield];
            else
                value += feature_yields.base[yield];
            if (river) value += feature_yields.river[yield];
            if (hills) value += feature_yields.hills[yield];
            value = std::max<std::int64_t>(0, value);
        }
        if (value > std::numeric_limits<std::int16_t>::max())
            throw std::overflow_error("Fresh feature yield exceeds CvPlot short cache");
        after.plot.yields[yield] = static_cast<std::int16_t>(value);
    }
    draft.source_order.push_back(BnwFreshFeatureEffectStep::yield_recomputed);
    if (after.plot.yields != before.yields) {
        if (!maximum_symbol_events)
            throw std::length_error("Fresh feature updateSymbols budget exceeded");
        draft.yield_symbol_after = after.plot.yields;
        draft.source_order.push_back(BnwFreshFeatureEffectStep::yield_symbol_staged);
    }
    after.plot.impassable = feature.impassable;
    draft.source_order.push_back(BnwFreshFeatureEffectStep::impassability_updated);
    // updateSeeFromSight(true) traverses plots, but fresh InitPlots has no
    // owned plot, city, unit or recon source to change team visibility.
    draft.source_order.push_back(BnwFreshFeatureEffectStep::empty_sight_restored);
    after.variety = 0;
    draft.source_order.push_back(BnwFreshFeatureEffectStep::variety_forced_zero);
    after.rough = feature.rough;
    draft.source_order.push_back(BnwFreshFeatureEffectStep::rough_feature_checked);
    if (feature.natural_wonder) {
        if (after.map_natural_wonders == std::numeric_limits<std::int32_t>::max() ||
            after.area_natural_wonders == std::numeric_limits<std::int32_t>::max())
            throw std::overflow_error("Fresh natural wonder counter exceeds SDK int32");
        ++after.map_natural_wonders;
        draft.source_order.push_back(
            BnwFreshFeatureEffectStep::map_natural_wonders_incremented);
        ++after.area_natural_wonders;
        draft.source_order.push_back(
            BnwFreshFeatureEffectStep::area_natural_wonders_incremented);
    }
    draft.source_order.push_back(
        BnwFreshFeatureEffectStep::old_feature_removal_skipped);
    draft.after = after;
    draft.blockers = {"closed_engine_feature_callback_ack",
                      "atomic_feature_grid_area_yield_symbol_handoff"};
    return draft;
}

} // namespace civ5::runtime
