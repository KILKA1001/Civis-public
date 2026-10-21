#pragma once
#include "civ5/runtime/vp_plot_religion.hpp"
#include "civ5/runtime/vp_great_work_profile.hpp"

namespace civ5::runtime {
struct VpGreatWorkRules {
    struct Building {
        std::int64_t building_class=-1,slot=-1,yield=-1,slots=0;
        YieldValues theme_yields;
        std::vector<std::int64_t> themes; // SQL result order; no twelve-theme capacity.
    };
    struct Belief {std::int64_t minimum_population=0;YieldValues yields;};
    std::map<std::int64_t,Building> buildings;
    std::map<std::int64_t,Belief> beliefs;
    std::set<std::int64_t> yields;
    std::uint64_t building_slots=0,class_slots=0;
    std::int64_t tourism=6,culture_local=11,art=-1,artifact=-1;
    std::array<std::int64_t,5> slots{-1,-1,-1,-1,-1}; // art/artifact, literature, music, film, relic
};
VpGreatWorkRules load_vp_great_work_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
void restore_vp_great_works(const saves::SavedSessionSnapshot&,const VpGreatWorkRules&,CityYieldContext&);
class VpGreatWorkServices {
public:
    virtual ~VpGreatWorkServices()=default;
    virtual void unlock_capital_theming_achievement(std::int64_t player)=0; // ACHIEVEMENT_XP2_40
};
struct VpGreatWorkContext {
    CityYieldContext &cities;
    const CityYieldRules &city_rules;
    const VpGreatWorkRules &rules;
    const VpPlotReligionContext &religion;
    VpGreatWorkServices &services;
};
std::int64_t vp_building_from_class(const VpGreatWorkContext&,CityYieldState&,std::int64_t building_class,bool fallback=false);
std::int64_t vp_city_building_count(const VpGreatWorkContext&,const CityYieldState&,std::int64_t building);
bool vp_city_has_building_class(const VpGreatWorkContext&,const CityYieldState&,std::int64_t building_class);
std::int64_t vp_great_works_in_building(const VpGreatWorkContext&,const CityYieldState&,std::int64_t building_class);
std::int64_t vp_city_great_works(const VpGreatWorkContext&,CityYieldState&,bool ignore_yield=true);
std::int64_t vp_great_works_in_slot(const VpGreatWorkContext&,CityYieldState&,std::int64_t slot,bool artifact=false,bool art=false);
std::int64_t vp_themed_buildings(const VpGreatWorkContext&,const CityYieldState&);
std::int64_t vp_theming_multiplier(const VpGreatWorkContext&,const CityYieldState&);
std::int64_t vp_theming_bonus(const VpGreatWorkContext&,CityYieldState&,std::int64_t building_class);
std::int64_t vp_current_theming_bonuses(const VpGreatWorkContext&,const CityYieldState&,std::int64_t yield);
struct VpGreatWorkYield {
    std::int64_t works=0,matching_works=0,per_work=0,beliefs=0,secondary=0,pantheon=0,
        base_times100=0,tourism_times100=0,themes=0,type_bonus=0,extra_themes=0,total_times100=0;
    bool suppressed=false;
};
// Complete fresh building great-work yield, including source achievement requests.
// Does not update the full city yield, tourism influence or theming arrangement.
VpGreatWorkYield vp_great_work_yield(const VpGreatWorkContext&,CityYieldState&,std::int64_t yield);
std::int64_t vp_cached_great_work_yield(const VpGreatWorkContext&,CityYieldState&,std::int64_t yield);
void invalidate_vp_great_work_yields(CityYieldContext&,const CityYieldState&);
void invalidate_vp_building_class_cache(CityYieldContext&,const CityYieldState&);
} // namespace civ5::runtime
