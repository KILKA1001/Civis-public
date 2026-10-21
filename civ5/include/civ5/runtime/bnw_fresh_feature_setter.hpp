#pragma once
#include "civ5/runtime/bnw_setup_plot_grid.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace civ5::runtime {

// Feature Info is read from the activated selected rules, never inferred from
// a Lua enum ID. The pointer pins the source database for the map session.
struct BnwFreshFeatureSetInfo {
    const content::RulesDatabase *source_rules = nullptr;
    std::int32_t id = -1;
    bool impassable = false, rough = false, natural_wonder = false;
};
BnwFreshFeatureSetInfo resolve_bnw_fresh_feature_set_info(
    const content::RulesDatabase &rules, std::int32_t feature_id);

enum class BnwFreshFeatureSetStep {
    remove_empty_sight, gameplay_feature_changed_pending,
    feature_write_pending, yield_update_pending, impassability_projected,
    restore_empty_sight_pending, variety_forced_zero,
    rough_feature_projected, natural_wonder_count_pending,
    old_improvement_removal_skipped
};
enum class BnwFreshFeatureSetBlocker {
    invalid_fresh_contract, required_services_pending
};
struct BnwFreshFeatureSetPreflight {
    BnwFreshFeatureSetBlocker blocker =
        BnwFreshFeatureSetBlocker::invalid_fresh_contract;
    std::size_t plot_index = 0;
    std::int32_t new_feature_id = -1;
    const content::RulesDatabase *source_rules = nullptr;
    std::optional<BnwSetupPlot> before;
    // Expansion2 CvPlot::setFeatureType overwrites every supplied variety.
    std::int32_t effective_variety = 0;
    bool projected_impassable = false;
    bool projected_rough = false;
    std::vector<BnwFreshFeatureSetStep> source_order;
    std::vector<std::string_view> missing_services;
};

// Only the first NO_FEATURE -> ordinary feature transition in detached
// AddFeatures. No owned state changes: the SDK graphics event and feature
// yield/visibility callbacks must be attached before Lua may write a feature.
BnwFreshFeatureSetPreflight inspect_bnw_fresh_feature_set(
    const BnwSetupPlotGrid &grid, const BnwFreshAreaStructure &areas,
    std::span<const std::int32_t> feature_types, std::size_t plot_index,
    std::int32_t new_feature_id, std::int32_t requested_variety,
    const BnwSelectedFeatureEligibility &eligibility,
    const BnwFreshFeatureSetInfo &info,
    std::int32_t minimum_water_size);

} // namespace civ5::runtime
