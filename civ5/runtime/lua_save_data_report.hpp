#pragma once
#include "lua_gameinfo_report.hpp"
#include <iomanip>
#include <algorithm>

namespace civ5::validation {
struct LuaSavedDatabaseOutput {
    const saves::LoadedSave *source=nullptr;
    const saves::SavedSessionDecodeContext *context=nullptr;
    std::filesystem::path input_path,output_path;
    std::string input_sha256;
};
inline std::string saved_lua_value(const runtime::ScriptValue &value){
    if(std::holds_alternative<std::monostate>(value))return "nil";
    if(const auto *v=std::get_if<bool>(&value))return *v?"true":"false";
    if(const auto *v=std::get_if<std::int64_t>(&value))return std::to_string(*v);
    if(const auto *v=std::get_if<std::string>(&value))return gameinfo_lua_string(*v);
    std::ostringstream text;text<<std::setprecision(17)<<std::get<double>(value);return text.str();
}
inline std::string lua_save_data_report(const saves::SavedDatabaseObservation &saved,
        const content::RulesDatabase &rules,runtime::ResearchContext &players,
        const std::vector<runtime::TeamTechnologies> &teams,const content::InfoTypeCatalog &types,
        const content::Catalog &catalog,const runtime::SavedContentProfile &profile,std::string_view library_mod={},
        const LuaSavedDatabaseOutput &output={}){
    if(saved.non_sqlite_image_skipped)throw std::runtime_error("Saved database is unverified");
    if(saved.image.empty()){
        if(!output.output_path.empty())throw std::runtime_error("Cannot update an absent saved database");
        return "{\"available\":false,\"full_profile_loaded\":false}";
    }
    if(!saved.quick_check_passed)throw std::runtime_error("Saved database integrity was not checked");
    auto database=content::RulesDatabase::from_saved_image(saved.image);
    const auto world=runtime::lua_gameplay_services(rules,players,teams,types);
    runtime::LuaScriptHost host({128ULL*1024*1024,100000000},{},{},world,runtime::lua_saved_data_services(database));
    std::uint64_t tables=0,rows=0,fields=0;
    std::vector<std::string> names;
    database->query({"SELECT name FROM sqlite_master WHERE type='table'",{}},[&](auto row){names.push_back(std::get<std::string>(row[0]));});
    for(const auto &name:names){
        std::string quoted="\"";for(const char c:name){quoted+=c;if(c=='"')quoted+=c;}quoted+='"';
        const auto sql="SELECT * FROM "+quoted;
        host.execute("compare","saved_query=Modding.OpenSaveData().Query("+gameinfo_lua_string(sql)+")","=saved-table");
        auto cursor=database->script_query(sql,{});
        if(!cursor)throw std::runtime_error("Cannot inspect saved mod table");
        std::string checks;std::uint64_t batch=0;
        while(const auto row=cursor->next()){
            checks+="do local row=assert(saved_query())\n";
            for(const auto &[key,value]:*row){
                checks+="assert(row["+gameinfo_lua_string(key)+"]=="+saved_lua_value(value)+")\n";++fields;
            }
            checks+="end\n";++rows;
            if(++batch==32){host.execute("compare",checks,"=saved-table-values");checks.clear();batch=0;}
        }
        if(!cursor->error().empty())throw std::runtime_error("Saved mod table inspection failed");
        checks+="assert(saved_query()==nil);saved_query=nil;collectgarbage('collect')";
        host.execute("compare",checks,"=saved-table-end");++tables;
    }
    std::string library="null";
    std::optional<std::string> library_source;
    for(std::size_t index=0;index<catalog.packages.size()&&!library_mod.empty();++index){
        const auto &package=catalog.packages.at(index);
        if(package.id!=library_mod)continue;
        if(library!="null")throw std::runtime_error("Ambiguous diagnostic table-loader package");
        const bool selected=std::find(profile.enabled_mods.begin(),profile.enabled_mods.end(),index)!=profile.enabled_mods.end();
        const std::string relative="ThirdParty/TableSaverLoader.lua";
        bool declared=false;for(const auto &file:package.files)declared|=file.path==relative;
        if(!declared)throw std::runtime_error("Diagnostic package has no table loader");
        const auto path=content::package_file(package.path.parent_path(),relative);
        const auto identity=runtime::identify_file(path);
        assets::LooseResourceIndex source(package.path.parent_path());source.add_file(path);
        const auto asset=source.find(assets::AssetKey(relative));
        const auto bytes=std::get<assets::AssetHandle>(asset).read_bytes(64ULL*1024*1024);
        const std::string script(bytes.begin(),bytes.end());
        library_source=script;
        host.execute("squads",script,"@"+relative);
        host.execute("squads",R"lua(
            squad_names={};TableLoad(squad_names,'upMainsquadNamesLutStore')
            local count=0
            for row in Modding.OpenSaveData().Query('SELECT * FROM upMainsquadNamesLutStore_Data') do
                assert(row.parent=='_' and (row.varType==2 or row.varType==0))
                local expected=row.varType==0 and tonumber(row.varText) or row.varText
                assert(squad_names[tonumber(string.sub(row.key,2))]==expected);count=count+1
            end
            assert(count>0)
            for row in Modding.OpenSaveData().Query('SELECT loadErrors FROM upMainsquadNamesLutStore_Info WHERE ID=(SELECT MAX(ID) FROM upMainsquadNamesLutStore_Info)') do
                assert(row.loadErrors=='none')
            end
            GameEvents.CivisLoadedSquads.Add(function() return count end)
            squad_names[1]='Civis saved database verification'
            squad_names[5000000001]=5000000002
            TableSave(squad_names,'upMainsquadNamesLutStore')
        )lua","=saved-squad-state");
        const auto count=host.accumulate_int("CivisLoadedSquads");
        if(!count.value||!count.errors.empty())throw std::runtime_error("Squad names did not load");
        auto restored=content::RulesDatabase::from_saved_image(database->saved_image());
        runtime::LuaScriptHost reload({128ULL*1024*1024,100000000},{},{},world,runtime::lua_saved_data_services(restored));
        reload.execute("squads",script,"@"+relative);
        reload.execute("squads",R"lua(
            local names={};TableLoad(names,'upMainsquadNamesLutStore')
            assert(names[1]=='Civis saved database verification' and names[5000000001]==5000000002)
            for row in Modding.OpenSaveData().Query('SELECT loadErrors FROM upMainsquadNamesLutStore_Info WHERE ID=(SELECT MAX(ID) FROM upMainsquadNamesLutStore_Info)') do
                assert(row.loadErrors=='none')
            end
        )lua","=modified-squad-state-reloaded");
        reload.complete_loading();
        if(runtime::identify_file(path).sha256!=identity.sha256)throw std::runtime_error("Original table loader changed");
        library="{\"source_sha256\":\""+identity.sha256+"\",\"original_source_executed\":true,\"source_unchanged\":true,\"names_loaded\":"+
            std::to_string(*count.value)+",\"mod_selected_in_save\":"+(selected?"true":"false")+
            ",\"modified_image_reloaded\":true,\"wide_key_and_value\":true,\"ui_events_executed\":false}";
    }
    if(!library_mod.empty()&&library=="null")throw std::runtime_error("Diagnostic table-loader package unavailable");
    std::string written="null";
    if(!output.output_path.empty()){
        if(!output.source||!output.context)throw std::runtime_error("Saved database output requires its source snapshot and decode context");
        if(runtime::ascii_lower(assets::path_utf8(output.output_path.extension()))!=".civissave")
            throw std::runtime_error("Saved database output must use .CivisSave");
        host.execute("persist",R"lua(
            local db=Modding.OpenSaveData()
            local value=(db.GetValue('__CivisSaveRoundTrip') or 5000000000)+1
            assert(db.SetValue('__CivisSaveRoundTrip',value))
            GameEvents.CivisWrittenValue.Add(function() return value end)
        )lua","=saved-database-write-probe");
        const auto expected=host.accumulate_int("CivisWrittenValue");
        if(!expected.value||!expected.errors.empty())throw std::runtime_error("Saved database marker was not written");
        const auto image=database->saved_image();
        if(runtime::identify_file(output.input_path).sha256!=output.input_sha256)
            throw std::runtime_error("Input save changed before database write");
        const auto document=saves::make_civis_import_document(*output.source,output.input_sha256,std::span(image));
        saves::write_civis_save(output.output_path,document);
        const auto loaded=saves::read_loaded_save(output.output_path);
        if(loaded.bytes!=output.source->bytes||loaded.source_prefix!=output.source->source_prefix||loaded.header!=output.source->header||
           loaded.source_sha256!=output.source->source_sha256.value_or(output.input_sha256)||loaded.saved_database_override!=image)
            throw std::runtime_error("Written Civis save changed retained source data or current database");
        const auto session=saves::observe_loaded_session(loaded,*output.context);
        if(!session.saved_database.image_from_civis_section||session.saved_database.image!=image||
           session.saved_database.start_offset!=saved.start_offset||session.saved_database.end_offset!=saved.end_offset||
           session.saved_database.declared_image_bytes!=saved.declared_image_bytes)
            throw std::runtime_error("Shared saved session did not apply its database section");
        auto file_database=content::RulesDatabase::from_saved_image(session.saved_database.image);
        runtime::LuaScriptHost file_host({128ULL*1024*1024,100000000},{},{},world,runtime::lua_saved_data_services(file_database));
        file_host.execute("reload","assert(Modding.OpenSaveData().GetValue('__CivisSaveRoundTrip')=="+
            std::to_string(*expected.value)+")","=civis-file-database-restored");
        if(library_source){
            file_host.execute("reload",*library_source,"@TableSaverLoader.lua");
            file_host.execute("reload",R"lua(
                local names={};TableLoad(names,'upMainsquadNamesLutStore')
                assert(names[1]=='Civis saved database verification' and names[5000000001]==5000000002)
                for row in Modding.OpenSaveData().Query('SELECT loadErrors FROM upMainsquadNamesLutStore_Info WHERE ID=(SELECT MAX(ID) FROM upMainsquadNamesLutStore_Info)') do
                    assert(row.loadErrors=='none')
                end
            )lua","=civis-file-source-library-restored");
        }
        file_host.complete_loading();
        written="{\"file_sha256\":\""+runtime::identify_file(output.output_path).sha256+"\",\"image_bytes\":"+
            std::to_string(image.size())+",\"marker\":"+std::to_string(*expected.value)+
            ",\"shared_session_reloaded\":true,\"retained_source_unchanged\":true,\"library_reloaded\":"+(library_source?"true":"false")+"}";
    }
    host.complete_loading();
    std::ostringstream out;out<<"{\"available\":true,\"image_bytes\":"<<saved.image.size()<<",\"tables_checked\":"<<tables
        <<",\"rows_checked\":"<<rows<<",\"fields_checked\":"<<fields<<",\"database_from_civis_section\":"<<(saved.image_from_civis_section?"true":"false")
        <<",\"full_profile_loaded\":false,\"squads_library\":"<<library<<",\"written_save\":"<<written<<'}';
    return out.str();
}
} // namespace civ5::validation
