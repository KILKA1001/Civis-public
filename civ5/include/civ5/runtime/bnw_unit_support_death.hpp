#pragma once
#include "civ5/runtime/bnw_unit_class_death.hpp"
#include <array>

namespace civ5::runtime {

struct BnwUnitSupportRule {
    std::int64_t unit_type=-1;
    std::uint32_t unit_hash=0;
    bool land=false,military_support=false;
    std::int64_t work_rate=0,unhappiness=0,extra_maintenance_cost=0,
                 nuke_damage_level=-1;
};
struct BnwUnitSupportRules {
    std::map<std::int64_t,BnwUnitSupportRule> units;
};

// Expansion2 CvUnitEntry::CacheResults Units columns with exact catalog ID,
// hash and Domain binding. Must use the same activated BNW database as the
// unit-class rules and imported save.
BnwUnitSupportRules load_bnw_unit_support_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const BnwUnitClassDeathRules &class_rules);

struct BnwUnitSupportDeath {
    BnwUnitClassDeath class_death;
    std::int64_t builder_delta=0,unhappiness_delta=0,extra_cost_delta=0,
                 nuke_delta=0,military_delta=0;
    std::array<std::int64_t,5> before{};
    // CvPlayer::changeExtraUnitCost and changeNumMilitaryUnits can set
    // GameData_DIRTY_BIT for the active player. Publication is not here.
    bool needs_active_player_ui_check=false;
};

// SDK order after team/player class counters: builders, unhappiness, extra
// cost, nuke count, military count. Earlier kill effects and later experience,
// capture, map, resources, maintenance, Lua/UI remain mandatory blockers.
BnwUnitSupportDeath plan_bnw_unit_support_death(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitClassDeath &class_death,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules);

saves::SavedSessionSnapshot project_bnw_unit_support_death(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitSupportDeath &plan,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules);

} // namespace civ5::runtime
