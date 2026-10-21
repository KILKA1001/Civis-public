#pragma once
static constexpr char lua_modding_bootstrap[]=R"lua(
local query, evaluated, open_save_data, open_user_data, delete_user_data = ...
local pairs, rawset, setmetatable, getmetatable = pairs, rawset, setmetatable, getmetatable
return function(env,context)
    env.Modding = {
        GetEvaluatedFilePath = evaluated,
        OpenSaveData = open_save_data and function() return open_save_data(context) end,
        OpenUserData = open_user_data and function(name,version) return open_user_data(context,name,version) end,
        DeleteUserData = delete_user_data and function(name,version) return delete_user_data(context,name,version) end,
        GetActivatedModEntryPoints = query and function(kind)
            local rows = query(kind)
            local index = 0
            return function()
                index = index + 1
                -- Retail iterator resets its SQL cursor at end, so a later
                -- manual call starts again. Each result is a fresh row table.
                if index > #rows then index = 0; return nil end
                local row = {}
                for name,value in pairs(rows[index]) do rawset(row,name,value) end
                local meta = {}
                for name,value in pairs(getmetatable(rows[index])) do meta[name]=value end
                return setmetatable(row, meta)
            end
        end
    }
end
)lua";
