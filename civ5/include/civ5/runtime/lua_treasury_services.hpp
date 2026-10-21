#pragma once
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/treasury.hpp"

namespace civ5::runtime {
// Bind BNW's GetFounderBenefitsReligion CallAccumulator to an already loaded
// session-local Lua host. The host must outlive the returned callback. Mods
// supply ordinary Lua handlers; no mod-specific C++ implementation is needed.
TreasuryServices lua_treasury_services(LuaScriptHost &host);
} // namespace civ5::runtime
