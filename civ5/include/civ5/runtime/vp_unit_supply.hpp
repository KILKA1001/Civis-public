#pragma once
#include "civ5/runtime/maintenance.hpp"
#include "civ5/runtime/vp_city_supply_profile.hpp"
#include "civ5/runtime/vp_unit_supply_profile.hpp"

namespace civ5::runtime {
struct VpUnitSupplyRules {
    struct Bonus {std::int64_t base=0,city=0,population=0,era_flat=0,era_modifier=0,percent=0;};
    struct Handicap {Bonus player,ai,minor;};
    std::map<std::int64_t,Handicap> handicaps;
    YieldValues era_base,minor_traits;
    std::map<std::int64_t,std::string> minor_types;
};
VpUnitSupplyRules load_vp_unit_supply_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
void restore_vp_unit_supply(const saves::SavedSessionSnapshot&,const content::InfoTypeCatalog&,
    const ResearchContext&,PlayerResearchContext&,CityYieldContext&,MaintenanceContext&);
struct VpUnitSupplyContext {
    MaintenanceContext &units;
    ResearchContext &players;
    PlayerResearchContext &diplomacy;
    CityYieldContext &cities;
    std::span<const TeamTechnologies> technologies;
    const VpUnitSupplyRules &rules;
    const content::RulesDatabase &database;
    const content::GlobalDefineCache &defines;
    const content::CustomModOptions &options;
};
bool vp_can_change_war_peace(VpUnitSupplyContext&,std::int64_t team,std::int64_t other);
std::int64_t vp_highest_war_weariness_percent(VpUnitSupplyContext&,std::int64_t player);
struct VpUnitSupplyComponents {std::int64_t handicap=0,cities=0,population=0,expended=0;};
VpUnitSupplyComponents vp_unit_supply_components(const VpUnitSupplyContext&,std::int64_t player,bool ignore_reduction=false);
struct VpUnitSupplyQuote {
    VpUnitSupplyComponents components;
    std::int64_t effective_cities=0,city_reduction=0,empire_modifier=0,minor_modifier=0,extra_city_modifier=0;
    std::int64_t base=0,war_percent=0,capacity=0,units=0,over=0;
    bool unlimited=false,cached=false,minor_unready=false;
};
// Populates both unsaved capacity caches on a miss. Component diagnostics are
// present only on that path. Barbarians have semantic unlimited supply.
VpUnitSupplyQuote vp_unit_supply(VpUnitSupplyContext&,std::int64_t player,bool check_war_weariness=true);
void invalidate_vp_unit_supply(MaintenanceContext&,std::int64_t player);
} // namespace civ5::runtime
