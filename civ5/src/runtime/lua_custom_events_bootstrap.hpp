#pragma once
// Retail LuaEventFactory (0x700540/0x6DB200), distinct from GameEvents:
// weak listener groups, synchronous unprotected calls, no return values.
static constexpr char lua_custom_events_bootstrap[]=R"lua(
local invoke = ...
local rawget, rawset, rawequal, setmetatable = rawget, rawset, rawequal, setmetatable
local type, next, select = type, next, select
local groups={}
local factory={__index=function(root,name)
    local group=rawget(groups,name)
    if group==nil then
        group=setmetatable({}, {__mode='v'})
        rawset(groups,name,group)
    end
    local list={}
    group[#group+1]=list
    local event={}
    event.Add=function(...)
        local n,value=select('#',...),nil
        if n>0 then value=select(n,...) end
        list[#list+1]=value
        return value
    end
    event.Count=function() return #list end
    event.Remove=function(...)
        local argc,value=select('#',...),nil
        if argc>0 then value=select(argc,...) end
        local n,i,current=#list,0,nil
        repeat
            if rawequal(current,value) then
                for j=i,n do list[j]=list[j+1] end
            end
            i=i+1
            current=list[i]
        until current==nil
    end
    event.RemoveAll=function()
        for i=1,#list do list[i]=nil end
    end
    local function call(...)
        for _,listeners in next,group do
            if type(listeners)=='table' then
                local n=#listeners
                for i=1,n do invoke(listeners[i],...) end
            end
        end
    end
    event.Call=call
    -- __call captures the original Call closure; assigning event.Call later
    -- does not change it. The actual metatable is hidden behind an empty one.
    setmetatable(event,{__metatable={},__call=function(_,...) call(...) end})
    rawset(root,name,event)
    return event
end}
return function(env)
    env.LuaEventFactory=factory
    env.LuaEvents=setmetatable({},factory)
end
)lua";
