#pragma once
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include <algorithm>
#include <array>
#include <sstream>

namespace civ5::validation {
// Executes one real selected entrypoint on a disposable copy of the imported
// world. This is not UI context loading, a full mod profile, or a combat turn.
inline std::string lua_city_bombard_report(const content::RulesDatabase &database,
        const runtime::ResearchContext &source_players,const std::vector<runtime::TeamTechnologies> &source_teams,
        const content::Catalog &catalog,const runtime::SavedContentProfile &profile,const content::InfoTypeCatalog &types){
    const content::PackageManifest *package=nullptr;
    for(const auto index:profile.enabled_mods){
        const auto &candidate=catalog.packages.at(index);
        if(candidate.id=="d1b6328c-ff44-4b0d-aad7-c657f83610cd"){
            if(package)throw std::runtime_error("Ambiguous active Community Patch");package=&candidate;
        }
    }
    if(!package)throw std::runtime_error("City bombard Lua check requires selected Community Patch");
    const std::string relative="Core Files/New Lua/GlobalCityBombardRange.lua";
    bool declared=false;
    for(const auto &file:package->files)declared|=file.path==relative;
    bool entrypoint=false;
    if(const auto *entries=package->document.child("EntryPoints"))for(const auto &entry:entries->children)
        entrypoint|=entry.name=="EntryPoint"&&entry.attribute("type")=="InGameUIAddin"&&entry.attribute("file")==relative;
    if(!declared||!entrypoint)throw std::runtime_error("City bombard source is not a selected declared entrypoint");
    const auto path=content::package_file(package->path.parent_path(),relative);
    const auto identity=runtime::identify_file(path);
    assets::LooseResourceIndex index(package->path.parent_path());index.add_file(path);
    const auto resolved=index.find(assets::AssetKey(relative));
    const auto bytes=std::get<assets::AssetHandle>(resolved).read_bytes(64ULL*1024*1024);
    const std::string source(bytes.begin(),bytes.end());
    auto players=source_players;auto teams=source_teams;
    runtime::LuaScriptHost host({64ULL*1024*1024,10000000},{},{},runtime::lua_gameplay_services(database,players,teams,types));
    host.execute("city-bombard",source,"@"+relative);
    host.execute("city-bombard","assert(GameEvents.TeamTechResearched.Count()==1 and GameEvents.GetBombardRange.Count()==1)","=registered-hooks");
    const auto maximum=database.global_define("MAX_CITY_ATTACK_RANGE",content::GlobalDefineKind::integer);
    const auto indirect=database.global_define("CAN_CITY_USE_INDIRECT_FIRE",content::GlobalDefineKind::integer);
    if(!maximum.found||!maximum.error.empty()||!indirect.found||!indirect.error.empty())throw std::runtime_error("Missing city bombard defines");
    const auto max_range=std::get<std::int64_t>(maximum.value);
    if(max_range<0||max_range>9007199254740992LL)throw std::runtime_error("City bombard define outside exact Lua range");
    const auto default_range=max_range*(std::get<std::int64_t>(indirect.value)==1?-1:1);
    std::vector<std::pair<std::int64_t,std::int64_t>> technologies;
    const auto rows=database.query_rows({"SELECT ID, BombardRange, BombardIndirect FROM Technologies WHERE BombardRange > 0 ORDER BY BombardRange ASC, BombardIndirect ASC",{}},
        [&](const content::RulesRowView &row){
            technologies.emplace_back(row.integer("ID"),std::min(max_range,row.integer("BombardRange"))*(row.integer("BombardIndirect")==1?-1:1));
        });
    if(!rows.all_succeeded())throw std::runtime_error(rows.error);
    std::vector<std::int64_t> tech_ids;
    database.query({"SELECT ID FROM Technologies WHERE ID >= 0 ORDER BY ID",{}},[&](auto row){tech_ids.push_back(std::get<std::int64_t>(row[0]));});
    std::uint64_t flags_checked=0;
    for(const auto &team:teams){
        std::string checks;
        for(const auto tech:tech_ids){
            checks+="assert(Teams["+std::to_string(team.team())+"]:IsHasTech("+std::to_string(tech)+")=="+(team.has_tech(tech)?"true":"false")+")\n";
            ++flags_checked;
        }
        host.execute("city-bombard",checks,"=saved-technology-flags");
    }
    std::map<std::int64_t,std::int64_t> ranges;
    const auto check_range=[&](std::int64_t player,std::int64_t expected){
        const std::array<runtime::ScriptValue,2> args{player,std::int64_t{0}};
        const auto result=host.accumulate_int("GetBombardRange",args);
        if(!result.errors.empty())throw std::runtime_error(result.errors.front());
        if(result.value!=expected)throw std::runtime_error("Original GetBombardRange event disagrees with saved technology state");
    };
    for(const auto &[id,player]:players.players){
        const auto team=std::find_if(teams.begin(),teams.end(),[&](const auto &team){return team.team()==player.team;});
        if(team==teams.end())throw std::runtime_error("Saved player team is missing");
        auto expected=default_range;
        for(const auto &[technology,range]:technologies)if(team->has_tech(technology))expected=range;
        check_range(id,expected);ranges[id]=expected;
    }
    std::string notification="null";
    for(const auto &[id,player]:players.players){
        auto team=std::find_if(teams.begin(),teams.end(),[&](const auto &team){return team.team()==player.team;});
        for(const auto &[technology,range]:technologies){
            if(team->has_tech(technology)||ranges.at(id)==range)continue;
            const auto before=ranges.at(id);
            team->set_has_tech(technology,true);
            // Controlled Hook delivery checks the original listener body.
            // Complete world acquisition remains a separate required service.
            const std::array<runtime::ScriptValue,3> args{team->team(),technology,std::int64_t{1}};
            const auto delivered=host.call_hook("TeamTechResearched",args);
            if(!delivered.errors.empty())throw std::runtime_error(delivered.errors.front());
            check_range(id,range);
            host.execute("city-bombard","assert(Teams["+std::to_string(team->team())+"]:IsHasTech("+std::to_string(technology)+"))","=live-research-state");
            notification="{\"controlled\":true,\"player\":"+std::to_string(id)+",\"team\":"+std::to_string(team->team())+
                ",\"technology\":"+std::to_string(technology)+",\"before\":"+std::to_string(before)+",\"after\":"+std::to_string(range)+"}";
            break;
        }
        if(notification!="null")break;
    }
    host.complete_loading();
    if(runtime::identify_file(path).sha256!=identity.sha256)throw std::runtime_error("Original city bombard Lua changed");
    std::ostringstream out;
    out<<"{\"source_sha256\":\""<<identity.sha256<<"\",\"original_source_executed\":true,\"source_unchanged\":true,"
         "\"disposable_world_copy\":true,\"full_profile_loaded\":false,\"native_event_value_dispatch\":true,"
         "\"players_checked\":"<<ranges.size()<<",\"teams_registered\":"<<teams.size()<<",\"technology_rows\":"<<technologies.size()
       <<",\"messages\":"<<host.messages().size()<<",\"initial_signed_ranges\":[";
    bool first=true;for(const auto &[id,range]:ranges){if(!first)out<<',';first=false;out<<'['<<id<<','<<range<<']';}
    out<<"],\"technology_flags_checked\":"<<flags_checked<<",\"research_notification\":"<<notification<<'}';return out.str();
}
} // namespace civ5::validation
