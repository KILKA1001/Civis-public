#pragma once

#include "civ5/runtime/bnw_deficit_selection.hpp"
#include "civ5/content/rules_database.hpp"
#include <string>
#include <vector>

namespace civ5::runtime {

struct BnwDeficitRuleLoad {
    BnwDeficitRules rules;
};

// Bind active BNW Units, Domains, Defines and InfoTypeCatalog. The intrinsic
// power calculation follows CvUnitEntry::DoUpdatePower, including free
// promotion modifiers in source Info ID order. Never read an invented
// Units.Power SQL column. Validates the catalog ID/hash/table against active
// SQL; unsupported or ambiguous modifier rows block the whole load.
// This does not restore military AI, plot fighting state or unit scrap effects.
BnwDeficitRuleLoad load_bnw_deficit_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines);

} // namespace civ5::runtime
