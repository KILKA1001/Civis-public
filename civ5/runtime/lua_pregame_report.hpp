#pragma once
#include "civ5/runtime/pregame_roster.hpp"
#include "lua_gameinfo_report.hpp"

namespace civ5::validation {
inline std::string lua_pregame_report(const saves::PregameObservation &saved,const content::RulesDatabase &database,
        runtime::ResearchContext &players,const std::vector<runtime::TeamTechnologies> &teams,
        const content::InfoTypeCatalog &types,const content::Catalog &catalog,const runtime::SavedContentProfile &profile){
    const auto roster=runtime::restore_pregame_roster(saved,database);
    runtime::LuaScriptHost host({128ULL*1024*1024,100000000},{},{},runtime::lua_gameplay_services(database,players,teams,types),{},
        runtime::lua_pregame_services(roster));
    std::string checks="assert(Game.GetActivePlayer()=="+std::to_string(roster.active_player)+")\n";
    for(const auto &[id,slot]:roster.slots){
        checks+="assert(PreGame.GetSlotStatus("+std::to_string(id)+")=="+std::to_string(slot.status)+")\n";
        checks+="assert(PreGame.GetCivilization("+std::to_string(id)+")=="+std::to_string(slot.civilization)+")\n";
    }
    host.execute("setup",checks,"=saved-pregame-roster");
    std::string source_report="null";
    for(const auto index:profile.enabled_mods){
        const auto &package=catalog.packages.at(index);
        if(package.id!="8411a7a8-dad3-4622-a18e-fcc18324c799")continue;
        if(source_report!="null")throw std::runtime_error("Ambiguous selected Vox Populi");
        const std::string relative="Core Files/New Lua/CBP_IncaFunctions.lua";
        bool declared=false;for(const auto &file:package.files)declared|=file.path==relative;
        if(!declared)throw std::runtime_error("Selected VP package does not declare Inca source");
        const auto path=content::package_file(package.path.parent_path(),relative);
        const auto identity=runtime::identify_file(path);
        assets::LooseResourceIndex sources(package.path.parent_path());sources.add_file(path);
        const auto asset=sources.find(assets::AssetKey(relative));
        const auto bytes=std::get<assets::AssetHandle>(asset).read_bytes(64ULL*1024*1024);
        host.execute("inca",std::string(bytes.begin(),bytes.end()),"@"+relative);
        // Compare the source predicate across the full SQL civilization
        // inventory, not only the Inca result observed during initialization.
        host.execute("inca",R"lua(
            local maximum=DB.Query("SELECT Value FROM Defines WHERE Name='MAX_MAJOR_CIVS' LIMIT 1")().Value
            local count=0
            for civilization in DB.Query('SELECT ID FROM Civilizations') do
                local expected=false
                for i=0,maximum-1 do
                    local status=PreGame.GetSlotStatus(i)
                    if (status==1 or status==3) and PreGame.GetCivilization(i)==civilization.ID then expected=true end
                end
                assert(JFD_IsCivilisationActive(civilization.ID)==expected);count=count+1
            end
            local active=JFD_IsCivilisationActive(GameInfoTypes.CIVILIZATION_INCA)
            assert(GameEvents.PlayerCityFounded.Count()==(active and 1 or 0))
            GameEvents.CivisPregameCivilizations.Add(function() return count end)
            GameEvents.CivisPregameIncaActive.Add(function() return active end)
        )lua","=source-inca-initialization");
        if(runtime::identify_file(path).sha256!=identity.sha256)throw std::runtime_error("Original Inca source changed");
        const auto count=host.accumulate_int("CivisPregameCivilizations");
        const auto active=host.test_all("CivisPregameIncaActive");
        if(!count.value||!active.value||!count.errors.empty()||!active.errors.empty())
            throw std::runtime_error("Inca script initialization report unavailable");
        source_report="{\"source_sha256\":\""+identity.sha256+"\",\"source_unchanged\":true,\"original_source_executed\":true,\"civilizations_checked\":"+
            std::to_string(*count.value)+",\"inca_active\":"+(*active.value?"true":"false")+",\"city_founded_event_executed\":false}";
    }
    host.complete_loading();
    std::ostringstream out;out<<"{\"active_player\":"<<roster.active_player<<",\"slots_checked\":"<<roster.slots.size()
        <<",\"full_profile_loaded\":false,\"inca_source\":"<<source_report<<",\"slots\":[";
    bool first=true;for(const auto &[id,slot]:roster.slots){if(!first)out<<',';first=false;
        out<<"{\"id\":"<<id<<",\"status\":"<<slot.status<<",\"civilization\":"<<slot.civilization<<'}';}
    out<<"]}";return out.str();
}
} // namespace civ5::validation
