#pragma once
#include "civ5/runtime/vp_network_point_binding.hpp"
#include "civ5/runtime/vp_espionage_turn.hpp"

namespace civ5::runtime {
struct VpGatheringIntelStep {
    std::int64_t spy_owner=-1,target_owner=-1,city=-1;
    std::size_t spy_index=0;
    VpNetworkPointRateQuote rate;
    std::optional<VpNetworkPointPreview> network_points;
};
// Execute the VP ProcessSpy gathering-intel branch only when promotion,
// wonder discovery, passive bonuses and mission notifications are proven not
// to fire. The city is the same live object observed by the graphics layer.
// Other spy states and effectful branches require their own GameCore hosts.
VpGatheringIntelStep process_vp_gathering_intel_without_effect_host(
    const MaintenanceWorld &world,CityYieldContext &cities,
    const TreasuryContext &treasury,const TreasuryRules &treasury_rules,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    const content::CustomModOptions &options,std::int64_t spy_owner,std::size_t spy_index);
} // namespace civ5::runtime
