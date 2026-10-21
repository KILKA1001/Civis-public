#pragma once
#include "civ5/runtime/vp_network_point_binding.hpp"
#include "civ5/runtime/vp_espionage_turn.hpp"
#include "civ5/runtime/vp_city_construction_intrigue.hpp"
#include "civ5/runtime/vp_diplomacy_intrigue.hpp"

namespace civ5::runtime {
struct VpSchmoozeStep {
    std::int64_t spy_owner=-1,target_owner=-1,city=-1;
    std::size_t spy_index=0;
    VpNetworkPointRateQuote rate;
    std::optional<VpDiplomatNetworkPointPreview> network_points;
    // True only when the live VP owner has no attack operations at all, so
    // UncoverIntrigue's first branch cannot emit a sneak-attack message.
    bool no_sneak_attack_operations=false;
    std::optional<std::size_t> target_army_count;
    std::optional<bool> building_land_army,building_naval_army,building_combined_army;
    std::optional<bool> target_human_for_units;
    bool military_might_branch_skipped=false;
    // Source branch candidate only; AddIntrigueMessage has not run.
    std::string building_army_intrigue_candidate;
    std::optional<bool> building_army_intrigue_duplicate;
    std::optional<VpConstructionIntrigueScan> construction_scan;
    std::optional<VpDiplomaticIntrigueScan> diplomatic_scan;
    std::optional<bool> diplomatic_intrigue_duplicate;
    // Later VP actions can remain unresolved after safe network-point progress.
    std::string unresolved;
};
// VP ProcessSpy's diplomat branch through the first action needing an effect
// host. Safe network points are applied before promotion and intrigue, in the
// source GameCore's order. The selected SQL/XML rules set rate and thresholds.
VpSchmoozeStep process_vp_schmooze_without_effect_host(
    const MaintenanceWorld &world,CityYieldContext &cities,
    const TreasuryContext &treasury,const TreasuryRules &treasury_rules,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    const content::CustomModOptions &options,std::int64_t spy_owner,std::size_t spy_index);
} // namespace civ5::runtime
