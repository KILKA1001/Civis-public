#pragma once
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/research_context.hpp"
#include <filesystem>

namespace civ5::runtime {
struct PregameRoster;
struct LiveSessionState;
// Read bindings retain this exact allocation. Numeric provider only; genuine
// SourceOwner currentness is still required before/after each VM operation.
// BNW IsHuman reuses the native string-option read, which has no cache stores.
ScriptGameplayProvider lua_gameplay_read_services(const content::RulesDatabase&,
    std::shared_ptr<const LiveSessionState>,const content::InfoTypeCatalog&,
    const PregameRoster *roster=nullptr);
ScriptSaveDataProvider lua_saved_data_services(std::shared_ptr<content::RulesDatabase> database);
// Session-local named mod databases. Optional source files are read once and
// copied; Lua mutations and DeleteUserData never touch the supplied directory.
ScriptUserDataProvider lua_user_data_services(std::filesystem::path source_root={});
// References the same owned team/player state used by research, and the same
// activated database/type catalog. Those owners must outlive the host. No original binary
// or file cache is opened by these bindings.
// IsHuman may resolve/cache a missing PitBoss option. VP reason queries also
// require the selected CustomMods cache, whose owner must outlive the host.
ScriptGameplayProvider lua_gameplay_services(const content::RulesDatabase&,
    ResearchContext&,const std::vector<TeamTechnologies>&,const content::InfoTypeCatalog&,
    const content::CustomModOptions *options=nullptr,const PregameRoster *roster=nullptr);
} // namespace civ5::runtime
