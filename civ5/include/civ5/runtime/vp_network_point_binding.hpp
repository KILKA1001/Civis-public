#pragma once
#include "civ5/runtime/vp_network_points.hpp"
#include "civ5/runtime/maintenance.hpp"
#include "civ5/runtime/treasury.hpp"

namespace civ5::runtime {
// Resolve the VP-only rate inputs from one imported live world and the selected
// modded rules database. No common city/player schema or turn state is changed.
VpNetworkPointRateInputs bind_vp_network_point_rate_inputs(
    const MaintenanceWorld &world,
    const TreasuryContext &treasury,const TreasuryRules &treasury_rules,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    const content::CustomModOptions &options,std::int64_t spy_owner,
    std::size_t spy_index,const VpSpyCityBinding &target);
} // namespace civ5::runtime
