#pragma once
#include "civ5/runtime/bnw_unit_capture_death_quote.hpp"
#include "civ5/runtime/bnw_unit_kill_entry.hpp"

namespace civ5::runtime {

struct BnwUnitKillFrontier {
    BnwUnitKillEntry entry;
    BnwUnitRemoval removal;
    BnwUnitClassDeath class_death;
    BnwUnitSupportDeath support_death;
    BnwDeathXpQuote xp;
    BnwUnitCaptureDeathQuote capture;
    // Earliest missing CvUnit::kill effect is CanSaveUnit, which can cancel.
    // Following getCaptureDefinition, setXY/removeUnit/resources and more
    // remain unported. Thus no gameplay commit is ever authorized here.
    bool can_commit_kill=false;
};

// One source-bound, source-order diagnostic plan. Rules must come from the
// activated BNW database; this accepts only the NO_PLAYER capture branch.
BnwUnitKillFrontier plan_bnw_unit_kill_frontier(
    const saves::SavedSessionSnapshot &source,
    std::uint64_t player_index,std::uint64_t unit_index,
    std::int64_t killer,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules,
    const BnwDeathXpRules &xp_rules);

// Atomic detached projection of the proved counters plus the ID29 roster
// shape. The ID29 removal is applied last, matching deleteUnit order; absent
// intervening SDK effects are explicitly outside this diagnostic result.
saves::SavedSessionSnapshot project_bnw_unit_kill_frontier_diagnostic(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitKillFrontier &frontier,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules,
    const BnwDeathXpRules &xp_rules);

} // namespace civ5::runtime
