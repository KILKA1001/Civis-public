#pragma once
void lua_event_dispatch_tests(){
    using namespace civ5::runtime;
    LuaScriptHost host;
    check(!host.call_hook("Absent").value&&!host.test_any("Absent").value&&
          !host.accumulate_int("Absent",{},91).value&&!host.accumulate("Absent",{},1.25).value,
          "Every event dispatch mode must distinguish no listeners from false or zero");
    host.execute("A",R"lua(
        hits=0
        GameEvents.Mixed.Add(function(id,s) assert(id==5000000001 and s=='a\0b'); hits=hits+1; return false end)
        GameEvents.Mixed.Add(function() hits=hits+1; return 0 end)
        GameEvents.Integer.Add(function(...) assert(select('#',...)==0); return '2.7' end)
        GameEvents.Integer.Add(function() return -1.7 end)
        GameEvents.Integer.Add(function() return true end)
        GameEvents.Integer.Add(function() return {} end)
        GameEvents.Integer.Add(function() return 'not a number' end)
        GameEvents.Integer.Add(function() end)
        GameEvents.Integer.Add(function() return 5000000001,999 end)
        GameEvents.Fractions.Add(function() return 0.6 end)
        GameEvents.Fractions.Add(function() return 0.6 end)
        GameEvents.Tie.Add(function() return 2.5 end)
        GameEvents.Tie.Add(function() return 3.5 end)
        GameEvents.Tie.Add(function() return -2.5 end)
        GameEvents.Fault.Add(function() error('event callback failed') end)
        GameEvents.Fault.Add(function() hits=hits+1; return 7 end)
        GameEvents.AllFaults.Add(function() error({}) end)
        GameEvents.Real.Add(function() return '1.25' end)
        GameEvents.Real.Add(function() return 2.5 end)
        GameEvents.Real.Add(function() return false end)
        GameEvents.WideReal.Add(function() return 16777216 end)
        GameEvents.WideReal.Add(function() return 1 end)
        GameEvents.WideReal.Add(function() return -16777216 end)
        GameEvents.LargeReal.Add(function() return 1e50 end)
        GameEvents.MinusOne.Add(function() return -1 end)
        GameEvents.PlusOne.Add(function() return 1 end)
        GameEvents.TooWide.Add(function() return 9007199254740992 end)
        GameEvents.TooWide.Add(function() return 1 end)
        GameEvents.Infinite.Add(function() return math.huge end)
        GameEvents.NotANumber.Add(function() return 0/0 end)
        GameEvents.Yield.Add(function() coroutine.yield() end)
    )lua","=event-modes.lua");
    const std::array<ScriptValue,2> args{std::int64_t{5000000001LL},std::string("a\0b",3)};
    check(host.call_hook("Mixed",args).value==false,"Hook Call must AND callback truthiness within its context");
    check(host.test_any("Mixed",args).value==true,"TestAny must OR callback truthiness including numeric zero");
    host.execute("A","assert(hits==4)","=no-short-circuit.lua");
    check(host.accumulate_int("Integer",{},100).value==5000000102LL,"Integer accumulator must convert each result and add caller initial value outside arguments");
    check(host.accumulate_int("Fractions").value==2,"Retail Lua FISTP rounds each fractional callback, not the final sum");
    check(host.accumulate_int("Tie").value==4,"Default native rounding is nearest with ties to even");
    const int rounding=std::fegetround();
    struct RestoreRounding {int mode;~RestoreRounding(){std::fesetround(mode);}} restore{rounding};
    check(std::fesetround(FE_TOWARDZERO)==0,"Test process must support native rounding control");
    const auto truncated=host.accumulate_int("Fractions").value;
    std::fesetround(rounding);
    check(truncated==0,"Integer conversion must honor the active process rounding mode");
    auto fault=host.accumulate_int("Fault",{},20);
    check(fault.value==27&&fault.errors.size()==1,"Failed numeric callback must be recorded and skipped while later callbacks still run");
    check(host.test_any("Fault").value==true&&host.call_hook("Fault").value==true,"Boolean modes must skip failed results and continue");
    check(host.accumulate_int("AllFaults",{},23).value==23&&host.accumulate_int("AllFaults").errors.size()==1,
          "An all-failed nonempty list remains a handled accumulator with zero contribution and diagnostics");
    check(host.test_any("AllFaults").value==false&&host.call_hook("AllFaults").value==true,"All-failed modes must preserve their native initial result");
    check(host.accumulate("Real",{},0.5).value==4.25,"Real accumulator must accept numeric strings and add initial value");
    check(host.accumulate("WideReal").value==1&&host.accumulate("LargeReal").value==1e50,"Real accumulation must not inherit float32 rounding or range limits");
    check(host.accumulate_int("MinusOne",{},INT64_MAX).value==INT64_MAX-1,"Native caller initial values stay exact int64 outside the Lua VM");
    rejects([&]{host.accumulate_int("PlusOne",{},INT64_MAX);});
    rejects([&]{host.accumulate_int("TooWide");});
    rejects([&]{host.accumulate_int("Infinite");});
    rejects([&]{host.accumulate("Infinite");});
    rejects([&]{host.accumulate("NotANumber");});
    check(host.call_hook("Yield").errors.size()==1&&host.accumulate_int("Yield").errors.size()==1,"Every event mode must use protected non-yielding callback threads");
    host.execute("A",R"lua(
        assert(GameEvents.Absent.Call()==nil and GameEvents.Absent.TestAny()==nil)
        assert(GameEvents.Absent.Accumulate()==nil and GameEvents.Absent.AccumulateINT()==nil)
        assert(GameEvents.Mixed.Call(5000000001,'a\0b')==false)
        assert(GameEvents.Mixed.TestAny(5000000001,'a\0b')==true)
        assert(GameEvents.Integer.AccumulateINT()==5000000002)
        assert(GameEvents.Real.Accumulate()==3.75)
        math.event_trace={}
        GameEvents.Contexts.Add(function() math.event_trace[#math.event_trace+1]='A'; return false end)
        late=0
        GameEvents.Mutate.Add(function() GameEvents.Mutate.Add(function() late=late+1; return 5 end); return 2 end)
        GameEvents.Nested.Add(function() return GameEvents.Real.Accumulate() end)
    )lua","=script-event-methods.lua");
    host.execute("B","GameEvents.Contexts.Add(function() math.event_trace[#math.event_trace+1]='B'; return true end)","=other-hook-context.lua");
    const auto context_result=host.call_hook("Contexts");
    host.execute("A",std::string("assert(#math.event_trace==2 and (math.event_trace[2]=='B')==")+(context_result.value==true?"true":"false")+")","=context-call-result.lua");
    check(context_result.errors.empty(),"Call result must come from the last traversed context while every context runs");
    check(host.test_all("Contexts").value==false&&host.test_any("Contexts").value==true,"TestAll/TestAny aggregate all contexts rather than resetting at each one");
    check(host.accumulate_int("Mutate").value==2&&host.accumulate_int("Mutate").value==7,"Numeric dispatch snapshots count but reads the live handler list");
    check(host.accumulate("Nested").value==3.75,"Nested Lua event dispatch must preserve the outer numeric result");
    host.complete_loading();check(host.loading_complete(),"Successfully loaded contexts remain usable after reported dispatch errors");
    LuaScriptHost bounded({0,10000});
    bounded.execute("spin","GameEvents.Spin.Add(function() while true do end end)","=spin-event.lua");
    rejects([&]{bounded.accumulate_int("Spin");});
    rejects([&]{bounded.call_hook("Spin");});
}
