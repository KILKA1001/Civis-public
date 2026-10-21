#pragma once
void lua_custom_events_tests(){
    using namespace civ5::runtime;
    LuaScriptHost host;
    host.execute("provider",R"lua(
        context_marker='provider'
        assert(type(LuaEvents)=='table' and getmetatable(LuaEvents)==LuaEventFactory)
        assert(rawget(LuaEvents,'Exchange')==nil)
        local event=LuaEvents.Exchange
        assert(rawget(LuaEvents,'Exchange')==event and event.Count()==0)
        assert(type(getmetatable(event))=='table' and next(getmetatable(event))==nil)
        assert(not pcall(setmetatable,event,{}))
        assert(event.TestAll==nil and event.CallImmediate==nil)
        event.note='writable'
        assert(event.note=='writable')
        provider_value={id=5000000001}
        event.Add(function(value,output,...)
            assert(select('#',...)==2 and select(1,...)==nil and select(2,...)==false)
            assert(value.id==5000000001 and context_marker=='provider')
            value.changed=5000000002
            output.same=value;output.provider=provider_value
            return false,99 -- All return values are discarded.
        end)
        assert(event.Count()==1)
        LuaEvents.SameThread.Add(function(thread) assert(coroutine.running()==thread) end)
        GameEvents.Exchange.Add(function() error('wrong event namespace') end)
        assert(select('#',LuaEvents.NoHandlers.Call())==0 and select('#',LuaEvents.NoHandlers())==0)
    )lua","=lua-events-provider");
    host.execute("consumer",R"lua(
        context_marker='consumer'
        local e=LuaEvents.Exchange
        LuaEvents.SameThread(coroutine.running())
        assert(e.Count()==0) -- Count and removals are local to this event object.
        local value,output={id=5000000001},{}
        assert(select('#',e.Call(value,output,nil,false))==0)
        assert(value.changed==5000000002 and output.same==value and output.provider.id==5000000001)
        assert(context_marker=='consumer' and provider_value==nil)
        output.provider.id=5000000003
        e.Call=function() error('replacement field') end
        assert(select('#',e(value,output,nil,false))==0 and output.provider.id==5000000003)
        e.RemoveAll() -- Does not remove provider handlers.
        e(value,output,nil,false)
        LuaEvents[5000000001].Add(function(t) t.wide=true end)
        local t={};LuaEvents[5000000001](t);assert(t.wide)
        local key={};LuaEvents[key].Add(function(t) t.identity=key end)
        LuaEvents[key](t);assert(t.identity==key)
    )lua","=lua-events-consumer");
    host.execute("provider","assert(provider_value.id==5000000003 and LuaEvents.Exchange.Count()==1)","=shared-reference");
    host.complete_loading();check(host.loading_complete(),"LuaEvents must synchronously preserve shared references, nil arguments, function environments and wide keys across contexts");
    host.execute("errors",R"lua(
        local event,log,token=LuaEvents.Fail,{},{}
        event.Add(function() log[#log+1]=1;error(token) end)
        event.Add(function() log[#log+1]=2 end)
        local ok,err=pcall(event.Call)
        assert(not ok and err==token and #log==1 and log[1]==1)
        assert(event.Count()==2)
        event.RemoveAll();assert(event.Count()==0)
        local callable=setmetatable({}, {__call=function(_,n) log[#log+1]=n end})
        assert(event.Add('ignored',callable)==callable)
        event(7);assert(log[2]==7)
        event.RemoveAll()
        local function duplicate() end
        event.Add(duplicate);event.Add(duplicate);event.Add(duplicate)
        event.Remove(duplicate);assert(event.Count()==1) -- Adjacent duplicate skip, same as GameEvents.
        event.RemoveAll()
        event.Add(function() coroutine.yield('forbidden') end)
        local thread=coroutine.create(function() event() end)
        assert(not coroutine.resume(thread) and coroutine.status(thread)=='dead')
        event.RemoveAll()
        event.Add(12)
        assert(not pcall(event.Call)) -- Add itself does not require a function.
        event.RemoveAll()
    )lua","=lua-events-errors");
    host.complete_loading();check(host.loading_complete(),"LuaEvents errors must propagate unchanged and abort remaining listeners; calls must not yield across the native boundary");
    host.execute("mutation",R"lua(
        local e,log=LuaEvents.Mutation,{}
        local function late() log[#log+1]='late' end
        e.Add(function() log[#log+1]='first';e.Add(late) end)
        e();assert(table.concat(log,',')=='first')
        log={};e();assert(table.concat(log,',')=='first,late')
        e.RemoveAll();log={}
        e.Add(function() log[1]='first';e.RemoveAll() end)
        e.Add(late)
        assert(not pcall(e.Call) and log[1]=='first' and log[2]==nil) -- Live nil callback in snapshotted range.
        local old=LuaEvents.Replace
        local called=0;old.Add(function() called=called+1 end)
        LuaEvents.Replace=nil
        local replacement=LuaEvents.Replace
        assert(old~=replacement and replacement.Count()==0)
        replacement();assert(called==1)
        old=nil
        collectgarbage('collect');collectgarbage('collect')
        replacement();assert(called==1) -- Weak groups cannot keep discarded event listeners alive.
    )lua","=lua-events-mutation");
    host.complete_loading();check(host.loading_complete(),"LuaEvents must preserve callback-list mutation rules and weak ownership when event objects are replaced");
    host.execute("bridge",R"lua(
        LuaEvents.Bridge.Add(function(t) t.value=t.value+5000000001 end)
        GameEvents.Bridge.Add(function(v)
            local t={value=v};LuaEvents.Bridge(t);return t.value
        end)
        LuaEvents.BadBridge.Add(function() error('nested Lua event error') end)
        GameEvents.BadBridge.Add(function() LuaEvents.BadBridge() end)
    )lua","=lua-events-gameplay-bridge");
    const std::array<ScriptValue,1> args{std::int64_t{5000000002LL}};
    const auto result=host.accumulate_int("Bridge",args);
    check(result.value==10000000003LL&&result.errors.empty(),"Gameplay callbacks must exchange data through LuaEvents without narrowing or switching namespaces");
    const auto error=host.call_hook("BadBridge");
    check(error.errors.size()==1&&error.errors.front().find("nested Lua event error")!=std::string::npos,"Outer protected gameplay dispatch must receive a nested LuaEvents error");
}
