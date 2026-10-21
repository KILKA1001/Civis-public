#include "civ5/content/mod_script_registry.hpp"
#include "civ5/platform/windows_path.hpp"
#include <array>
#include <fstream>
#include <iterator>
#include <map>
#include <set>

namespace civ5::content {
namespace {
// Retail mod registry declarations. No relationship MaxVersion/int32 ceiling.
constexpr char schema[]=R"sql(
CREATE TABLE Mods('RowID' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
 'ModID' TEXT NOT NULL,'Version' TEXT,'Type' INTEGER DEFAULT 1,
 'Installed' BOOL DEFAULT 0,'Enabled' BOOL DEFAULT 0,'Activated' BOOL DEFAULT 0,'LastRetrieved' DATETIME);
CREATE TABLE ModEntryPoints('ModID' TEXT NOT NULL,'Version' INTEGER NOT NULL,
 'Name' TEXT NOT NULL,'Description' TEXT NOT NULL,'Type' TEXT NOT NULL,'File' TEXT,
 FOREIGN KEY(ModID,Version) REFERENCES Mods(ModID,Version));
CREATE TABLE ModFiles('ModID' TEXT NOT NULL,'Version' INTEGER NOT NULL,'Path' TEXT NOT NULL,
 'md5' TEXT,'EvaluatedPath' TEXT,Import INTEGER DEFAULT 0,PRIMARY KEY('ModID','Version','Path'),
 FOREIGN KEY(ModID,Version) REFERENCES Mods(ModID,Version));
CREATE INDEX ModsEnabledIndex On Mods('Enabled');
)sql";
constexpr char entries_sql[]="SELECT * FROM ModEntryPoints inner join Mods on ModEntryPoints.ModID = Mods.ModID and ModEntryPoints.Version = Mods.Version WHERE Mods.Activated = 1 AND ModEntryPoints.Type = ?";
constexpr char file_sql[]="SELECT md5,EvaluatedPath FROM ModFiles where ModID = ? and Version = ? and Path = ? LIMIT 1";
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::string entry_text(const XmlNode *node){
    if(!node)return {};
    const auto first=node->text.find_first_not_of(" \t\n\r");
    return first==std::string::npos?std::string{}:node->text.substr(first,node->text.find_last_not_of(" \t\n\r")-first+1);
}
std::optional<std::string> text(const DatabaseValue &value){
    if(std::holds_alternative<std::monostate>(value))return {};
    return std::get<std::string>(value);
}
runtime::ScriptValue script_value(const DatabaseValue &value){
    return std::visit([](const auto &v)->runtime::ScriptValue{return v;},value);
}
std::string ascii_lower(std::string value){
    for(auto &c:value)if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');
    return value;
}
}
ModScriptRegistry::ModScriptRegistry(const Catalog &catalog,const runtime::SavedContentProfile &profile,
                                    std::span<const ModRegistration> ordered,
                                    std::span<const std::size_t> use_installed_bytes,
                                    std::span<const runtime::VerifiedManifestHash> verified_hashes) {
    require(profile.all_requirements_located,"Cannot register an incomplete content selection");
    const std::set<std::size_t> selected(profile.enabled_mods.begin(),profile.enabled_mods.end());
    const std::set<std::size_t> installed(use_installed_bytes.begin(),use_installed_bytes.end());
    require(selected.size()==profile.enabled_mods.size(),"Duplicate selected mod");
    for(const auto index:installed)require(selected.contains(index),"Unselected installed-byte Lua package");
    std::set<std::size_t> registered;
    std::set<std::pair<std::string,std::uint64_t>> identities;
    registry_.apply_sql(schema);
    std::vector<DatabaseStatement> inserts;
    for(const auto &registration:ordered){
        const auto &package=catalog.packages.at(registration.package);
        require(package.kind==PackageKind::mod,"Mod registry requires a mod manifest");
        require(registered.insert(registration.package).second&&identities.emplace(package.id,package.version).second,
                "Duplicate mod registration");
        require(registration.row_id>0,"Mod registration requires an explicit positive row ID");
        require(package.version<=static_cast<std::uint64_t>(INT64_MAX),"Mod version exceeds SQLite signed 64-bit storage");
        const auto version=static_cast<std::int64_t>(package.version);
        inserts.push_back({"INSERT INTO Mods(RowID,ModID,Version,Type,Installed,Enabled,Activated,LastRetrieved) VALUES(?,?,?,?,?,?,?,?)",
            {registration.row_id,package.id,std::to_string(version),registration.type,
             std::int64_t(registration.installed),std::int64_t(registration.enabled),std::int64_t(selected.contains(registration.package)),
             registration.last_retrieved?DatabaseValue(*registration.last_retrieved):DatabaseValue{}}});
        if(const auto *entries=package.document.child("EntryPoints"))for(const auto &entry:entries->children){
            if(entry.name!="EntryPoint")continue;
            const auto *name=entry.child("Name"),*description=entry.child("Description");
            // Preserve authored case, file spelling and duplicate entrypoints.
            inserts.push_back({"INSERT INTO ModEntryPoints(ModID,Version,Name,Description,Type,File) VALUES(?,?,?,?,?,?)",
                {package.id,version,entry_text(name),entry_text(description),
                 entry.attribute("type"),entry.attribute("file")}});
        }
        std::map<std::string,const ModFile*,std::less<>> files;
        for(const auto &file:package.files){
            const auto [found,added]=files.emplace(file.path,&file);
            if(!added){
                // Identical declarations produce the same primary-key row.
                // Conflicting duplicate declarations need the native importer
                // error path; never guess that a later declaration wins.
                require(found->second->md5==file.md5&&found->second->import==file.import,
                        "Conflicting duplicate ModFiles declaration requires importer support");
                continue;
            }
            // GetEvaluatedFilePath is a registry lookup, not a file read. The
            // native cache retains declarations even after files disappear.
            // Actual context/include loading must validate and open its source.
            const auto relative=std::filesystem::path(std::u8string(file.path.begin(),file.path.end()));
            require(!file.path.empty()&&file.path.find('\0')==std::string::npos&&file.path.find(':')==std::string::npos&&
                    !relative.has_root_path(),"Invalid mod registry relative path");
            for(const auto &part:relative)require(part!=".."&&part!=".","Invalid mod registry path component");
            auto root=std::filesystem::absolute(package.path.parent_path());root.make_preferred();
            const auto root_text=root.u8string();
            const auto evaluated=std::string(root_text.begin(),root_text.end())+"\\"+file.path;
            inserts.push_back({"INSERT INTO ModFiles(ModID,Version,Path,md5,EvaluatedPath,Import) VALUES(?,?,?,?,?,?)",
                              {package.id,version,file.path,file.md5,evaluated,std::int64_t(file.import)}});
        }
        if(selected.contains(registration.package))
            if(const auto *entries=package.document.child("EntryPoints"))
                for(const auto &entry:entries->children){
                    if(entry.name!="EntryPoint")continue;
                    const auto path=entry.attribute("file");
                    const auto declared=files.find(path);
                    std::string verified_md5;
                    if(declared!=files.end())
                        for(const auto &hash:verified_hashes)
                            if(hash.package==registration.package&&hash.path==path&&
                               hash.declared_md5==ascii_lower(declared->second->md5)){
                                verified_md5=hash.actual_md5;
                                break;
                            }
                    entrypoints_.push_back({package.id,entry.attribute("type"),path,
                        declared==files.end()?std::string{}:declared->second->md5,
                        package.version,package.path.parent_path(),declared!=files.end(),
                        installed.contains(registration.package),verified_md5});
                }
    }
    for(const auto index:selected)require(registered.contains(index),"Selected mod is missing from the explicit registry order");
    registry_.apply_statements(inserts);
}
std::vector<runtime::ScriptRow> ModScriptRegistry::entry_points(std::string_view type)const {
    static constexpr std::array<const char*,14> names{"ModID","Version","Name","Description","Type","File",
        "RowID","ModID","Version","Type","Installed","Enabled","Activated","LastRetrieved"};
    std::vector<runtime::ScriptRow> rows;
    registry_.query({entries_sql,{std::string(type)}},[&](auto values){
        require(values.size()==names.size(),"Unexpected mod entrypoint schema");
        auto &row=rows.emplace_back();
        // The DLL converts only declared lowercase "boolean" to Lua bool.
        // This registry uses BOOL, so Installed/Enabled/Activated stay numbers.
        for(std::size_t i=0;i<names.size();++i)row.emplace_back(names[i],script_value(values[i]));
    });return rows;
}
std::vector<ModEntrypointSource> ModScriptRegistry::planned_entry_points(std::string_view type)const {
    std::vector<ModEntrypointSource> selected;
    for(const auto &entry:entrypoints_)if(entry.type==type)selected.push_back(entry);
    return selected;
}
bool execute_mod_lua_entrypoint(const ModEntrypointSource &entry,runtime::LuaScriptHost &host,
                                std::string_view context){
    if(!ascii_lower(entry.file).ends_with(".lua"))
        throw std::invalid_argument("Entrypoint is not a Lua source: "+entry.file);
    if(!entry.declared)throw std::runtime_error("Lua entrypoint is absent from manifest Files: "+entry.file);
    const auto path=package_file(entry.package_root,entry.file);
    std::ifstream in(platform::extended_windows_path(path),std::ios::binary);
    if(!in)throw std::runtime_error("Cannot read Lua entrypoint: "+entry.file);
    const std::string source{std::istreambuf_iterator<char>(in),{}};
    if(in.bad())throw std::runtime_error("Lua entrypoint read failed: "+entry.file);
    bool md5_match=true;
    if(!entry.md5.empty()){
        const auto bytes=std::span(reinterpret_cast<const std::uint8_t*>(source.data()),source.size());
        const auto actual_md5=ascii_lower(runtime::bytes_md5(bytes));
        md5_match=actual_md5==ascii_lower(entry.md5);
        if(!md5_match&&!entry.use_installed_bytes&&
           actual_md5!=entry.verified_actual_md5)
            throw std::runtime_error("Lua entrypoint manifest MD5 mismatch: "+entry.file);
    }
    host.execute(context,source,"@"+entry.file);
    return md5_match;
}
std::optional<runtime::ScriptEvaluatedFile> ModScriptRegistry::evaluated_file(std::string_view mod,
                                              std::int64_t version,std::string_view path)const {
    std::optional<runtime::ScriptEvaluatedFile> result;
    registry_.query({file_sql,{std::string(mod),version,std::string(path)}},[&](auto row){
        result=runtime::ScriptEvaluatedFile{text(row[0]),text(row[1])};
    });return result;
}
#include "mod_script_activated_entrypoint_cursor_owned.inc"
runtime::ScriptModdingProvider ModScriptRegistry::provider()const {
    return {[this](std::string_view type){return entry_points(type);},
            [this](std::string_view id,std::int64_t version,std::string_view path){return evaluated_file(id,version,path);}};
}
} // namespace civ5::content
