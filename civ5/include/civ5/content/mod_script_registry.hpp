#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/save_preflight.hpp"

namespace civ5::runtime {class BnwSelectedStockLuaBootstrap;}
namespace civ5::content {
struct ModRegistration {
    std::size_t package=0;
    std::int64_t row_id=0,type=1;
    bool installed=true,enabled=false;
    std::optional<std::string> last_retrieved;
};
struct ModEntrypointSource {
    std::string mod_id, type, file, md5;
    std::uint64_t version=0;
    std::filesystem::path package_root;
    bool declared=false;
    bool use_installed_bytes=false;
    std::string verified_actual_md5;
};
// Source SQL registry, separate from the gameplay database and original cache.
// Registration order/metadata are explicit inputs, not save or catalog order.
// All selected mods must be registered exactly once; extra registered mods
// stay inactive. A native DLL still requires its own x64 gameplay port.
class ModScriptRegistry {
  public:
    ModScriptRegistry(const Catalog&,const runtime::SavedContentProfile&,
                      std::span<const ModRegistration> ordered_registration,
                      std::span<const std::size_t> use_installed_bytes={},
                      std::span<const runtime::VerifiedManifestHash> verified_hashes={});
    std::vector<runtime::ScriptRow> entry_points(std::string_view type) const;
    // Selected entrypoints in explicit registration and authored manifest order.
    // XML and other kinds remain in this plan for their owning frontend.
    std::vector<ModEntrypointSource> planned_entry_points(std::string_view type) const;
    std::optional<runtime::ScriptEvaluatedFile> evaluated_file(std::string_view mod,
                           std::int64_t version,std::string_view path) const;
    // This registry must outlive its Lua host and remain immutable.
    runtime::ScriptModdingProvider provider() const;
  private:
    friend class ::civ5::runtime::BnwSelectedStockLuaBootstrap;
    std::unique_ptr<runtime::ScriptQuery> activated_entrypoint_query_owned(std::string_view)const;
    RulesDatabase registry_;
    std::vector<ModEntrypointSource> entrypoints_;
};
// Run a declared Lua entrypoint in the caller's explicit context. Manifest MD5
// is strict unless this source belongs to an explicitly selected local package.
// The owner completes LuaScriptHost loading only after every selected entrypoint
// and its UI/XML dependencies have been handled.
// Returns whether the executed source still matches its manifest MD5.
bool execute_mod_lua_entrypoint(const ModEntrypointSource&,runtime::LuaScriptHost&,
                                std::string_view context);
} // namespace civ5::content
