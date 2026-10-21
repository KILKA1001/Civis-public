#pragma once
#include "civ5/runtime/vp_nature_yield.hpp"
#include "civ5/runtime/vp_unit_yield_profile.hpp"
#include "civ5/runtime/resource_visibility.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_nature_yield_report(saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,const runtime::PlayerResearchContext &roster,const std::vector<runtime::TeamTechnologies> &technologies,
    runtime::CityYieldContext &cities,runtime::MaintenanceContext &units,const runtime::MaintenanceRules &unit_rules,
    const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false,\"full_plot_yield\":false}";
    const auto rules=runtime::load_vp_nature_yield_rules(db);auto state=runtime::restore_vp_nature_yields(snapshot,rules,cities,units);
    auto &map=*snapshot.map_projection;const auto pass=runtime::restore_plot_passability(snapshot,db,types);
    auto ms=runtime::restore_plot_movement(snapshot,players,roster);runtime::PlotMovementContext movement{map,pass,players,roster,ms,types,defines};
    const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snapshot);
    runtime::PlotWaterContext water{map,ws,wr,types,pass,players,cities,defines,options};
    const auto rr=runtime::load_resource_visibility_rules(db,types,players.profile);auto rs=runtime::restore_resource_visibility(snapshot,players,rr,types);
    runtime::ResourceVisibilityContext resource{map,rs,rr,players,roster,technologies,types,defines,false};
    const auto active=snapshot.pregame->archive.integers.at("active_player");const auto active_team=players.players.at(active).team;
    runtime::VpNatureYieldContext context{rules,state,water,movement,cities,units,unit_rules,active_team};
    const auto def=[&](const char *name){return std::get<std::int64_t>(defines.find(name)->lookup.value);};
    std::ostringstream out;
    const auto list=[&](const auto &v){out<<'[';bool comma=false;for(const auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto rows=[&](const auto &v){out<<'[';bool comma=false;for(const auto &y:v){if(comma)out<<',';comma=true;pairs(y);}out<<']';};
    const auto surfaces=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,s]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<s.nonadditive<<','<<s.natural_wonder<<','<<s.pseudo_natural_wonder<<',';rows(s.yields);out<<']';}out<<']';};
    out<<"{\"applicable\":true,\"full_nature_helper\":true,\"full_plot_yield\":false,\"load_hooks_complete\":false,\"pillage_filter_applied\":false,\"map_cells\":"<<map.cells.size()
       <<",\"active_player\":"<<active<<",\"active_team\":"<<active_team<<",\"desert_production\":"<<def("BALANCE_PRODUCTION_DESERT_IMPROVEMENT")
       <<",\"strength_threshold\":"<<def("CITY_STRENGTH_THRESHOLD_FOR_BONUSES")<<",\"balance_vp\":"<<options->enabled("BALANCE_VP")
       <<",\"plot_extensions\":"<<options->enabled("PLOTS_EXTENSIONS")<<",\"no_gp_resource\":"<<options->enabled("BALANCE_NO_RESOURCE_YIELDS_FROM_GP_IMPROVEMENT")
       <<",\"monopolies_enabled\":"<<options->enabled("BALANCE_RESOURCE_MONOPOLIES")<<",\"yields\":[";
    bool comma=false;for(const auto &[id,y]:rules.yields){if(comma)out<<',';comma=true;out<<'['<<id<<','<<y.hills<<','<<y.mountain<<','<<y.lake<<','<<y.city<<','<<y.population_offset<<','<<y.population_divisor<<','<<y.min_city<<',';list(y.city_geography);out<<']';}
    out<<"],\"terrains\":";surfaces(rules.terrains);out<<",\"features\":";surfaces(rules.features);
    out<<",\"resources\":[";comma=false;for(const auto &[id,y]:rules.resources){if(comma)out<<',';comma=true;out<<'['<<id<<',';pairs(y);out<<']';}
    out<<"],\"improvements\":[";comma=false;for(const auto &[id,i]:rules.improvements){if(comma)out<<',';comma=true;out<<'['<<id<<','<<i.connects_all_resources<<','<<i.no_followup<<']';}
    out<<"],\"plots\":[";comma=false;for(const auto &[id,p]:rules.plots){if(comma)out<<',';comma=true;out<<'['<<id<<',';pairs(p.base);out<<",[";bool inner=false;
        for(const auto &[feature,changes]:p.adjacent_features){if(inner)out<<',';inner=true;out<<'['<<feature<<',';rows(changes);out<<']';}out<<"]]";}
    out<<"],\"extra\":[";comma=false;for(const auto &e:state.extra_yields){if(comma)out<<',';comma=true;out<<'['<<e.x<<','<<e.y<<',';pairs(e.yields);out<<']';}
    out<<"],\"players\":[";comma=false;for(const auto &[id,p]:state.players){if(comma)out<<',';comma=true;out<<'['<<id<<','<<cities.players.at(id).total_population<<',';list(p.has_global_monopoly);out<<',';rows(p.yields);out<<']';}
    out<<"],\"cities\":[";comma=false;for(const auto &[owner,p]:cities.players)for(const auto &[id,city]:p.cities){const auto &s=*runtime::vp_city_plot_profile_city(cities,owner,id).yields;
        if(comma)out<<',';comma=true;out<<'['<<owner<<','<<id<<','<<city.x<<','<<city.y<<','<<city.population<<','<<(p.capital==id)<<','<<s.garrison<<','<<s.garrison_override<<','<<s.strength<<',';pairs(s.per_strength_times100);out<<']';}
    out<<"],\"units\":[";comma=false;for(const auto &[owner,p]:units.players)for(const auto &[id,u]:p.units){const auto &s=*runtime::vp_unit_yield_profile(u).plot_yields;
        if(comma)out<<',';comma=true;out<<'['<<owner<<','<<id<<','<<u.type<<','<<u.civilian<<',';if(s.position)out<<'['<<s.position->first<<','<<s.position->second<<']';else out<<"null";
        out<<','<<s.death_delay<<','<<s.base_combat<<',';pairs(s.garrison);out<<',';pairs(s.fortification);out<<']';}
    out<<"],\"cells\":[";comma=false;for(const auto &p:map.cells){if(!p.units||!p.revealed_teams)throw std::runtime_error("VP nature diagnostic missing shared unit/reveal state");
        if(comma)out<<',';comma=true;out<<"[[";bool inner=false;for(const auto &u:*p.units){if(inner)out<<',';inner=true;out<<'['<<u.owner<<','<<u.id<<']';}out<<"],";list(*p.revealed_teams);out<<']';}
    const auto owning=[&](const saves::SavedMapCellProjection &p)->runtime::CityYieldState*{
        const auto &ref=p.yield_city_override&&(p.yield_city_override->owner!=-1||p.yield_city_override->id!=-1)?p.yield_city_override:p.yield_city;
        if(!ref)throw std::runtime_error("Unresolved VP nature diagnostic city reference");const auto player=cities.players.find(ref->owner);if(player==cities.players.end())return nullptr;
        const auto city=player->second.cities.find(ref->id);return city==player->second.cities.end()?nullptr:&city->second;};
    out<<"],\"passes\":[";
    for(int iteration=0;iteration<2;++iteration){if(iteration){out<<',';for(const auto &[id,p]:state.players)runtime::refresh_vp_global_monopoly_cache(state,rules,*options,id);
            for(const auto &[id,t]:ms.teams)runtime::refresh_plot_movement_team_status(movement,id);}
        out<<"{\"monopolies\":[";comma=false;for(const auto &[id,p]:state.players){if(comma)out<<',';comma=true;out<<'['<<id<<',';list(p.global_monopoly_cache);out<<']';}
        out<<"],\"cases\":[";comma=false;for(const auto &p:map.cells){auto *city=owning(p);const auto feature=p.feature_type.value(),improvement=p.improvement_type.value();
            if(comma)out<<',';comma=true;out<<'[';bool inner=false;
            for(const auto &[yield,y]:rules.yields)for(int mode=0;mode<4;++mode){auto player=mode==2?std::int64_t{-1}:mode==1?active:p.owner.value();if(mode!=2&&mode!=1&&player==-1&&city)player=city->owner;
                const auto res=runtime::plot_resource_type(resource,p.index,player==-1?-1:players.players.at(player).team);
                const auto q=runtime::vp_nature_yield(context,p.index,yield,player,mode==3?-1:feature,res,improvement,city,mode==1);
                if(inner)out<<',';inner=true;out<<q.total;}out<<']';
        }out<<"]}";
    }
    out<<"],\"final_garrisons\":[";comma=false;for(const auto &[owner,p]:cities.players)for(const auto &[id,city]:p.cities){if(comma)out<<',';comma=true;out<<'['<<owner<<','<<id<<','<<runtime::vp_city_plot_profile_city(cities,owner,id).yields->garrison<<']';}
    out<<"]}";return out.str();
}
} // namespace civ5::validation
