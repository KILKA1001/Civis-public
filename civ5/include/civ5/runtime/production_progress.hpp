#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
// Active Info IDs are sparse and wide. Source array spans are import checks,
// never host allocation limits or permanent mod capacities.
struct ProductionProgressRules {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    std::set<std::int64_t> units,buildings,projects,processes,specialists;
    std::uint64_t unit_slots=0,building_slots=0,project_slots=0;
    // Preserve each table's active source slot and Type, including NULL and
    // duplicate names that the global Info catalog may overwrite.
    std::map<std::string,std::map<std::int64_t,std::optional<std::string>>> info_identity;
};
ProductionProgressRules load_production_progress_rules(const content::RulesDatabase&,
    content::TechnologyProfile);
void restore_production_progress(const saves::SavedSessionSnapshot&,
    const ProductionProgressRules&,const content::InfoTypeCatalog&,CityYieldContext&);
// The stored progress of the current order, before this turn's yield is added.
// A process or empty queue has no per-item production cache.
std::int64_t current_production_progress_times100(const CityYieldState&,
    const ProductionProgressRules&);
} // namespace civ5::runtime
