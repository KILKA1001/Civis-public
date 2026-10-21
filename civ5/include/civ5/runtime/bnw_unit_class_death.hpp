#pragma once
#include "civ5/runtime/bnw_unit_roster_checkpoint.hpp"
#include "civ5/content/rules_database.hpp"
#include <map>

namespace civ5::runtime {

struct BnwUnitClassBinding {
    std::int64_t unit_type=-1,unit_class=-1;
    std::uint32_t unit_hash=0,class_hash=0;
};
struct BnwUnitClassDeathRules {
    std::uint64_t class_slots=0;
    std::vector<std::uint32_t> class_hashes;
    std::map<std::int64_t,BnwUnitClassBinding> units;
};

// Units.Class / UnitClasses from the activated BNW DB, with Info catalog
// ID/hash validation and complete dense rosters.
BnwUnitClassDeathRules load_bnw_unit_class_death_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types);

struct BnwUnitClassDeath {
    BnwUnitRemoval removal;
    std::int64_t team=-1,unit_class=-1;
    std::int64_t team_count_before=0,player_count_before=0;
};

// CvUnit::kill(false), Expansion2 CvUnit.cpp after clearCombat/setReconPlot:
// team.changeUnitClassCount then player.changeUnitClassCount. This is only
// the two detached counters. Earlier and later kill effects remain blockers.
BnwUnitClassDeath plan_bnw_unit_class_death(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitRemoval &removal,
    const BnwUnitClassDeathRules &rules);

saves::SavedSessionSnapshot project_bnw_unit_class_death(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitClassDeath &plan,
    const BnwUnitClassDeathRules &rules);

} // namespace civ5::runtime
