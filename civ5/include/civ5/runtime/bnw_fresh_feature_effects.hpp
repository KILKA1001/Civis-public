#pragma once
#include "civ5/runtime/bnw_fresh_feature_events.hpp"
#include "civ5/runtime/bnw_fresh_yield.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshFeatureYieldInfo {
    const content::RulesDatabase *source_rules = nullptr;
    std::int32_t id = -1;
    bool yield_not_additive = false;
    std::array<std::int32_t, 6> base{}, river{}, hills{};
    std::array<std::int32_t, 6> mountain{};
};
BnwFreshFeatureYieldInfo load_bnw_fresh_feature_yield_info(
    const content::RulesDatabase &rules, const BnwFreshFeatureSetInfo &feature);

enum class BnwFreshFeatureEffectStep {
    feature_written, yield_recomputed, yield_symbol_staged,
    impassability_updated, empty_sight_restored, variety_forced_zero,
    rough_feature_checked, map_natural_wonders_incremented,
    area_natural_wonders_incremented, old_feature_removal_skipped
};

struct BnwFreshFeatureProjectedState {
    BnwSetupPlot plot;
    std::int32_t feature_type = -1;
    std::int32_t variety = 0;
    bool rough = false;
    std::int32_t map_natural_wonders = 0;
    std::int32_t area_natural_wonders = 0;
};

struct BnwFreshFeatureEffectDraft {
    std::uint64_t event_sequence = 0;
    BnwFreshFeatureProjectedState before;
    std::optional<BnwFreshFeatureProjectedState> after;
    std::vector<BnwFreshFeatureEffectStep> source_order;
    std::optional<std::array<std::int16_t, 6>> yield_symbol_after;
    // The closed-engine callback and live-world event/counter handoff still
    // lack proof. This speculative draft never authorizes Lua success.
    std::vector<std::string_view> blockers;
    bool can_publish() const noexcept { return false; }
    void rollback() noexcept;
};

// Models only CvMap::InitPlots state: no owner/city/unit/recon/resource/route/
// improvement, no plot-extra yield, and no religious/gameplay modifiers.
// The caller must have verified that empty-world precondition separately.
// This computes the SDK suffix after an imagined synchronous callback ack;
// it does not acknowledge the callback or mutate the supplied owned grid.
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
    std::uint64_t maximum_symbol_events);

} // namespace civ5::runtime
