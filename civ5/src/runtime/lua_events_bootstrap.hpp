#pragma once
// Event list mechanics recovered from the pinned retail EXE. Cross-context
// Lua table traversal order is not claimed equivalent across architectures.
static constexpr char lua_events_bootstrap[]=R"lua(
local rawset, rawget, rawequal, type, pairs, setmetatable, getmetatable = rawset, rawget, rawequal, type, pairs, setmetatable, getmetatable
local select, error, pcall = select, error, pcall
local invoke, configure_context, configure_modding, configure_gameplay, sum, configure_lua_events, configure_pregame, configure_map, configure_content, configure_ui_events, configure_enums, configure_locale, log, journal = ...
local contexts, groups, errors = {}, {}, {}
local function dispatch(mode, name, ...)
    local action=journal(0,6,'',type(name)=='string' and name or '(non-string event key)',mode,0)
    local errors_before=#errors
    local listeners = groups[name]
    if not listeners then journal(1,action,0,0,false); return nil end
    local had, result = false, mode=='TestAll'
    local observed=0
    if mode=='AccumulateINT' or mode=='Accumulate' then result=0 end
    for _, list in pairs(listeners) do
        local n = #list
        observed=observed+n
        -- Retail Call resets its AND result at every context, including an
        -- empty final context. TestAll preserves it across contexts.
        if mode=='Call' then result=n>0 end
        if n > 0 then had = true end
        for i = 1, n do
            local ok, value = invoke(list[i], ...)
            if ok then
                if mode=='Call' or mode=='TestAll' then
                    result = (not not value) and result
                elseif mode=='TestAny' then
                    result = (not not value) or result
                else
                    result = sum(mode=='AccumulateINT',result,value)
                end
            else
                errors[#errors + 1] = type(value) == 'string' and value or '(non-string Lua error)'
            end
        end
    end
    journal(1,action,observed,0,#errors>errors_before)
    if had then return result end
    return nil
end
-- Native55F2E0 uses actual weak listener-group/list VALUES. Metadata is weak
-- in the list key and never retains a list after its real event disappears.
local native_game_event_lists=setmetatable({}, {__mode='k'})
local function native_game_event_join(context,name,group,list)
    if type(group)~='table' or type(list)~='table' then error('Native GameEvents actual list/group absent') end
    if groups[name]~=nil and not rawequal(groups[name],group) then error('Native GameEvents runtime group rebound') end
    groups[name]=group
    native_game_event_lists[list]={context,name}
end
local function native_game_event_journal(list,phase,op,action,removed)
    local origin=native_game_event_lists[list]
    if not origin then error('Native GameEvents list has no actual runtime owner') end
    if phase==0 then return journal(0,op,origin[1],type(origin[2])=='string' and origin[2] or '(non-string event key)','',#list) end
    journal(1,action,#list,removed,false)
    return action
end
local function create(context, base, prepared, native_print, native_include)
    if native_print~=nil and (type(native_print)~='function' or type(prepared)~='table' or
       not rawequal(rawget(prepared,'print'),native_print)) then
        error('Selected actual native print delivery changed')
    end
    if native_include~=nil and (native_print==nil or type(native_include)~='function' or
       type(prepared)~='table' or not rawequal(rawget(prepared,'include'),native_include)) then
        error('Selected actual include delivery changed')
    end
    if contexts[context] then
        if native_include~=nil and not rawequal(rawget(contexts[context],'include'),native_include) then
            error('Retained selected include rebound')
        end
        if prepared and not rawequal(contexts[context],prepared) then error('Prepared context environment rebound') end
        if native_print~=nil and not rawequal(rawget(contexts[context],'print'),native_print) then
            error('Retained selected native print rebound')
        end
        return contexts[context]
    end
    local env = prepared or {}
    if type(env)~='table' then error('Prepared context environment is not a table') end
    if native_print==nil then
        setmetatable(env, {__index=base})
    elseif getmetatable(env)~=nil then
        error('Selected restricted environment acquired a metatable')
    end
    env._G = env
    if native_print==nil then
    env.print=function(...)
        local values={}
        for i=1,select('#',...) do
            local value=env.tostring(select(i,...))
            if type(value)~='string' then error("'tostring' must return a string to 'print'") end
            values[i]=value
        end
        log(context,table.concat(values,'\t'))
    end
    end
    local events = {}
    setmetatable(events, {__index=function(_, name)
        if type(name) ~= 'string' then error('GameEvents requires an event name') end
        local listeners = groups[name]
        if not listeners then listeners = {}; groups[name] = listeners end
        local list = {}
        listeners[context] = list
        local event = {}
        event.Add = function(...)
            local n, value = select('#', ...), nil
            if n > 0 then value = select(n, ...) end
            local action=journal(0,3,context,name,'',#list)
            list[#list + 1] = value
            journal(1,action,#list,0,false)
            return value
        end
        event.Count = function() return #list end
        event.Remove = function(...)
            local argc, value = select('#', ...), nil
            if argc > 0 then value = select(argc, ...) end
            -- Native removal advances after shifting adjacent duplicates.
            local action=journal(0,4,context,name,'',#list)
            local removed=0
            local n, i, current = #list, 0, nil
            repeat
                if rawequal(current, value) then
                    if i>0 and current~=nil then removed=removed+1 end
                    for j = i, n do list[j] = list[j + 1] end
                end
                i = i + 1
                current = list[i]
            until current == nil
            journal(1,action,#list,removed,false)
        end
        event.RemoveAll = function()
            local before=#list
            local action=journal(0,5,context,name,'',before)
            local removed=0
            for i = 1, #list do if list[i]~=nil then removed=removed+1 end; list[i] = nil end
            journal(1,action,#list,removed,false)
        end
        for _,mode in pairs({'Call','TestAll','TestAny','AccumulateINT','Accumulate'}) do
            event[mode] = function(...) return dispatch(mode,name,...) end
        end
        rawset(events, name, event)
        return event
    end})
    if native_print==nil then env.GameEvents = events end
    configure_lua_events(env)
    if native_include==nil then
        if configure_context then configure_context(env, context) end
    elseif not rawequal(rawget(env,'include'),native_include) then
        error('Selected actual include replaced before setup')
    end
    if configure_modding then configure_modding(env,context) end
    if configure_gameplay then configure_gameplay(env, context) end
    if configure_pregame then configure_pregame(env) end
    if configure_map then configure_map(env) end
    if configure_content then configure_content(env) end
    if configure_ui_events then configure_ui_events(env, context) end
    if configure_enums then configure_enums(env) end
    if configure_locale then configure_locale(env) end
    if native_print~=nil and (getmetatable(env)~=nil or not rawequal(rawget(env,'print'),native_print)) then
        error('Selected native print replaced during context setup')
    end
    if native_include~=nil and not rawequal(rawget(env,'include'),native_include) then
        error('Selected include replaced during context setup')
    end
    contexts[context] = env
    return env
end
-- Private native nested CallHook: isolate errors from a calling outer event.
-- Use the same dispatch/listeners/journal and restore the outer accumulator
-- even when a callback/native source operation fails under this pcall.
local function dispatch_owned_nested(mode,name,...)
    local outer=errors
    errors={}
    local ok,result=pcall(dispatch,mode,name,...)
    local nested=errors
    errors=outer
    if not ok then error(result) end
    return result,nested
end
return {create=create, native_game_event_join=native_game_event_join, native_game_event_journal=native_game_event_journal, dispatch_owned_nested=dispatch_owned_nested, dispatch=function(mode,name, ...)
    errors = {}
    local result = dispatch(mode,name, ...)
    return result, errors
end}
)lua";
