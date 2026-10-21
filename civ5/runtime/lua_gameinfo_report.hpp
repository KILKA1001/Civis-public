#pragma once
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string gameinfo_lua_string(std::string_view value){
    std::string result="\"";
    for(const unsigned char c:value){
        result+='\\';result+=static_cast<char>('0'+c/100);
        result+=static_cast<char>('0'+(c/10)%10);result+=static_cast<char>('0'+c%10);
    }
    return result+'"';
}
// Read-only checks against the actual activated saved profile. This validates
// database APIs and an optional original library, not full UI/game activation.
inline std::string lua_gameinfo_report(const content::RulesDatabase &database,
        runtime::ResearchContext &players,const std::vector<runtime::TeamTechnologies> &teams,
        const content::InfoTypeCatalog &types,const content::Catalog &catalog,const runtime::SavedContentProfile &profile){
    runtime::LuaScriptHost host({128ULL*1024*1024,100000000},{},{},runtime::lua_gameplay_services(database,players,teams,types));
    std::string checks;std::uint64_t pending=0;
    for(const auto &[name,entry]:types.entries()){
        checks+="assert(GameInfoTypes["+gameinfo_lua_string(name)+"]=="+std::to_string(entry.id)+")\n";
        if(++pending==256){host.execute("rules",checks,"=saved-info-types");checks.clear();pending=0;}
    }
    if(!checks.empty())host.execute("rules",checks,"=saved-info-types");
    host.execute("rules",R"lua(
        local names={'Technologies','Civilizations','Yields','Units','Buildings','Specialists'}
        local rows,fields=0,0
        for _,name in ipairs(names) do
            local t=assert(GameInfo[name])
            local query=DB.Query('SELECT * FROM '..name)
            local iter=t()
            local count=0
            while true do
                local expected,actual=query(),iter()
                if expected==nil then assert(actual==nil and iter()==nil);break end
                assert(type(actual)=='table')
                for key,value in pairs(expected) do assert(actual[key]==value,name..'.'..key);fields=fields+1 end
                assert(t[actual.Type]==actual and t[actual.ID]==actual)
                count=count+1
            end
            assert(#t==count)
            rows=rows+count
        end
        GameEvents.CivisInfoRows.Add(function() return rows end)
        GameEvents.CivisInfoFields.Add(function() return fields end)
    )lua","=saved-gameinfo-rows");
    std::string library="null";
    for(const auto index:profile.enabled_mods){
        const auto &package=catalog.packages.at(index);
        if(package.id!="d1b6328c-ff44-4b0d-aad7-c657f83610cd")continue;
        if(library!="null")throw std::runtime_error("Ambiguous selected Community Patch");
        const std::string relative="Core Files/VPUI/VPUI_core.lua";
        bool declared=false;for(const auto &file:package.files)declared|=file.path==relative;
        if(!declared)throw std::runtime_error("Selected package does not declare VPUI core source");
        const auto path=content::package_file(package.path.parent_path(),relative);
        const auto identity=runtime::identify_file(path);
        assets::LooseResourceIndex sources(package.path.parent_path());sources.add_file(path);
        const auto asset=sources.find(assets::AssetKey(relative));
        const auto bytes=std::get<assets::AssetHandle>(asset).read_bytes(64ULL*1024*1024);
        host.execute("rules",std::string(bytes.begin(),bytes.end()),"@"+relative);
        host.execute("rules",R"lua(
            assert(VP.GameInfoTypes==GameInfoTypes)
            local option=DB.Query("SELECT Value FROM CustomModOptions WHERE Name='BALANCE_CORE_JFD'")().Value
            local yield_type=option==1 and 'YIELD_JFD_SOVEREIGNTY' or 'YIELD_CULTURE_LOCAL'
            assert(VP.GetNumYields()==DB.Query('SELECT ID FROM Yields WHERE Type=?',yield_type)().ID+1)
            local associations=0
            for trait in DB.Query('SELECT DISTINCT TraitType FROM Leader_Traits') do
                local expected={}
                for row in DB.Query('SELECT C.ID FROM Leader_Traits L JOIN Civilization_Leaders CL ON CL.LeaderheadType=L.LeaderType JOIN Civilizations C ON C.Type=CL.CivilizationType WHERE L.TraitType=?',trait.TraitType) do
                    expected[row.ID]=(expected[row.ID] or 0)+1;associations=associations+1
                end
                for _,id in ipairs(VP.GetCivsFromTrait(trait.TraitType) or {}) do
                    assert(expected[id] and expected[id]>0);expected[id]=expected[id]-1
                end
                for _,count in pairs(expected) do assert(count==0) end
            end
            local specialists={}
            for row in DB.Query('SELECT * FROM GreatPersons ORDER BY ID') do
                if row.Specialist then specialists[row.Specialist]=row.ID end
            end
            for specialist,id in pairs(specialists) do assert(VP.GetGreatPersonInfoFromSpecialist(specialist).ID==id) end
            GameEvents.CivisTraitAssociations.Add(function() return associations end)
        )lua","=original-vpui-library-results");
        if(runtime::identify_file(path).sha256!=identity.sha256)throw std::runtime_error("Original VPUI library changed");
        const auto associations=host.accumulate_int("CivisTraitAssociations");
        if(!associations.value||!associations.errors.empty())throw std::runtime_error("VPUI trait associations unavailable");
        library="{\"source_sha256\":\""+identity.sha256+"\",\"source_unchanged\":true,\"original_source_executed\":true,\"trait_associations\":"+
            std::to_string(*associations.value)+",\"ui_controls_executed\":false}";
    }
    host.complete_loading();
    const auto rows=host.accumulate_int("CivisInfoRows"),fields=host.accumulate_int("CivisInfoFields");
    if(!rows.value||!fields.value||!rows.errors.empty()||!fields.errors.empty())throw std::runtime_error("GameInfo comparison results unavailable");
    std::ostringstream out;
    out<<"{\"types_checked\":"<<types.names()<<",\"tables_checked\":6,\"rows_checked\":"<<*rows.value<<",\"fields_checked\":"<<*fields.value
       <<",\"full_profile_loaded\":false,\"vpui_library\":"<<library<<'}';return out.str();
}
} // namespace civ5::validation
