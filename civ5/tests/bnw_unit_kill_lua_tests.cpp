#include "civ5/runtime/bnw_unit_kill_lua.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5::runtime;
void check(bool valid,const char *message) {
    if (!valid) throw std::runtime_error(message);
}
template<class F> void rejects(F run) {
    bool caught=false;
    try {run();} catch (const ScriptError &) {caught=true;}
    check(caught,"Expected BNW kill Lua blocker");
}
} // namespace

int main() {
    try {
        // Expansion2 CvUnit.cpp:1166 tests CanSaveUnit before UnitPrekill.
        const BnwUnitKillLuaArguments delayed{5,7,11,3,4,true,-1};
        LuaScriptHost unloaded;
        rejects([&]{(void)call_bnw_unit_kill_lua(unloaded,delayed);});
        unloaded.complete_loading();
        const auto absent=call_bnw_unit_kill_lua(unloaded,delayed);
        check(absent.decision==BnwUnitKillLuaDecision::proceed &&
              !absent.can_save_handled && !absent.prekill_handled,
              "No-listener Lua result was mistaken for a save veto");

        LuaScriptHost saving;
        saving.execute("gameplay",R"lua(
            trace={}
            GameEvents.CanSaveUnit.Add(function(owner,id)
                assert(owner==5 and id==7)
                trace[#trace+1]='save'
                return true
            end)
            GameEvents.UnitPrekill.Add(function(...)
                trace[#trace+1]='prekill'
            end)
            GameEvents.Check.Add(function()
                return #trace==1 and trace[1]=='save'
            end)
        )lua","=save-unit.lua");
        saving.complete_loading();
        const auto saved=call_bnw_unit_kill_lua(saving,delayed);
        check(saved.decision==BnwUnitKillLuaDecision::saved_by_script &&
              saved.can_save_handled && !saved.prekill_handled &&
              saving.test_all("Check").value==true,
              "CanSaveUnit true did not cancel kill before UnitPrekill");

        LuaScriptHost mixed;
        mixed.execute("gameplay",R"lua(
            count=0
            GameEvents.CanSaveUnit.Add(function() count=count+1; return true end)
            GameEvents.CanSaveUnit.Add(function() count=count+1; return false end)
            GameEvents.UnitPrekill.Add(function() count=count+1 end)
            GameEvents.Check.Add(function() return count==3 end)
        )lua","=mixed-save.lua");
        mixed.complete_loading();
        check(call_bnw_unit_kill_lua(mixed,delayed).decision==
                  BnwUnitKillLuaDecision::proceed &&
              mixed.test_all("Check").value==true,
              "CanSaveUnit must use TestAll across every listener");

        LuaScriptHost proceeding;
        proceeding.execute("gameplay",R"lua(
            trace={}
            GameEvents.CanSaveUnit.Add(function(owner,id)
                assert(owner==5 and id==7)
                trace[#trace+1]='save'
                return false
            end)
            GameEvents.UnitPrekill.Add(function(owner,id,kind,x,y,delay,killer)
                assert(owner==5 and id==7 and kind==11 and x==3 and y==4)
                assert(killer==-1 and type(delay)=='boolean')
                trace[#trace+1]=delay and 'prekill:true' or 'prekill:false'
                return false -- Hook result cannot cancel kill.
            end)
            GameEvents.Check.Add(function()
                return table.concat(trace,',')==
                    'save,prekill:true,save,prekill:false'
            end)
        )lua","=prekill.lua");
        proceeding.complete_loading();
        const auto first=call_bnw_unit_kill_lua(proceeding,delayed);
        auto immediate=delayed;immediate.delayed=false;
        const auto second=call_bnw_unit_kill_lua(proceeding,immediate);
        check(first.decision==BnwUnitKillLuaDecision::proceed &&
              second.decision==BnwUnitKillLuaDecision::proceed &&
              first.can_save_handled && first.prekill_handled &&
              second.can_save_handled && second.prekill_handled &&
              proceeding.test_all("Check").value==true,
              "kill(true)/kill(false) Lua order, arguments or ignored Hook result changed");

        LuaScriptHost failed_test;
        failed_test.execute("gameplay",R"lua(
            called=false
            GameEvents.CanSaveUnit.Add(function() error('test failed') end)
            GameEvents.UnitPrekill.Add(function() called=true end)
            GameEvents.Check.Add(function() return not called end)
        )lua","=failed-test.lua");
        failed_test.complete_loading();
        rejects([&]{(void)call_bnw_unit_kill_lua(failed_test,delayed);});
        check(failed_test.test_all("Check").value==true,
              "Failed CanSaveUnit still reached UnitPrekill");
        LuaScriptHost failed_hook;
        failed_hook.execute("gameplay",R"lua(
            GameEvents.CanSaveUnit.Add(function() return false end)
            GameEvents.UnitPrekill.Add(function() error('hook failed') end)
        )lua","=failed-hook.lua");
        failed_hook.complete_loading();
        rejects([&]{(void)call_bnw_unit_kill_lua(failed_hook,delayed);});
        auto out_of_range=delayed;out_of_range.unit_id=INT64_MAX;
        rejects([&]{(void)call_bnw_unit_kill_lua(proceeding,out_of_range);});
        std::cout << "PASS BNW unit kill Lua bridge\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW unit kill Lua bridge: " << error.what() << '\n';
        return 1;
    }
}
