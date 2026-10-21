#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include <span>
namespace civ5::runtime { struct SavedContentProfile; struct VerifiedManifestHash; }
namespace civ5::content {
// Caller supplies the authoritative ordered IncludeFileList for each context.
// The same mount table used by other asset consumers selects source payloads.
// resources must outlive the resulting provider/host. No filesystem fallback.
runtime::ScriptSourceProvider lua_asset_sources(
    const assets::ResourceMountTable &resources,
    std::function<std::vector<std::string>(std::string_view)> ordered_files,
    std::uint64_t source_budget=64ULL*1024*1024);
// Make retail Assets/Gameplay/Lua available to include() from the selected
// installation. Existing selected DLC/mod mounts keep precedence.
std::uint64_t mount_base_game_lua(assets::ResourceMountTable &resources,
                                  const std::filesystem::path &game_root);
struct LuaModMountReport {
    std::uint64_t declared=0,mounted=0,profile_omitted=0,manifest_md5_mismatches=0;
    std::vector<std::string> failures;
    bool complete()const{return failures.empty()&&declared==mounted+profile_omitted;}
};
// A verified compatibility profile may deliberately omit declared imported
// Lua files (for example when another UI component replaces them). This is a
// generic, exact manifest declaration; the common loader knows no VP policy.
struct LuaImportOmission {
    std::string mod_id,path,md5;
    std::uint64_t version=0;
};
// Registration order is explicit; the saved header/catalog are not assumed to
// reproduce the original Modding registry order. Only selected, declared
// import="1" Lua files enter the VFS. Manifest MD5 is strict by default; a
// caller may select installed packages for a diagnostic replay of local edits.
LuaModMountReport mount_selected_mod_lua(assets::ResourceMountTable &resources,
    const Catalog &catalog,const runtime::SavedContentProfile &profile,
    std::span<const std::size_t> registration_order,
    std::span<const LuaImportOmission> profile_omissions={},
    std::span<const std::size_t> use_installed_bytes={},
    std::span<const runtime::VerifiedManifestHash> verified_hashes={});
} // namespace civ5::content
