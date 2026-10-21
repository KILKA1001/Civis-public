#pragma once
// Retail 0x4972D0 matches IncludeFileList with Lua string.find, executes the
// first match by default and returns matched filenames even on chunk failure.
static constexpr char lua_include_bootstrap[]=R"lua(
local files, compile, report, strict_map, journal = ...
local type, error, pcall = type, error, pcall
local rawget, rawset = rawget, rawset
local sub = string.sub
return function(env, context, selected_list)
    local list
    if selected_list~=nil then
        if type(selected_list)~='table' or strict_map then error('Selected actual master include table unsupported') end
        list=selected_list
    else
    env.RefreshIncludeFileList = function()
        local list = env.IncludeFileList
        if list == nil then list = {}; env.IncludeFileList = list end
        for i=1,#list do rawset(list,i,nil) end
        local names = files(context)
        for i=1,#names do rawset(list,i,names[i]) end
    end
    env.RefreshIncludeFileList()
    -- Native include captures this table; replacing the global does not
    -- replace the table captured by an already registered include closure.
    list = env.IncludeFileList
    end
    if strict_map then
        local available, loading = {}, {}
        local function stem(request)
            if type(request) ~= 'string' then error('CIVIS_BNW_INCLUDE_INVALID:name', 0) end
            local name = request
            if sub(name, -4) == '.lua' then name = sub(name, 1, -5) end
            if name == '' or string.find(name, '[^%w_%-]') then
                error('CIVIS_BNW_INCLUDE_INVALID:' .. request, 0)
            end
            return string.lower(name)
        end
        for i = 1, #list do
            local path = rawget(list, i)
            local name = stem(path)
            if available[name] then error('CIVIS_BNW_INCLUDE_AMBIGUOUS:' .. path, 0) end
            available[name] = path
        end
        env.include = function(request, all)
            if all then error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:include(all)', 0) end
            local name = stem(request)
            local path = available[name]
            if path == nil then error('CIVIS_BNW_INCLUDE_MISSING:' .. request, 0) end
            if loading[name] then error('CIVIS_BNW_INCLUDE_CYCLE:' .. path, 0) end
            loading[name] = true
            local action=journal(0,1,context,path,'',0)
            local chunk, failure, kind = compile(context, path, env)
            if chunk == nil then
                journal(1,action,0,0,true)
                if kind == 'io' then error('CIVIS_BNW_INCLUDE_IO:' .. tostring(failure), 0) end
                if kind == 'bytecode' then error('CIVIS_BNW_INCLUDE_INVALID:platform bytecode', 0) end
                error(failure, 0)
            end
            chunk()
            journal(1,action,0,0,false)
            loading[name] = nil
            return {path}
        end
        return
    end
    env.include = function(pattern, all)
        local find = env.string.find
        if type(find) ~= 'function' then error('string.find must be a valid function.') end
        local result = {}
        local count = #list
        for i=1,count do
            local path = rawget(list,i)
            if type(path) == 'string' and find(path, pattern) ~= nil then
                local action=journal(0,1,context,path,'',0)
                local chunk, failure = compile(context, path, env)
                if chunk then
                    local ok, message = pcall(chunk)
                    if not ok then failure=type(message)=='string' and message or '(non-string Lua error)' end
                end
                journal(1,action,0,0,failure~=nil)
                if failure then report(context, path, failure) end
                result[#result+1] = path
                if not all then break end
            end
        end
        return result
    end
end
)lua";
