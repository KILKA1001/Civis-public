#pragma once
#include "civ5/runtime/resource_visibility.hpp"
#include "civ5/runtime/vp_resource_visibility_profile.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string resource_visibility_report(const saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &db,
        const content::InfoTypeCatalog &types,runtime::ResearchContext &players,const runtime::PlayerResearchContext &roster,
        const std::vector<runtime::TeamTechnologies> &technologies,const content::GlobalDefineCache &defines){
    const auto rules=runtime::load_resource_visibility_rules(db,types,players.profile);
    auto state=runtime::restore_resource_visibility(snapshot,players,rules,types);
    const auto &map=*snapshot.map_projection;const bool vp=players.profile==content::TechnologyProfile::vp542;
    runtime::ResourceVisibilityContext c{map,state,rules,players,roster,technologies,types,defines,false};
    const auto pass=runtime::restore_plot_passability(snapshot,db,types);
    runtime::LuaScriptHost host({128ULL*1024*1024,100000000},{},{},{},{},{},runtime::with_resource_visibility(runtime::lua_map_services(map,pass),c));
    std::ostringstream out;const auto list=[&](const auto &values){out<<'[';bool comma=false;for(const auto n:values){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto resolve=[&](const auto &id,const auto &hash){if(id)return *id;if(hash){if(!*hash)return std::int64_t{-1};if(const auto *t=types.find_hash(*hash))return t->id;}
        throw std::runtime_error("Unresolved diagnostic content identity");};
    out<<"{\"full_plot_yield\":false,\"load_hooks_complete\":false,\"debug_mode\":false,\"barbarian_camp\":"
       <<std::get<std::int64_t>(defines.find("BARBARIAN_CAMP_IMPROVEMENT")->lookup.value)<<",\"resources\":[";
    bool comma=false;for(const auto &[id,r]:rules.resources){if(comma)out<<',';comma=true;out<<'['<<id<<','<<r.technology<<','<<r.policy<<']';}
    out<<"],\"players\":[";comma=false;
    for(const auto &[id,p]:players.players){if(!p.policies_resolved)throw std::runtime_error("Unresolved diagnostic policies");
        if(comma)out<<',';comma=true;out<<'['<<id<<','<<p.team<<','<<p.alive<<','<<p.slot_status<<',';list(p.policies);out<<",[";
        bool entry=false;if(vp)for(const auto &[r,t]:runtime::vp_resource_visibility_profile(state).alternate_technologies.at(id)){if(entry)out<<',';entry=true;out<<'['<<r<<','<<t.reveal<<','<<t.city_trade<<']';}out<<"]]";
    }
    out<<"],\"teams\":[";comma=false;
    for(const auto &[id,t]:state.teams){if(comma)out<<',';comma=true;out<<'['<<id<<',';
        if(vp)out<<runtime::vp_resource_visibility_profile(state).observers.at(id);else out<<"null";
        out<<',';list(t.forced_resources);out<<',';list(roster.teams.at(id).members);out<<",[";
        bool entry=false;for(const auto &team:technologies)if(team.team()==id)for(const auto &[tech,v]:team.nondefault_values())if(v.has_tech){if(entry)out<<',';entry=true;out<<tech;}out<<"]]";
    }
    out<<"],\"cells\":[";comma=false;
    std::vector<std::int64_t> raw;raw.reserve(map.cells.size());
    std::string checks;std::uint64_t pending=0,lua_checks=0;
    const auto active=players.players.at(snapshot.pregame->archive.integers.at("active_player")).team;
    for(const auto &p:map.cells){
        if(!p.force_resource_reveal)throw std::runtime_error("Unresolved diagnostic plot force flags");
        const auto resource=runtime::plot_resource_type(c,p.index);raw.push_back(resource);
        if(comma)out<<',';comma=true;out<<'['<<resource<<','<<resolve(p.improvement_type,p.improvement_type_hash)<<',';list(*p.force_resource_reveal);out<<']';
        const auto filtered=runtime::plot_resource_type(c,p.index,active);
        checks+="do local p=Map.GetPlotByIndex("+std::to_string(p.index)+");assert(p:GetResourceType()=="+std::to_string(resource)+
            " and p:GetResourceType("+std::to_string(active)+")=="+std::to_string(filtered)+") end\n";lua_checks+=2;
        if(++pending==128){host.execute("resources",checks,"=saved-resource-visibility");pending=0;checks.clear();}
    }
    if(!checks.empty())host.execute("resources",checks,"=saved-resource-visibility");
    out<<"],\"cases\":[";comma=false;
    // Every resource-bearing cell for every saved team, including dead teams.
    // No-resource cells need one team probe because the source returns first.
    for(std::size_t i=0;i<raw.size();++i)for(const auto &[id,t]:state.teams){
        if(comma)out<<',';comma=true;out<<'['<<i<<','<<id<<','<<runtime::plot_resource_type(c,i,id)<<',';
        if(vp)out<<runtime::plot_resource_type(c,i,id,true);else out<<"null";out<<']';
        if(raw[i]==-1)break;
    }
    out<<"],\"refreshed_observers\":[";comma=false;
    if(vp)for(const auto &[id,t]:state.teams){runtime::refresh_resource_observer(c,id);if(comma)out<<',';comma=true;out<<'['<<id<<','<<runtime::vp_resource_visibility_profile(state).observers.at(id)<<']';}
    out<<"],\"lua_checks\":"<<lua_checks<<'}';return out.str();
}
} // namespace civ5::validation
