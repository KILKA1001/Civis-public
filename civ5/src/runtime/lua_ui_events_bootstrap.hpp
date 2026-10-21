#pragma once
// Civis UI event bridge. The active UI supplies its own capability names;
// this is deliberately separate from GameEvents and LuaEvents. Dispatch order
// across independent Lua contexts is not claimed equivalent to retail EXE.
static constexpr char lua_ui_events_bootstrap[]=R"lua(
local invoke, command, notifications, commands, end_turn_event, created_event, unit_damage_event = ...
local pairs, type, rawset, rawequal, select, setmetatable = pairs, type, rawset, rawequal, select, setmetatable
local groups = {}
local function dispatch(name, ...)
    local members = groups[name]
    local count, errors = 0, {}
    if members then
        for _, list in pairs(members) do
            local n = #list
            for i = 1, n do
                count = count + 1
                local ok, value = invoke(list[i], ...)
                if not ok then
                    errors[#errors + 1] = type(value)=='string' and value or '(non-string UI callback error)'
                end
            end
        end
    end
    return count, errors
end
local function configure(env, context)
    local root = {}
    setmetatable(root, {__index=function(_, name)
        if type(name)~='string' then return nil end
        if commands[name] then
            local value = function(...) return command(name, ...) end
            rawset(root, name, value)
            return value
        end
        if not notifications[name] then return nil end
        if name=="SerialEventEndTurnDirty" then
            local event=end_turn_event(context); rawset(root,name,event); return event
        end
        if name=="SerialEventUnitCreated" and created_event then
            local event=created_event(context)
            if event then rawset(root,name,event);return event end
        end
        if name=="SerialEventUnitSetDamage" and unit_damage_event then
            local event=unit_damage_event(context)
            if event then rawset(root,name,event);return event end
        end
        local members = groups[name]
        if not members then members={}; groups[name]=members end
        local list={}
        members[context]=list
        local event={}
        event.Add=function(...)
            local n,value=select('#',...),nil
            if n>0 then value=select(n,...) end
            list[#list+1]=value
            return value
        end
        event.Count=function() return #list end
        event.Remove=function(...)
            local n,value=select('#',...),nil
            if n>0 then value=select(n,...) end
            local length,i,current=#list,0,nil
            repeat
                if rawequal(current,value) then
                    for j=i,length do list[j]=list[j+1] end
                end
                i=i+1;current=list[i]
            until current==nil
        end
        event.RemoveAll=function()
            for i=1,#list do list[i]=nil end
        end
        rawset(root,name,event)
        return event
    end})
    env.Events=root
end
return {configure=configure,dispatch=dispatch}
)lua";
