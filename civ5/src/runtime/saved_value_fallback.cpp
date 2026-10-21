#include "civ5/runtime/saved_value_fallback.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/runtime/lua_locale_services.hpp"
#include <charconv>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>

namespace civ5::runtime {
namespace {
std::uint64_t version(std::string_view value){
    std::uint64_t number=0;
    const auto [end,error]=std::from_chars(value.data(),value.data()+value.size(),number);
    if(value.empty()||error!=std::errc{}||end!=value.data()+value.size())
        throw std::runtime_error("Invalid saved-value fallback version");
    return number;
}
void valid_name(std::string_view name){
    if(name.empty()||name=="."||name==".."||name.find('\0')!=name.npos||
       name.find_first_of("\\/:*?\"<>|")!=name.npos)
        throw std::runtime_error("Invalid saved-value fallback user-data name");
}
}
SavedValueFallbackPlan parse_saved_value_fallback_plan(const content::XmlNode &root){
    if(root.name!="SavedValueFallback")throw std::runtime_error("Expected SavedValueFallback plan");
    SavedValueFallbackPlan plan;
    plan.mod_id=content::canonical_guid(root.attribute("modId"));
    plan.mod_version=version(root.attribute("modVersion"));
    plan.user_data_name=root.attribute("userData");
    if(!plan.user_data_name.empty())valid_name(plan.user_data_name);
    const auto user_version=root.attribute("userDataVersion");
    if(!user_version.empty()){
        const auto parsed=version(user_version);
        if(parsed>INT64_MAX)throw std::runtime_error("User-data version exceeds x64 range");
        plan.user_data_version=static_cast<std::int64_t>(parsed);
    }
    if(root.children.empty()||root.children.size()>1024)
        throw std::runtime_error("Saved-value fallback needs 1..1024 keys");
    std::set<std::string> unique;
    for(const auto &node:root.children){
        if(node.name=="LuaPreset"){
            if(plan.lua_preset)throw std::runtime_error("Duplicate saved-value Lua preset");
            SavedValueFallbackPlan::LuaPreset preset;
            preset.call=node.attribute("call");
            if(preset.call.empty()||preset.call.size()>1024||preset.call.find('\0')!=preset.call.npos||
               node.children.empty()||node.children.size()>16)
                throw std::runtime_error("Invalid saved-value Lua preset");
            std::set<std::string> scripts;
            for(const auto &script:node.children){
                if(script.name!="Script"||!script.children.empty())
                    throw std::runtime_error("Invalid saved-value Lua script entry");
                auto path=script.attribute("path");
                if(path.empty()||path.size()>512||!scripts.insert(path).second||
                   ascii_lower(std::filesystem::path(std::u8string(path.begin(),path.end())).extension().string())!=".lua")
                    throw std::runtime_error("Invalid or duplicate saved-value Lua script path");
                preset.scripts.push_back(std::move(path));
            }
            plan.lua_preset=std::move(preset);
            continue;
        }
        if(node.name!="Key"||!node.children.empty())throw std::runtime_error("Invalid saved-value fallback key element");
        auto key=node.attribute("name");
        if(key.empty()||key.size()>256||key.find('\0')!=key.npos||!unique.insert(key).second)
            throw std::runtime_error("Invalid or duplicate saved-value fallback key");
        plan.keys.push_back(std::move(key));
    }
    if(plan.keys.empty()||
       (plan.lua_preset?(!plan.user_data_name.empty()||!user_version.empty()):
                        (plan.user_data_name.empty()||user_version.empty())))
        throw std::runtime_error("Saved-value fallback needs exactly one source and at least one key");
    return plan;
}
SavedValueFallbackPlan read_saved_value_fallback_plan(const std::filesystem::path &path){
    return parse_saved_value_fallback_plan(content::read_xml(path,64*1024));
}
std::shared_ptr<content::RulesDatabase> load_saved_value_fallback_source(
    const std::filesystem::path &user_data_root,const SavedValueFallbackPlan &plan){
    if(plan.lua_preset)throw std::runtime_error("Lua preset is not a ModUserData source");
    valid_name(plan.user_data_name);
    if(user_data_root.empty())throw std::runtime_error("Saved-value fallback needs a user-data root");
    const auto root=std::filesystem::canonical(user_data_root);
    if(!std::filesystem::is_directory(root))throw std::runtime_error("User-data root is not a directory");
    const auto name=plan.user_data_name+"-"+std::to_string(plan.user_data_version)+".db";
    const auto candidate=root/std::filesystem::path(std::u8string(name.begin(),name.end()));
    if(!std::filesystem::exists(candidate))throw std::runtime_error("Saved-value fallback source is absent");
    const auto resolved=std::filesystem::canonical(candidate);
    if(resolved.parent_path()!=root||!std::filesystem::is_regular_file(resolved)||
       std::filesystem::file_size(resolved)>64ULL*1024*1024)
        throw std::runtime_error("Saved-value fallback source is invalid or escapes its directory");
    std::ifstream input(resolved,std::ios::binary);
    if(!input)throw std::runtime_error("Cannot read saved-value fallback source");
    const std::vector<std::uint8_t> image(std::istreambuf_iterator<char>{input},{});
    if(!input.eof()&&input.fail())throw std::runtime_error("Incomplete saved-value fallback source");
    return content::RulesDatabase::from_saved_image(image);
}
std::shared_ptr<content::RulesDatabase> load_saved_value_fallback_lua_source(
    const content::PackageManifest &package,const content::RulesDatabase &activated_rules,
    const SavedValueFallbackPlan &plan,std::string_view ui_language){
    if(!plan.lua_preset||package.kind!=content::PackageKind::mod||
       package.id!=plan.mod_id||package.version!=plan.mod_version)
        throw std::runtime_error("Saved-value Lua preset requires the selected exact mod version");
    auto source=content::RulesDatabase::new_saved_values();
    std::optional<ScriptLocaleSession> locale;
    if(!ui_language.empty())locale.emplace(activated_rules,std::string(ui_language));
    LuaScriptHost scripts({64ULL*1024*1024,10000000},{},{},{},
        lua_saved_data_services(source),{},{},{},{},{},{},
        locale?locale->provider():ScriptLocaleProvider{});
    std::uint64_t total_bytes=0;
    for(const auto &relative:plan.lua_preset->scripts){
        const content::ModFile *declared=nullptr;
        for(const auto &file:package.files)if(file.path==relative){
            if(declared)throw std::runtime_error("Duplicate Lua preset file in mod manifest");
            declared=&file;
        }
        if(!declared||declared->md5.empty())
            throw std::runtime_error("Lua preset file is not verified by mod manifest: "+relative);
        const auto path=content::package_file(package.path.parent_path(),relative);
        const auto bytes=std::filesystem::file_size(path);
        if(bytes>2ULL*1024*1024||total_bytes>8ULL*1024*1024-bytes)
            throw std::runtime_error("Lua preset source exceeds the caller budget");
        total_bytes+=bytes;
        std::ifstream input(path,std::ios::binary);
        if(!input)throw std::runtime_error("Cannot read Lua preset file: "+relative);
        std::string text(static_cast<std::size_t>(bytes),'\0');
        input.read(text.data(),static_cast<std::streamsize>(text.size()));
        if(input.gcount()!=static_cast<std::streamsize>(text.size())||input.peek()!=std::char_traits<char>::eof())
            throw std::runtime_error("Lua preset file changed during read: "+relative);
        const auto exact=std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(text.data()),text.size());
        if(ascii_lower(bytes_md5(exact))!=ascii_lower(declared->md5))
            throw std::runtime_error("Lua preset file differs from selected mod manifest: "+relative);
        scripts.execute("saved-value-preset",text,"@"+relative);
    }
    scripts.execute("saved-value-preset",plan.lua_preset->call,"=saved-value-preset-call");
    scripts.complete_loading();
    return source;
}
SavedValueFallbackReport fill_missing_saved_values(content::RulesDatabase &saved,
    const content::RulesDatabase &user_data,const SavedValueFallbackPlan &plan){
    SavedValueFallbackReport report;
    std::set<std::string> unique;
    std::vector<content::DatabaseStatement> inserts;
    for(const auto &key:plan.keys){
        if(key.empty()||key.find('\0')!=key.npos||!unique.insert(key).second)
            throw std::runtime_error("Invalid or duplicate saved-value fallback key");
        std::optional<content::DatabaseValue> saved_value;
        saved.query({"SELECT Value FROM SimpleValues WHERE Name=? LIMIT 1",{key}},
            [&](std::span<const content::DatabaseValue> row){saved_value=row[0];});
        std::optional<content::DatabaseValue> value;
        user_data.query({"SELECT Value FROM SimpleValues WHERE Name=? LIMIT 1",{key}},
            [&](std::span<const content::DatabaseValue> row){value=row[0];});
        if(saved_value){
            ++report.present;
            if(std::holds_alternative<std::monostate>(*saved_value))++report.present_null;
            else if(value&&!std::holds_alternative<std::monostate>(*value)){
                if(*saved_value!=*value)
                    throw std::runtime_error("Saved-value fallback source conflicts with saved key: "+key);
                ++report.matching_source;
            }
            continue;
        }
        if(!value||std::holds_alternative<std::monostate>(*value)){
            ++report.missing_source;report.missing_keys.push_back(key);continue;
        }
        inserts.push_back({"INSERT OR IGNORE INTO SimpleValues(Name,Value) VALUES(?,?)",{key,*value}});
    }
    if(!inserts.empty())saved.apply_statements(inserts);
    report.copied=inserts.size();
    return report;
}
} // namespace civ5::runtime
