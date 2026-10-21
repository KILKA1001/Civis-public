#include "civ5/content/lua_assets.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <tuple>
namespace civ5::content {
namespace {
std::uint64_t free_lua_mount_id(const assets::ResourceMountTable &resources){
    std::vector<std::uint64_t> ids;
    ids.reserve(resources.mounts().size());
    for(const auto &mount:resources.mounts())ids.push_back(mount.id);
    std::sort(ids.begin(),ids.end());
    std::uint64_t candidate=1;
    for(const auto id:ids){
        if(id<candidate)continue;
        if(id>candidate)break;
        if(candidate==std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("No free wide mount ID for Lua assets");
        ++candidate;
    }
    return candidate;
}
}
runtime::ScriptSourceProvider lua_asset_sources(const assets::ResourceMountTable &resources,
    std::function<std::vector<std::string>(std::string_view)> ordered_files,std::uint64_t source_budget) {
    if(!ordered_files)throw std::invalid_argument("Lua assets require an ordered file inventory");
    return {std::move(ordered_files),[&resources,source_budget](std::string_view,std::string_view name){
        const auto selected=resources.resolve(assets::AssetKey(std::string(name)));
        if(const auto *handle=std::get_if<assets::AssetHandle>(&selected.asset)){
            const auto bytes=handle->read_bytes(source_budget);return std::string(bytes.begin(),bytes.end());
        }
        if(std::holds_alternative<assets::AmbiguousAsset>(selected.asset))throw runtime::ScriptError("Ambiguous Lua source: "+std::string(name));
        throw runtime::ScriptError("Missing Lua source: "+std::string(name));
    }};
}
std::uint64_t mount_base_game_lua(assets::ResourceMountTable &resources,
                                  const std::filesystem::path &game_root) {
    const auto root=std::filesystem::canonical(game_root);
    auto source=std::make_shared<assets::LooseResourceIndex>(root);
    source->scan({"Assets/Gameplay/Lua",{},true});
    const auto id=free_lua_mount_id(resources);
    // Native resolve() prefers existing selected mounts/default over this
    // non-preferred fallback, while keys() still discovers the base Lua names.
    resources.mount({id,0,0,assets::resource_lookup(source,false),true,false,
                     assets::resource_lookup_all(source,false),assets::resource_keys(source),source});
    return source->indexed_files();
}
LuaModMountReport mount_selected_mod_lua(assets::ResourceMountTable &resources,
    const Catalog &catalog,const runtime::SavedContentProfile &profile,
    std::span<const std::size_t> registration_order,
    std::span<const LuaImportOmission> profile_omissions,
    std::span<const std::size_t> use_installed_bytes,
    std::span<const runtime::VerifiedManifestHash> verified_hashes){
    if(!profile.all_requirements_located)
        throw std::invalid_argument("Lua mounts require all selected manifests");
    const std::set<std::size_t> selected(profile.enabled_mods.begin(),profile.enabled_mods.end());
    const std::set<std::size_t> installed(use_installed_bytes.begin(),use_installed_bytes.end());
    if(selected.size()!=profile.enabled_mods.size())
        throw std::invalid_argument("Duplicate selected Lua mod");
    for(const auto index:installed)if(!selected.contains(index))
        throw std::invalid_argument("Unselected installed-byte Lua package");
    const auto lower=[](std::string value){
        for(auto &c:value)if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');
        return value;
    };
    using OmissionKey=std::tuple<std::string,std::uint64_t,std::string>;
    std::set<OmissionKey> omitted;
    for(const auto &entry:profile_omissions){
        const auto key=OmissionKey{lower(entry.mod_id),entry.version,entry.path};
        if(!omitted.insert(key).second)throw std::invalid_argument("Duplicate Lua profile omission");
        std::size_t matches=0;
        for(const auto index:selected){
            const auto &package=catalog.packages.at(index);
            if(package.kind!=PackageKind::mod||package.id!=std::get<0>(key)||package.version!=entry.version)
                continue;
            for(const auto &file:package.files)
                if(file.path==entry.path&&file.import&&lower(file.path).ends_with(".lua")&&
                   lower(file.md5)==lower(entry.md5))++matches;
        }
        if(matches!=1)throw std::invalid_argument("Lua profile omission does not match one selected manifest import");
    }
    std::set<std::size_t> registered;
    LuaModMountReport report;
    for(const auto package_index:registration_order){
        const auto &package=catalog.packages.at(package_index);
        if(!registered.insert(package_index).second)
            throw std::invalid_argument("Duplicate Lua mod registration");
        if(!selected.contains(package_index))continue;
        if(package.kind!=PackageKind::mod)
            throw std::invalid_argument("Lua mount requires mod package");
        const auto root=package.path.parent_path();
        auto source=std::make_shared<assets::LooseResourceIndex>(root);
        std::map<std::string,std::pair<std::string,bool>> seen;
        for(const auto &file:package.files){
            const auto [prior,unique]=seen.emplace(file.path,std::pair{file.md5,file.import});
            if(!unique){
                if(prior->second!=std::pair{file.md5,file.import})
                    report.failures.push_back(package.id+"/"+file.path+": conflicting file declarations");
                continue;
            }
            if(!file.import||!lower(file.path).ends_with(".lua"))continue;
            ++report.declared;
            if(omitted.contains({package.id,package.version,file.path})){
                ++report.profile_omitted;
                continue;
            }
            try{
                const auto path=package_file(root,file.path);
                if(!file.md5.empty()){
                    const auto actual=lower(runtime::file_md5(path));
                    if(actual!=lower(file.md5)){
                        ++report.manifest_md5_mismatches;
                        const auto accepted=std::any_of(verified_hashes.begin(),verified_hashes.end(),
                            [&](const runtime::VerifiedManifestHash &verified){
                                return verified.package==package_index&&verified.path==file.path&&
                                       verified.declared_md5==lower(file.md5)&&
                                       verified.actual_md5==actual;
                            });
                        if(!installed.contains(package_index)&&!accepted)
                            throw std::runtime_error("manifest MD5 mismatch");
                    }
                }
                source->add_file(file.path);
                ++report.mounted;
            }catch(const std::exception &error){
                report.failures.push_back(package.id+"/"+file.path+": "+error.what());
            }
        }
        if(source->indexed_files()!=0){
            resources.mount({free_lua_mount_id(resources),0,0,assets::resource_lookup(source),true,true,
                             assets::resource_lookup_all(source),assets::resource_keys(source)});
        }
    }
    for(const auto package_index:selected)if(!registered.contains(package_index))
        throw std::invalid_argument("Selected Lua mod missing from registration order");
    return report;
}
} // namespace civ5::content
