#pragma once
static constexpr char lua_map_bootstrap[]=R"lua(
local grid, count, wrapx, wrapy, getplot, byindex, direction, distance, reference, x, y, index, water, mountain, impassable, index_alias, resource, calculated, revealed, active_team, active_player, preserve_native_gamecore = ...
local setmetatable = setmetatable
return function(env)
    local instances=setmetatable({},{__mode='v'})
    local methods={GetX=x,GetY=y,GetPlotIndex=index,IsWater=water,IsMountain=mountain,IsImpassable=impassable,__instances=instances}
    if index_alias then methods.GetIndex=index end
    if resource then methods.GetResourceType=resource end
    if calculated then methods.CalculateYield=calculated end
    if revealed then methods.IsRevealed=revealed end
    if not preserve_native_gamecore or not preserve_native_gamecore(env,"Game") then
    if active_team then env.Game=env.Game or {};env.Game.GetActiveTeam=active_team end
    if active_player then env.Game=env.Game or {};env.Game.GetActivePlayer=active_player end
    end
    local function plot(id)
        if id==nil then return nil end
        local instance=instances[id]
        if instance==nil then
            instance=setmetatable({__instance=reference(id)},{__index=methods})
            instances[id]=instance
        end
        return instance
    end
    if not preserve_native_gamecore or not preserve_native_gamecore(env,"Plot") then env.Plot=methods end
    if not preserve_native_gamecore or not preserve_native_gamecore(env,"Map") then
    env.Map={GetGridSize=grid,GetNumPlots=count,IsWrapX=wrapx,IsWrapY=wrapy,PlotDistance=distance,
        GetPlot=function(...) local id=getplot(...);if id~=nil then return plot(id) end end,
        GetPlotByIndex=function(...) local id=byindex(...);if id~=nil then return plot(id) end end,
        PlotDirection=function(...) return plot(direction(...)) end}
    end
    if not preserve_native_gamecore or not preserve_native_gamecore(env,"DirectionTypes") then
    env.DirectionTypes={NO_DIRECTION=-1,DIRECTION_NORTHEAST=0,DIRECTION_EAST=1,DIRECTION_SOUTHEAST=2,
        DIRECTION_SOUTHWEST=3,DIRECTION_WEST=4,DIRECTION_NORTHWEST=5,NUM_DIRECTION_TYPES=6}
    end
    if not preserve_native_gamecore or not preserve_native_gamecore(env,"PlotTypes") then
    env.PlotTypes={NO_PLOT=-1,PLOT_MOUNTAIN=0,PLOT_HILLS=1,PLOT_LAND=2,PLOT_OCEAN=3,NUM_PLOT_TYPES=4}
    end
end
)lua";
