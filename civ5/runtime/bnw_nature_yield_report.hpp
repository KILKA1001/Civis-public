#pragma once
#include "civ5/runtime/bnw_nature_yield.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string bnw_nature_yield_report(saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &db,
    const content::InfoTypeCatalog &types,runtime::ResearchContext &players,const runtime::PlayerResearchContext &roster,
    const std::vector<runtime::TeamTechnologies> &technologies,runtime::CityYieldContext &cities,const content::GlobalDefineCache &defines){
    if(players.profile!=content::TechnologyProfile::bnw)return "{\"applicable\":false,\"full_plot_yield\":false}";
    auto &map=*snapshot.map_projection;
    const auto rules=runtime::load_bnw_nature_yield_rules(db);
    const auto state=runtime::restore_bnw_nature_yields(snapshot,rules,cities);
    const auto rr=runtime::load_resource_visibility_rules(db,types,players.profile);
    auto rs=runtime::restore_resource_visibility(snapshot,players,rr,types);
    const runtime::ResourceVisibilityContext resources{map,rs,rr,players,roster,technologies,types,defines,false};
    const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snapshot);
    const auto pass=runtime::restore_plot_passability(snapshot,db,types);
    runtime::PlotWaterContext water{map,ws,wr,types,pass,players,cities,defines};
    runtime::BnwNatureYieldContext context{rules,state,water,resources};
    std::ostringstream out;
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto matrix=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,y]:v){if(comma)out<<',';comma=true;out<<'['<<id<<',';pairs(y);out<<']';}out<<']';};
    const auto list=[&](const auto &v){out<<'[';bool comma=false;for(const auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto surfaces=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,s]:v){if(comma)out<<',';comma=true;
        out<<'['<<id<<','<<s.nonadditive<<','<<s.natural_wonder<<',';pairs(s.base);out<<',';pairs(s.river);out<<',';pairs(s.hills);out<<']';}out<<']';};
    out<<"{\"applicable\":true,\"full_nature_helper\":true,\"full_plot_yield\":false,\"load_hooks_complete\":false,\"map_cells\":"<<map.cells.size()
       <<",\"culture\":"<<context.culture<<",\"yield_slots\":"<<rules.yield_slots<<",\"feature_slots\":"<<rules.feature_slots<<",\"yields\":[";
    bool comma=false;for(const auto &[id,y]:rules.yields){if(comma)out<<',';comma=true;out<<'['<<id<<','<<y.hills<<','<<y.mountain<<','<<y.lake<<']';}
    out<<"],\"terrains\":";surfaces(rules.terrains);out<<",\"features\":";surfaces(rules.features);out<<",\"resources\":";matrix(rules.resources);
    out<<",\"beliefs\":[";comma=false;for(const auto &[id,b]:rules.beliefs){if(comma)out<<',';comma=true;
        out<<'['<<id<<','<<b.pantheon<<',';matrix(b.terrains);out<<',';matrix(b.features);out<<',';matrix(b.resources);out<<',';pairs(b.natural_wonder);out<<',';pairs(b.natural_wonder_modifier);out<<']';}
    out<<"],\"players\":[";comma=false;for(const auto &[id,p]:state.players){if(comma)out<<',';comma=true;
        out<<'['<<id<<','<<p.natural_wonder_modifier<<',';matrix(p.unimproved_features);out<<',';pairs(p.natural_wonder);out<<']';}
    out<<"],\"leagues\":[";comma=false;for(const auto &l:state.leagues){if(comma)out<<',';comma=true;out<<'[';list(l.members);out<<',';list(l.culture_per_natural_wonder);out<<']';}
    std::vector<std::int64_t> teams{-1};for(const auto &[id,t]:rs.teams)teams.push_back(id);
    out<<"],\"query_teams\":";list(teams);out<<",\"cases\":[";comma=false;
    for(const auto &p:map.cells)for(const auto &[y,info]:rules.yields)for(const auto team:teams){
        if(comma)out<<',';comma=true;
        out<<'['<<p.index<<','<<y<<','<<team<<','<<runtime::bnw_nature_yield(context,p.index,y,team,false).total
           <<','<<runtime::bnw_nature_yield(context,p.index,y,team,true).total<<','<<runtime::bnw_best_nature_yield(context,p.index,y,team)<<']';
    }
    out<<"]}";return out.str();
}
} // namespace civ5::validation
