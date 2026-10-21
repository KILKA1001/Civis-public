#pragma once
#include "civ5/runtime/plot_water.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string plot_water_report(saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &database,
        const content::InfoTypeCatalog &types,const runtime::ResearchContext &players,runtime::CityYieldContext &cities,
        const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(!snapshot.map_projection)throw std::runtime_error("Water diagnostic needs the shared map");
    const auto rules=runtime::load_plot_water_rules(database,players.profile);
    const auto state=runtime::restore_plot_water(snapshot);
    const auto pass=runtime::restore_plot_passability(snapshot,database,types);
    const bool vp=players.profile==content::TechnologyProfile::vp542;
    if(vp)runtime::restore_vp_water_city_buildings(snapshot,cities,rules);
    auto &map=*snapshot.map_projection;
    runtime::PlotWaterContext c{map,state,rules,types,pass,players,cities,defines,options};
    if(vp)runtime::reset_plot_water_cache(c);
    const auto resolve=[&](const auto &id,const auto &hash){
        if(id&&hash)throw std::runtime_error("Ambiguous water diagnostic type");if(id)return *id;
        if(hash){if(!*hash)return std::int64_t{-1};if(const auto *entry=types.find_hash(*hash))return entry->id;}
        throw std::runtime_error("Missing water diagnostic type");
    };
    const auto def=[&](const char *name){const auto *d=defines.find(name);if(!d||!d->lookup.error.empty())throw std::runtime_error("Missing water define");return std::get<std::int64_t>(d->lookup.value);};
    std::ostringstream out;
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(auto [id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    out<<"{\"full_yield_update\":false,\"load_hooks_complete\":false,\"vp_cold_cache_reset\":"<<(vp?"true":"false")
       <<",\"grid\":["<<map.grid.width<<','<<map.grid.height<<','<<map.wrap_x<<','<<map.wrap_y<<']'
       <<",\"lake_max\":"<<(vp?0:def("LAKE_MAX_AREA_SIZE"))<<",\"ocean_min\":"<<def("MIN_WATER_SIZE_FOR_OCEAN")
       <<",\"passable_forts\":"<<(options&&options->enabled("GLOBAL_PASSABLE_FORTS")?"true":"false")<<",\"features\":";
    pairs(rules.features);out<<",\"improvements\":";pairs(rules.improvements);out<<",\"buildings\":";pairs(rules.buildings);
    out<<",\"teams\":[";bool comma=false;for(const auto &[id,p]:players.players){if(comma)out<<',';comma=true;out<<'['<<id<<','<<p.team<<']';}
    out<<"],\"landmasses\":[";comma=false;for(const auto &[id,m]:state.landmasses){if(comma)out<<',';comma=true;out<<'['<<id<<','<<m.tiles<<','<<m.water<<']';}
    out<<"],\"cities\":[";comma=false;
    for(const auto &[owner,p]:cities.players)for(const auto &[id,city]:p.cities){
        if(comma)out<<',';comma=true;out<<'['<<id<<','<<city.x<<','<<city.y<<','<<owner<<',';pairs(city.real_buildings);out<<',';pairs(city.free_buildings);out<<']';
    }
    out<<"],\"cells\":[";comma=false;
    for(const auto &p:map.cells){
        if(!p.plot_type||!p.owner||!p.landmass||!p.rivers||!p.improvement_pillaged||(vp&&!p.improvement_passable))
            throw std::runtime_error("Missing water diagnostic inputs");
        if(comma)out<<',';comma=true;
        out<<'['<<*p.plot_type<<','<<*p.owner<<','<<*p.landmass<<','<<resolve(p.feature_type,p.feature_type_hash)<<','
           <<resolve(p.improvement_type,p.improvement_type_hash)<<','<<p.rivers->is_river<<','<<pass.plots.at(p.index).impassable
           <<','<<p.is_city<<','<<p.improvement_passable.value_or(false)<<','<<*p.improvement_pillaged<<']';
    }
    out<<"],\"passes\":[";
    for(int iteration=0;iteration<2;++iteration){
        if(iteration)out<<',';out<<'[';comma=false;
        // Same interleaving as CvMap::updateYield: refresh a cell, then query
        // its values before proceeding to the next. No fixed-point shortcut.
        for(std::size_t id=0;id<map.cells.size();++id){
            if(vp)runtime::update_plot_water_flags(c,id);
            if(comma)out<<',';comma=true;
            out<<'['<<runtime::plot_is_lake(c,id)<<','<<runtime::plot_is_fresh_water(c,id)<<','
               <<runtime::plot_is_adjacent_to_land(c,id)<<','<<runtime::plot_is_adjacent_to_water(c,id)<<','
               <<runtime::plot_is_coastal_land(c,id)<<','<<runtime::plot_is_coastal_land(c,id,10)<<','
               <<runtime::plot_is_coastal_land(c,id,10,true,true)<<']';
        }out<<']';
    }
    out<<"]}";return out.str();
}
} // namespace civ5::validation
