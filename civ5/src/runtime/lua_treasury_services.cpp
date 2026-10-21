#include "civ5/runtime/lua_treasury_services.hpp"
#include <array>

namespace civ5::runtime {
TreasuryServices lua_treasury_services(LuaScriptHost &host) {
    if (!host.loading_complete())
        throw TreasuryUnavailable("Treasury Lua handlers are not fully loaded");
    TreasuryServices services;
    services.founder_benefits_religion=[&host](std::int64_t player)->std::optional<std::int64_t> {
        if (!host.loading_complete())
            throw TreasuryUnavailable("Treasury Lua handlers changed after binding");
        const std::array<ScriptValue,1> args{player};
        const auto result=host.accumulate_int("GetFounderBenefitsReligion",args);
        if (!result.errors.empty())
            throw TreasuryUnavailable("GetFounderBenefitsReligion Lua handler failed: "+result.errors.front());
        return result.value; // No listeners: native BNW founder selection runs.
    };
    return services;
}
} // namespace civ5::runtime
