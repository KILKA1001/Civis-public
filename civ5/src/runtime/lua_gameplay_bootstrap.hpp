#pragma once
static constexpr char lua_gameplay_bootstrap[]=R"lua(
local query, objects, has_tech, player_team, configure_info, game_turn, player_human, player_alive, player_leader, game_speed, player_ever_alive, player_minor, player_minor_type, player_civilization, get_unit, unit_ids, set_has_tech, preserve_native_namespace = ...
local pairs, type, setmetatable, rawset = pairs, type, setmetatable, rawset
local tostring = tostring
return function(env,context)
    configure_info(env,context)
    if not preserve_native_namespace or not preserve_native_namespace(env,"DB") then env.DB = {Query=query(context)} end
    if (game_turn or game_speed) and (not preserve_native_namespace or not preserve_native_namespace(env,"Game")) then
        env.Game={GetGameTurn=game_turn,GetGameSpeedType=game_speed}
    end
    local db_query = env.DB.Query
    if not preserve_native_namespace or not preserve_native_namespace(env,"GameDefines") then
    env.GameDefines = setmetatable({}, {__index=function(defines,key)
        local kind=type(key)
        local name=(kind=='string' or kind=='number') and tostring(key) or nil
        local row=db_query('SELECT Value FROM Defines WHERE Name = ? LIMIT 1',name)()
        local value=row and row.Value
        if value~=nil then rawset(defines,key,value) end
        return value
    end})
    end
    local own_team=preserve_native_namespace and preserve_native_namespace(env,"Team")
    local own_player=preserve_native_namespace and preserve_native_namespace(env,"Player")
    if not own_team then env.Team={IsHasTech=has_tech,SetHasTech=set_has_tech} end
    if not own_player then env.Player={GetTeam=player_team,IsHuman=player_human,IsAlive=player_alive,IsEverAlive=player_ever_alive,IsMinorCiv=player_minor,GetMinorCivType=player_minor_type,GetCivilizationType=player_civilization,GetLeaderType=player_leader} end
    if get_unit and not own_player then
        env.Player.GetUnitByID=get_unit
        env.Player.Units=function(player)
            local ids,quote=unit_ids(player);local cursor=0
            local iterator=function(state)
                local ids=unit_ids(state,quote) -- validates SAME native roster/serial before every next
                cursor=cursor+1;local id=ids[cursor]
                if id==nil then return end
                local unit=get_unit(state,id)
                assert(unit~=nil,'Unit roster changed during actual iterator')
                return unit
            end
            -- CvLuaPlayer::lUnits returns generator and the actual player state.
            return iterator,player
        end
    end
    if not preserve_native_namespace or not preserve_native_namespace(env,"Teams") then
        env.Teams=objects(true)
        for _,object in pairs(env.Teams) do setmetatable(object,{__index=env.Team}) end
    end
    if not preserve_native_namespace or not preserve_native_namespace(env,"Players") then
        env.Players=objects(false)
        for _,object in pairs(env.Players) do setmetatable(object,{__index=env.Player}) end
    end
end
)lua";
