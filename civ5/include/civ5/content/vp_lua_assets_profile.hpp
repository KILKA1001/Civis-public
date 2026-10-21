#pragma once
#include "civ5/content/lua_assets.hpp"
#include "civ5/content/vp_profile.hpp"
#include "civ5/runtime/save_preflight.hpp"

namespace civ5::content {
// The verified EUI installer intentionally omits some Lua imports declared by
// its unchanged CP/VP manifests. Only an exact selected manifest may inherit
// those omissions; other mods remain subject to normal file/MD5 checks.
std::vector<LuaImportOmission> vp_lua_import_omissions(
    const VpProfile &installer,const Catalog &catalog,
    const runtime::SavedContentProfile &selected);
} // namespace civ5::content
