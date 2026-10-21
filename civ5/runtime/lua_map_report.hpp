#pragma once
#include "civ5/runtime/lua_map_services.hpp"
#include "lua_gameinfo_report.hpp"

namespace civ5::validation {
inline std::string lua_map_report(const saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &database,
        runtime::ResearchContext &players,const std::vector<runtime::TeamTechnologies> &teams,
        const content::InfoTypeCatalog &types,const content::Catalog &catalog,const runtime::SavedContentProfile &profile,
        const content::CustomModOptions *options){
    const auto passability=runtime::restore_plot_passability(snapshot,database,types);
    const auto &map=*snapshot.map_projection;
    const runtime::MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,passability.layout};
    const auto services=runtime::lua_gameplay_services(database,players,teams,types,options);
    runtime::LuaScriptHost host({128ULL*1024*1024,100000000},{},{},services,{},{},runtime::lua_map_services(map,passability));
    std::string checks;std::uint64_t pending=0,impassable=0,water=0,mountains=0;
    for(const auto &cell:map.cells){
        if(!cell.plot_type)throw std::runtime_error("Missing diagnostic plot type");
        const auto id=std::to_string(cell.index),x=std::to_string(cell.x),y=std::to_string(cell.y);
        const bool is_water=*cell.plot_type==3,is_mountain=*cell.plot_type==0||
            (passability.layout==saves::GameStateLayout::vp542_corpus&&cell.terrain_type==7),blocked=passability.plots.at(cell.index).impassable;
        water+=is_water;mountains+=is_mountain;impassable+=blocked;
        checks+="do local p=Map.GetPlotByIndex("+id+");assert(p==Map.GetPlot("+x+","+y+") and p:GetPlotIndex()=="+id+
            " and p:GetX()=="+x+" and p:GetY()=="+y+" and p:IsWater()=="+(is_water?"true":"false")+
            " and p:IsMountain()=="+(is_mountain?"true":"false")+" and p:IsImpassable()=="+(blocked?"true":"false")+") end\n";
        if(++pending==128){host.execute("map",checks,"=saved-map-bindings");checks.clear();pending=0;}
    }
    if(!checks.empty())host.execute("map",checks,"=saved-map-bindings");
    std::string source_report="null";
    for(const auto package_index:profile.enabled_mods){
        const auto &package=catalog.packages.at(package_index);
        if(package.id!="e53b3cb8-48a9-4f46-8e1d-c9d3e8fa2233")continue;
        if(source_report!="null")throw std::runtime_error("Ambiguous selected Pontoon Bridges");
        const std::string relative="ImprovementPontoonBridge.lua";bool declared=false,entrypoint=false;
        for(const auto &file:package.files)declared|=file.path==relative;
        if(const auto *entries=package.document.child("EntryPoints"))for(const auto &entry:entries->children)
            entrypoint|=entry.name=="EntryPoint"&&entry.attribute("type")=="InGameUIAddin"&&entry.attribute("file")==relative;
        if(!declared||!entrypoint)throw std::runtime_error("Pontoon source is not a declared selected entrypoint");
        const auto path=content::package_file(package.path.parent_path(),relative);const auto identity=runtime::identify_file(path);
        assets::LooseResourceIndex sources(package.path.parent_path());sources.add_file(path);
        const auto bytes=std::get<assets::AssetHandle>(sources.find(assets::AssetKey(relative))).read_bytes(64ULL*1024*1024);
        host.execute("pontoon",std::string(bytes.begin(),bytes.end()),"@"+relative);
        host.execute("pontoon","assert(GameEvents.PlayerCanBuild.Count()==1 and GameEvents.PlotCanImprove.Count()==1 and GameEvents.TeamTechResearched.Count()==1 and GameEvents.CityProjectComplete.Count()==1)","=pontoon-registration");
        const auto type=[&](const char *name){const auto *entry=types.find(name);if(!entry)throw std::runtime_error("Missing Pontoon type");return entry->id;};
        const auto improvement=type("IMPROVEMENT_PONTOON_BRIDGE"),build=type("BUILD_PONTOON_BRIDGE"),build2=type("BUILD_PONTOON_BRIDGE2");
        std::optional<std::int64_t> human,ai;
        for(const auto &[id,player]:players.players)if(player.alive){if(services.player_human(id,0)){if(!human)human=id;}else if(!ai)ai=id;}
        if(!human||!ai)throw std::runtime_error("Pontoon diagnostic needs saved living human and AI players");
        std::uint64_t allowed_improvements=0,allowed_ai=0,cases=0;
        const auto validate=[&](const char *event,std::span<const runtime::ScriptValue> args,bool expected){
            const auto result=host.test_all(event,args);if(!result.errors.empty()||result.value!=expected)
                throw std::runtime_error(std::string("Saved Pontoon predicate mismatch: ")+event);++cases;
        };
        for(const auto &cell:map.cells){
            const auto x=static_cast<std::int64_t>(cell.x),y=static_cast<std::int64_t>(cell.y);
            std::vector<const saves::SavedMapCellProjection*> land,sea;
            for(int d=0;d<6;++d)if(const auto id=geometry.direction(x,y,d)){
                const auto &p=map.cells.at(*id);if(!p.plot_type)throw std::runtime_error("Unresolved neighbour plot");
                const bool mountain=*p.plot_type==0||(passability.layout==saves::GameStateLayout::vp542_corpus&&p.terrain_type==7);
                if(!mountain&&!passability.plots.at(*id).impassable)(*p.plot_type==3?sea:land).push_back(&p);
            }
            const auto distance=[&](const auto *a,const auto *b){return geometry.distance(a->x,a->y,b->x,b->y);};
            const bool improve=land.size()>3||(land.size()==2&&distance(land[0],land[1])>1)||
                (land.size()==3&&distance(land[0],land[1])+distance(land[1],land[2])+distance(land[0],land[2])>4);
            const bool ai_build=land.size()!=6&&!(sea.size()==2&&distance(sea[0],sea[1])==1);
            validate("PlotCanImprove",std::array<runtime::ScriptValue,3>{x,y,improvement},improve);
            for(const auto b:{build,build2}){
                validate("PlayerCanBuild",std::array<runtime::ScriptValue,5>{*ai,std::int64_t{-1},x,y,b},ai_build);
                validate("PlayerCanBuild",std::array<runtime::ScriptValue,5>{*human,std::int64_t{-1},x,y,b},true);
            }
            allowed_improvements+=improve;allowed_ai+=ai_build;
        }
        if(runtime::identify_file(path).sha256!=identity.sha256)throw std::runtime_error("Original Pontoon source changed");
        source_report="{\"source_sha256\":\""+identity.sha256+"\",\"source_unchanged\":true,\"original_source_executed\":true,\"predicate_cases\":"+
            std::to_string(cases)+",\"improvement_allowed\":"+std::to_string(allowed_improvements)+",\"ai_build_allowed\":"+std::to_string(allowed_ai)+
            ",\"human_player\":"+std::to_string(*human)+",\"ai_player\":"+std::to_string(*ai)+",\"route_mutations_executed\":false}";
    }
    host.complete_loading();
    return "{\"cells_checked\":"+std::to_string(map.cells.size())+",\"water\":"+std::to_string(water)+",\"mountains\":"+std::to_string(mountains)+
        ",\"impassable\":"+std::to_string(impassable)+",\"full_profile_loaded\":false,\"pontoon_source\":"+source_report+"}";
}
} // namespace civ5::validation
