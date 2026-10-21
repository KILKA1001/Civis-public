#pragma once
#include "civ5/runtime/vp_city_base_yields.hpp"
#include "civ5/runtime/vp_unit_yield_profile.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_city_base_yields_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,runtime::CityYieldContext &cities,const runtime::CityYieldRules &city_rules,
    runtime::MaintenanceContext &units,const runtime::TreasuryContext &treasury,const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    const auto rules=runtime::load_vp_city_base_yield_rules(db,types);runtime::restore_vp_city_base_yields(snap,cities,units);
    const auto gr=runtime::load_vp_great_work_rules(db,types);const auto rr=runtime::load_vp_plot_religion_rules(db,types);const auto fr=runtime::load_vp_city_food_rules(db);
    const auto br=runtime::load_vp_city_building_yield_rules(db,types);const auto hr=runtime::load_vp_city_happiness_rules(db,types);const auto tr=runtime::load_vp_city_trait_yield_rules(db,types);
    const auto pass=runtime::restore_plot_passability(snap,db,types);const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snap);
    runtime::PlotWaterContext water{*snap.map_projection,ws,wr,types,pass,players,cities,defines,options};runtime::VpPlotReligionContext religion{rr,water};
    struct Services:runtime::VpGreatWorkServices{void unlock_capital_theming_achievement(std::int64_t)override{throw std::runtime_error("Unexpected base yield achievement");}} services;
    runtime::VpGreatWorkContext great{cities,city_rules,gr,religion,services};runtime::VpCityFoodContext food{cities,players,units.team_eras,fr,defines};
    runtime::VpCityBuildingYieldContext buildings{br,great,units.team_eras};runtime::VpCityHappinessContext happiness{hr,great,food,units,players,db};
    runtime::VpCityReligionMinorContext rm{religion,treasury};runtime::VpCityTraitYieldContext traits{tr,players,cities,treasury,units.team_eras,defines,*options};
    runtime::VpCityBaseYieldContext c{rules,buildings,happiness,rm,traits};
    std::ostringstream out;const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto named=[](const auto &v,const char *name)->const auto&{for(const auto &x:v)if(x.name&&std::string_view(x.name)==name)return x;throw std::runtime_error("Missing base yield source");};
    out<<"{\"applicable\":true,\"base_calculation_complete\":true,\"city_yield_updater_complete\":false,\"yield_ids\":["<<rules.culture<<','<<rules.tourism<<','<<rules.culture_local<<"],\"yields\":";list(rules.yields);
    out<<",\"units_enabled\":"<<options->enabled("YIELD_MODIFIER_FROM_UNITS")<<",\"world_wonder_classes\":";pairs(rules.world_wonder_classes);out<<",\"natural_wonders\":";pairs(rules.natural_wonders);
    out<<",\"buildings\":[";bool comma=false;for(const auto &[id,b]:rules.buildings){if(comma)out<<',';comma=true;out<<'['<<id<<','<<b.building_class<<','<<b.culture<<']';}
    out<<"],\"source_leagues\":["<<snap.leagues.start_offset<<','<<snap.leagues.end_offset<<"],\"leagues\":[";comma=false;for(const auto &l:cities.leagues){if(comma)out<<',';comma=true;out<<'[';list(l.members);out<<',';list(l.culture_per_wonder);out<<']';}
    const auto *profile=runtime::find_vp_city_base_yield_profile(cities);if(!profile)throw runtime::CityYieldUnavailable("Missing VP base-yield profile report state");
    out<<"],\"players\":[";comma=false;
    for(const auto &sp:snap.all_players->players){const auto owner=sp.saved_player_id;auto &p=cities.players.at(owner);const auto &a=profile->players.at(owner);
        if(comma)out<<',';comma=true;out<<"{\"id\":"<<owner<<",\"total_population\":"<<p.total_population<<",\"landmarks_tourism\":"<<a.landmarks_tourism<<",\"state_religion\":";pairs(a.state_religion);
        out<<",\"non_specialists\":";pairs(a.non_specialists);out<<",\"capital_tiles\":";pairs(a.capital_tiles);out<<",\"cities\":[";bool inner=false;
        for(const auto &sc:sp.cities.cities){const auto cid=named(sc.core.fields,"m_iID").values.at(0);auto &city=p.cities.at(cid);const auto &b=profile->cities.at({owner,cid});
            if(inner)out<<',';inner=true;out<<"{\"id\":"<<cid<<",\"world_wonders\":"<<b.world_wonders<<",\"landmarks_tourism\":"<<b.landmarks_tourism<<",\"working\":";list(b.working);out<<",\"worked_plots\":";list(b.worked_plots);
            const char *names[]={"corporations","events","league","empire_population","happiness","health","previous_golden_ages"};
            const runtime::YieldValues *v[]={&b.corporations,&b.events,&b.league,&b.empire_population,&b.happiness,&b.health,&b.previous_golden_ages};
            for(std::size_t k=0;k<7;++k){out<<",\""<<names[k]<<"\":";pairs(*v[k]);}
            out<<",\"raw_yields\":[";bool row=false;for(auto y:rules.yields){const auto &v=city.yields.at(y);if(row)out<<',';row=true;out<<'['<<y<<','<<v.terrain<<','<<v.specialists<<','<<v.misc<<','<<v.per_population<<']';}
            const auto l=runtime::vp_city_landmark_tourism(c,city);out<<"],\"landmarks\":["<<l.percent<<','<<l.wonders<<','<<l.natural_wonders<<','<<l.terrain<<','<<l.total_times100<<"],\"quotes\":[";row=false;
            for(auto y:rules.yields){const auto q=runtime::vp_city_base_yield(c,city,y);if(row)out<<',';row=true;
                out<<'['<<y;for(auto n:{q.local_culture,q.great_works,q.terrain,q.buildings,q.policies,q.specialists,q.religion,q.minors,q.traits,q.corporations,q.events,q.connection,q.league,q.landmarks,q.units,q.state_religion,q.population,q.empire_population,q.industrial,q.non_specialists,q.misc,q.previous_golden_ages,q.capital_tiles,q.total_times100})out<<','<<n;out<<']';}
            out<<"]}";
        }out<<"],\"units\":[";inner=false;for(const auto &su:sp.units.units){const auto uid=named(su.core.fields,"m_iID").values.at(0);const auto &u=units.players.at(owner).units.at(uid);
            if(inner)out<<',';inner=true;out<<"{\"id\":"<<uid<<",\"source_core\":["<<su.core.start_offset<<','<<su.core.end_offset<<"],\"source_tail\":["<<su.tail.start_offset<<','<<su.tail.end_offset<<"],\"yields\":";pairs(*runtime::vp_unit_yield_profile(u).city_yield_change);out<<'}';}
        out<<"]}";
    }out<<"]}";return out.str();
}
} // namespace civ5::validation
