#pragma once

#include "civ5/runtime/bnw_plot_domain_binding.hpp"
#include <memory>
#include "civ5/runtime/bnw_unit_territory_probe.hpp"
#include "civ5/runtime/bnw_unit_visibility_probe.hpp"

namespace civ5::runtime {
class BnwNativeUnitAllocatorCore;

// Assemble the currently proved, read-only BNW plot-unit predicates. The
// owned unit registry and plot roster are rechecked on each query. Branches
// requiring a unit move, death, or unresolved visibility still reject.
BnwPlotUnitReadServices make_bnw_plot_unit_no_effect_services(
    BnwPlotDomainRules domain_rules,
    const content::RulesDatabase *active_rules);

// Add the canonical allocator's actual imported friendly count and live
// transport lookup. Foreign actors remain unresolved; callback keeps weak
// lineage only and validates the SAME state at every use.
BnwPlotUnitReadServices make_bnw_plot_unit_no_effect_services(
    BnwPlotDomainRules domain_rules,
    const content::RulesDatabase *active_rules,
    const std::shared_ptr<const BnwNativeUnitAllocatorCore>&allocator);

} // namespace civ5::runtime
