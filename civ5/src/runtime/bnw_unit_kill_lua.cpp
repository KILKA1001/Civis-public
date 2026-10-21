#include "civ5/runtime/bnw_unit_kill_lua.hpp"
#include <array>
#include <limits>
#include <string>

namespace civ5::runtime {
namespace {
void check_source_int(std::int64_t value,const char *name) {
    if (value<INT32_MIN || value>INT32_MAX)
        throw ScriptError(std::string("BNW kill Lua ")+name+
                          " is outside source int range");
}
void check_result(const ScriptTestResult &result,const char *event) {
    if (!result.errors.empty())
        throw ScriptError(std::string("BNW kill Lua ")+event+
                          " callback failed: "+result.errors.front());
}
} // namespace

BnwUnitKillLuaResult call_bnw_unit_kill_lua(
    LuaScriptHost &host,const BnwUnitKillLuaArguments &arguments) {
    if (!host.loading_complete())
        throw ScriptError("BNW kill Lua host has not loaded selected entrypoints");
    check_source_int(arguments.owner,"owner");
    check_source_int(arguments.unit_id,"unit ID");
    check_source_int(arguments.unit_type,"unit type");
    check_source_int(arguments.x,"x");
    check_source_int(arguments.y,"y");
    check_source_int(arguments.killer,"killer");
    if (arguments.owner<0 || arguments.unit_id<0 || arguments.unit_type<0)
        throw ScriptError("BNW kill Lua unit identity is invalid");

    const std::array<ScriptValue,2> test_args{
        arguments.owner,arguments.unit_id};
    const auto can_save=host.test_all("CanSaveUnit",test_args);
    check_result(can_save,"CanSaveUnit");
    if (can_save.value && *can_save.value)
        return {BnwUnitKillLuaDecision::saved_by_script,true,false};

    const std::array<ScriptValue,7> hook_args{
        arguments.owner,arguments.unit_id,arguments.unit_type,
        arguments.x,arguments.y,arguments.delayed,arguments.killer};
    const auto prekill=host.call_hook("UnitPrekill",hook_args);
    check_result(prekill,"UnitPrekill");
    return {BnwUnitKillLuaDecision::proceed,
            can_save.value.has_value(),prekill.value.has_value()};
}

} // namespace civ5::runtime
