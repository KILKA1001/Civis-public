#include "civ5/runtime/bnw_can_declare_war_lua.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5::runtime;
void check(bool yes,const char *message) {
    if(!yes) throw std::runtime_error(message);
}
template<class F> void rejects(F &&run) {
    bool caught=false;
    try {run();} catch(const ScriptError &) {caught=true;}
    check(caught,"BNW CanDeclareWar Lua bridge missed a blocker");
}
}

int main() {
    using namespace civ5::runtime;
    try {
        // Expansion2 CvTeam.cpp:1007-1025 pushes own team then target team.
        rejects([&]{(void)call_bnw_can_declare_war_lua(nullptr,3,7);});
        LuaScriptHost unloaded;
        rejects([&]{(void)call_bnw_can_declare_war_lua(&unloaded,3,7);});
        unloaded.complete_loading();
        const auto absent=call_bnw_can_declare_war_lua(&unloaded,3,7);
        check(absent.decision==BnwCanDeclareWarLuaDecision::allow &&
              !absent.handled,
              "No-listener CallTestAll should leave canDeclareWar allowed");

        LuaScriptHost ordered;
        ordered.execute("gameplay",R"lua(
            trace={}
            GameEvents.CanDeclareWar.Add(function(ours,theirs)
                assert(ours==3 and theirs==7)
                trace[#trace+1]='first'
                return true
            end)
            GameEvents.CanDeclareWar.Add(function(ours,theirs)
                assert(ours==3 and theirs==7)
                trace[#trace+1]='second'
                return false
            end)
            GameEvents.Check.Add(function()
                return table.concat(trace,',')=='first,second'
            end)
        )lua","=can-declare-war.lua");
        ordered.complete_loading();
        const auto veto=call_bnw_can_declare_war_lua(&ordered,3,7);
        check(veto.decision==BnwCanDeclareWarLuaDecision::veto &&
              veto.handled && ordered.test_all("Check").value==true,
              "CanDeclareWar changed argument/order or TestAll veto semantics");
        rejects([&]{(void)call_bnw_can_declare_war_lua(&ordered,7,3);});

        LuaScriptHost allowed;
        allowed.execute("gameplay",R"lua(
            GameEvents.CanDeclareWar.Add(function(ours,theirs)
                return ours==3 and theirs==7
            end)
        )lua","=allow-war.lua");
        allowed.complete_loading();
        check(call_bnw_can_declare_war_lua(&allowed,3,7).decision==
                  BnwCanDeclareWarLuaDecision::allow,
              "Handled true CanDeclareWar should allow");

        LuaScriptHost failed;
        failed.execute("gameplay",R"lua(
            GameEvents.CanDeclareWar.Add(function() error('war test failed') end)
        )lua","=failed-war.lua");
        failed.complete_loading();
        rejects([&]{(void)call_bnw_can_declare_war_lua(&failed,3,7);});
        rejects([&]{(void)call_bnw_can_declare_war_lua(&allowed,-1,7);});
        rejects([&]{(void)call_bnw_can_declare_war_lua(&allowed,3,63);});
        rejects([&]{(void)call_bnw_can_declare_war_lua(&allowed,3,3);});
        std::cout << "BNW CanDeclareWar Lua bridge PASS\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
