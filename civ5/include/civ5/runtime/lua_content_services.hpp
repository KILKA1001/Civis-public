#pragma once
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include <algorithm>
#include <map>
#include <set>

namespace civ5::runtime {
// The save-selected DLC set, not every DLC found beside the installation,
// determines gameplay activity. This service has no VP or GameCore policy.
inline ScriptContentProvider lua_content_services(const content::Catalog &catalog,
                                                   const SavedContentProfile &profile){
    ScriptContentProvider result;result.available=true;
    std::set<std::string> active;
    for(const auto index:profile.enabled_dlc){
        const auto &package=catalog.packages.at(index);
        if(package.kind!=content::PackageKind::dlc)throw ScriptError("Selected content package is not DLC");
        active.insert(package.id);
    }
    std::set<std::string> seen;
    for(const auto &package:catalog.packages){
        if(package.kind!=content::PackageKind::dlc||!seen.insert(package.id).second)continue;
        auto id=package.id;
        std::transform(id.begin(),id.end(),id.begin(),[](unsigned char c){
            return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):static_cast<char>(c);
        });
        result.packages.push_back({std::move(id),package.name,active.contains(package.id)});
    }
    return result;
}
} // namespace civ5::runtime
