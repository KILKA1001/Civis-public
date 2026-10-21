#pragma once

#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/bnw_plot_unit_validity_preflight.hpp"
#include <map>
#include <optional>
#include <string>

namespace civ5::runtime {

enum class BnwUnitDomain { land,sea,air,immobile,hover };
struct BnwPlotDomainUnitRule {
    std::string type;
    std::uint32_t hash=0;
    BnwUnitDomain domain=BnwUnitDomain::land;
};
struct BnwPlotDomainImprovementRule {
    std::string type;
    std::uint32_t hash=0;
    bool allows_walk_water=false;
};
struct BnwPlotDomainRules {
    std::map<std::int64_t,BnwPlotDomainUnitRule> units;
    std::map<std::int64_t,BnwPlotDomainImprovementRule> improvements;
};
struct BnwPlotDomainVerdict {
    std::optional<bool> valid;
    std::string reason;
    std::optional<BnwUnitDomain> effective_domain;
    bool water=false,city=false,hovering=false,all_terrain=false;
    bool embarked=false,walk_water=false;
};

// Bind numeric CvUnit::m_eUnitType and CvPlot improvement IDs to the active
// BNW InfoTypeCatalog and SQL rows. Unknown rules or collisions reject.
BnwPlotDomainRules load_bnw_plot_domain_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types);

// Exact CvPlot::isValidDomainForLocation branch. Source uses only plot type
// for isWater, not terrain ID; city is resolved from the saved plot-city
// handle and current player city collection. nullopt means an essential input
// cannot be proven. This is read-only and does not grant territory entry.
BnwPlotDomainVerdict evaluate_bnw_plot_valid_domain(
    const LiveSessionState &state,std::uint64_t plot_index,
    saves::SavedPlotUnitReference handle,
    const BnwPlotDomainRules &rules);

// Captures an immutable active-rule snapshot for BnwPlotUnitReadServices.
BnwUnitValidityProbe bind_bnw_plot_valid_domain(BnwPlotDomainRules rules);

} // namespace civ5::runtime
