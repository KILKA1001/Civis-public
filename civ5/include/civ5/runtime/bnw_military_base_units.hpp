#pragma once

#include "civ5/runtime/live_session.hpp"
#include <map>

namespace civ5::runtime {

enum class BnwMilitaryUnitDomain { land, sea, air, hover, other };
struct BnwMilitaryUnitRule {
    BnwMilitaryUnitDomain domain=BnwMilitaryUnitDomain::other;
    std::int64_t id=-1, ranged_combat=0, range=0, moves=0;
};
struct BnwMilitaryBaseUnitRules {
    bool resolved=false;
    std::int64_t explore_ai=-1, explore_sea_ai=-1;
    std::map<std::uint32_t,BnwMilitaryUnitRule> units;
};
struct BnwMilitaryBaseUnitCounts {
    std::int64_t land=0, ranged_land=0, mobile_land=0, air=0;
    std::int64_t anti_air=0, melee_land=0, naval=0;
    std::int64_t land_in_armies=0, naval_in_armies=0;
};

// Expansion2 CvMilitaryAI.cpp:2054 UpdateBaseData, unit-count loop only.
// Validates the selected active SQL and catalog, including the two explorer
// UnitAI IDs. No saved strategy counter is trusted as a current unit count.
BnwMilitaryBaseUnitRules load_bnw_military_base_unit_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines);

// The caller must attest that owned unit membership is current after load and
// all prior actions. This does not compute reserves, threat or navy size and
// cannot by itself enable DoDeficit.
BnwMilitaryBaseUnitCounts count_bnw_military_base_units(
    const LiveSessionState &state,std::int64_t player,
    const BnwMilitaryBaseUnitRules &rules,bool unit_roster_current);

} // namespace civ5::runtime
