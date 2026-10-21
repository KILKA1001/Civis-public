#include "civ5/runtime/bnw_fresh_feature_setter.hpp"
#include "civ5/content/rules_database.hpp"
#include <algorithm>
#include <optional>
#include <stdexcept>

namespace civ5::runtime {

BnwFreshFeatureSetInfo resolve_bnw_fresh_feature_set_info(
    const content::RulesDatabase &rules, std::int32_t feature_id) {
    if (feature_id < 0)
        throw std::invalid_argument("Fresh BNW feature setter needs a selected feature ID");
    std::optional<BnwFreshFeatureSetInfo> info;
    const auto report = rules.query_rows(
        {"SELECT ROWID AS CivisRowId, ID, Impassable, Rough, NaturalWonder "
         "FROM Features WHERE ID = ? ORDER BY ROWID",
         {static_cast<std::int64_t>(feature_id)}},
        [&](const content::RulesRowView &row) {
            if (info || !row.has_column("CivisRowId") ||
                !row.has_column("ID") || !row.has_column("Impassable") ||
                !row.has_column("Rough") || !row.has_column("NaturalWonder") ||
                row.integer("CivisRowId") != feature_id ||
                row.integer("ID") != feature_id)
                throw std::invalid_argument("Selected BNW feature setter Info is ambiguous or incomplete");
            info = BnwFreshFeatureSetInfo{
                &rules, feature_id, row.boolean("Impassable"),
                row.boolean("Rough"), row.boolean("NaturalWonder")};
        }, 2);
    if (!report.all_succeeded() || !info)
        throw std::invalid_argument("Selected BNW feature setter Info is unresolved: " +
                                    report.error);
    return *info;
}

BnwFreshFeatureSetPreflight inspect_bnw_fresh_feature_set(
    const BnwSetupPlotGrid &grid, const BnwFreshAreaStructure &areas,
    std::span<const std::int32_t> feature_types, std::size_t plot_index,
    std::int32_t new_feature_id, std::int32_t requested_variety,
    const BnwSelectedFeatureEligibility &eligibility,
    const BnwFreshFeatureSetInfo &info,
    std::int32_t minimum_water_size) {
    (void)requested_variety;
    BnwFreshFeatureSetPreflight result;
    result.plot_index = plot_index;
    result.new_feature_id = new_feature_id;
    if (plot_index >= grid.size() || feature_types.size() != grid.size() ||
        areas.area_ids.size() != grid.size() ||
        areas.landmass_ids.size() != grid.size() ||
        new_feature_id < 0 || info.id != new_feature_id ||
        eligibility.id != new_feature_id || !info.source_rules ||
        eligibility.source_rules != info.source_rules ||
        !std::all_of(feature_types.begin(), feature_types.end(),
                     [](std::int32_t value) { return value == -1; }))
        return result;
    const auto &plot = grid.at(plot_index);
    if (plot.area_id < 0 || plot.area_id != areas.area_ids[plot_index] ||
        plot.landmass_id < 0 || plot.landmass_id != areas.landmass_ids[plot_index] ||
        plot.terrain_type < 0 || !plot.terrain_type_assigned_by_script)
        return result;
    try {
        (void)find_bnw_fresh_area(areas, plot.area_id);
        if (!can_have_bnw_fresh_feature(
                grid, plot_index, new_feature_id, eligibility, areas,
                minimum_water_size, feature_types))
            return result;
    } catch (const std::invalid_argument &) {
        return result;
    }
    // CvPlot::reset has no owners, cities, units, recon sources or features.
    // AddFeatures has no API that can create them; updateSeeFromSight(false)
    // therefore traverses empty updateSight branches. The following SDK
    // callback is unconditional even when no graphics host is initialized.
    result.source_rules = info.source_rules;
    result.before = plot;
    result.projected_impassable = info.impassable;
    result.projected_rough = info.rough;
    result.source_order = {
        BnwFreshFeatureSetStep::remove_empty_sight,
        BnwFreshFeatureSetStep::gameplay_feature_changed_pending,
        BnwFreshFeatureSetStep::feature_write_pending,
        BnwFreshFeatureSetStep::yield_update_pending,
        BnwFreshFeatureSetStep::impassability_projected,
        BnwFreshFeatureSetStep::restore_empty_sight_pending,
        BnwFreshFeatureSetStep::variety_forced_zero,
        BnwFreshFeatureSetStep::rough_feature_projected
    };
    if (info.natural_wonder)
        result.source_order.push_back(
            BnwFreshFeatureSetStep::natural_wonder_count_pending);
    result.source_order.push_back(
        BnwFreshFeatureSetStep::old_improvement_removal_skipped);
    result.missing_services = {
        "GameplayFeatureChanged callback before feature write",
        "selected feature yield calculation and ordered updateSymbols",
        "atomic feature/impassable/rough state handoff"
    };
    if (info.natural_wonder)
        result.missing_services.push_back(
            "map and area natural-wonder counters");
    result.blocker = BnwFreshFeatureSetBlocker::required_services_pending;
    return result;
}

} // namespace civ5::runtime
