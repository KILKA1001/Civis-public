#pragma once
static constexpr char lua_map_setup_bootstrap[]=R"lua(
local grid, random, world_size, custom_option, world_rows, fractal_wrap,
      fractal_create, fractal_height, fractal_ridges, plot_count,
      plot_by_index, plot_value, plot_set_type, selected_define, adjacent_land, plot_set_terrain,
      adjacent_shallow, terrain_info_type, coastal_land, find_water,
      plot_area_id, area_value, inland_corner,
      plot_direction, river_edge, river_write, plot_set_area, calculate_areas,
      feature_info_type, find_biggest_area, can_have_feature,
      selected_climate, climate_rows, set_feature_type, fractal_rifts, recalculate_areas, feature_rows = ...
return function(env)
    if terrain_info_type then
        env.GameInfoTypes=setmetatable({}, {
            __index=function(types,key)
                if type(key)~='string' then
                    error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:GameInfoTypes key', 0)
                end
                return terrain_info_type(key)
            end,
            __newindex=function() error('BNW map GameInfoTypes is read-only', 0) end
        })
    end
    env.TerrainTypes=setmetatable({}, {
        __index=function(_,key)
            if key=='NO_TERRAIN' then return -1 end
            if terrain_info_type and type(key)=='string' and
               string.sub(key,1,8)=='TERRAIN_' then
                return terrain_info_type(key)
            end
            error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:TerrainTypes.' .. tostring(key), 0)
        end,
        __newindex=function() error('BNW map TerrainTypes is read-only', 0) end
    })
    env.FeatureTypes=setmetatable({}, {
        __index=function(_,key)
            if key=='NO_FEATURE' then return -1 end
            if feature_info_type and type(key)=='string' and
               string.sub(key,1,8)=='FEATURE_' then
                return feature_info_type(key)
            end
            error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:FeatureTypes.' .. tostring(key), 0)
        end,
        __newindex=function() error('BNW map FeatureTypes is read-only', 0) end
    })
    local standard_math=math
    env.math=setmetatable({clamp=function(value,low,high)
        if value<low then return low end
        if value>high then return high end
        return value
    end}, {__index=standard_math})
    if selected_define then
        env.GameDefines=setmetatable({}, {__index=function(defines,key)
            local kind=type(key)
            local name=(kind=='string' or kind=='number') and tostring(key) or nil
            if name==nil then return nil end
            local value=selected_define(name)
            if value~=nil then rawset(defines,key,value) end
            return value
        end})
    end
    env.Map={GetGridSize=grid}
    if climate_rows then
        env.Map.GetClimate=function()
            if selected_climate==nil then
                error('CIVIS_BNW_MAP_CLIMATE_UNRESOLVED:pregame climate choice', 0)
            end
            return selected_climate
        end
    end
    if plot_count then env.Map.GetNumPlots=plot_count end
    if plot_by_index and plot_value then
        local handles=setmetatable({}, {__mode='k'})
        local instances=setmetatable({}, {__mode='v'})
        local methods={}
        local function identity(self)
            local id=handles[self]
            if id==nil then error('Invalid BNW map setup plot handle', 2) end
            return id
        end
        methods.GetX=function(self) return plot_value(identity(self), 0) end
        methods.GetY=function(self) return plot_value(identity(self), 1) end
        methods.GetPlotIndex=function(self) return plot_value(identity(self), 2) end
        methods.GetPlotType=function(self) return plot_value(identity(self), 3) end
        methods.GetTerrainType=function(self) return plot_value(identity(self), 4) end
        methods.GetFeatureType=function(self) return plot_value(identity(self), 6) end
        if set_feature_type then
            methods.SetFeatureType=function(self,feature,variety)
                if type(feature)~='number' or feature%1~=0 or
                   (variety~=nil and (type(variety)~='number' or variety%1~=0)) then
                    error('CIVIS_BNW_FEATURE_WRITE_INVALID:Plot.SetFeatureType arguments', 0)
                end
                return set_feature_type(identity(self),feature,variety or -1)
            end
        end
        methods.IsWater=function(self) return self:GetPlotType()==env.PlotTypes.PLOT_OCEAN end
        methods.IsHills=function(self) return self:GetPlotType()==env.PlotTypes.PLOT_HILLS end
        methods.IsMountain=function(self) return self:GetPlotType()==env.PlotTypes.PLOT_MOUNTAIN end
        methods.IsRiver=function(self) return plot_value(identity(self), 5)~=0 end
        methods.IsLake=function(self) return plot_value(identity(self), 7)~=0 end
        if can_have_feature then
            methods.CanHaveFeature=function(self,feature)
                return can_have_feature(identity(self),feature)
            end
        end
        if plot_area_id and area_value then
            local area_handles=setmetatable({}, {__mode='k'})
            local area_instances=setmetatable({}, {__mode='v'})
            local area_methods={}
            local function area_identity(self)
                local id=area_handles[self]
                if id==nil then error('Invalid BNW map setup area handle', 2) end
                return id
            end
            area_methods.GetNumTiles=function(self)
                return area_value(area_identity(self),0)
            end
            area_methods.GetNumRiverEdges=function(self)
                return area_value(area_identity(self),1)
            end
            local function area_handle(id)
                local object=area_instances[id]
                if object==nil then
                    object=setmetatable({}, {
                        __index=function(_,key)
                            local method=area_methods[key]
                            if method then return method end
                            error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Area.' .. tostring(key),0)
                        end,
                        __newindex=function()
                            error('BNW setup area handle is read-only',0)
                        end,
                        __metatable='BNW setup area handle'
                    })
                    area_handles[object]=id
                    area_instances[id]=object
                end
                return object
            end
            area_methods.GetID=function(self) return area_identity(self) end
            methods.GetArea=function(self) return plot_area_id(identity(self)) end
            env.Map.GetArea=function(id)
                if type(id)~='number' or id%1~=0 then error('Invalid BNW setup area ID',0) end
                -- Validate against the current native pool before issuing a handle.
                area_value(id,0)
                return area_handle(id)
            end
            methods.Area=function(self)
                return area_handle(plot_area_id(identity(self)))
            end
            if find_biggest_area then
                env.Map.FindBiggestArea=function(water)
                    local id=find_biggest_area(not not water)
                    if id==nil then return nil end
                    return area_handle(id)
                end
            end
        end
        if inland_corner then
            methods.GetInlandCorner=function(self)
                local index=inland_corner(identity(self))
                if index==nil then return nil end
                return env.Map.GetPlotByIndex(index)
            end
        end
        if river_edge then
            methods.IsNEOfRiver=function(self) return river_edge(identity(self),0) end
            methods.IsWOfRiver=function(self) return river_edge(identity(self),1) end
            methods.IsNWOfRiver=function(self) return river_edge(identity(self),2) end
        end
        if river_write then
            local function preflight_river_write(self,edge,present,flow)
                if type(present)~='boolean' or type(flow)~='number' or flow%1~=0 then
                    error('CIVIS_BNW_RIVER_WRITE_INVALID:river edge arguments', 0)
                end
                river_write(identity(self),edge,present,flow)
            end
            methods.SetNEOfRiver=function(self,present,flow)
                preflight_river_write(self,0,present,flow)
            end
            methods.SetWOfRiver=function(self,present,flow)
                preflight_river_write(self,1,present,flow)
            end
            methods.SetNWOfRiver=function(self,present,flow)
                preflight_river_write(self,2,present,flow)
            end
        end
        if coastal_land then
            methods.IsCoastalLand=function(self) return coastal_land(identity(self)) end
        end
        if adjacent_land then
            methods.IsAdjacentToLand=function(self) return adjacent_land(identity(self)) end
        end
        if adjacent_shallow then
            methods.IsAdjacentToShallowWater=function(self)
                return adjacent_shallow(identity(self))
            end
        end
        methods.SetPlotType=function(self, new_type, recalculate, rebuild_graphics, erase_units_if_water)
            if not plot_set_type then
                error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetPlotType', 0)
            end
            if new_type==env.PlotTypes.PLOT_OCEAN and recalculate==nil and
               rebuild_graphics==nil and erase_units_if_water==nil then
                -- CvPlot.h defaults all three flags to true. The native
                -- setup callback accepts this only in the AddLakes probe.
                plot_set_type(identity(self), new_type, true, true, true)
                return
            end
            if new_type==env.PlotTypes.PLOT_HILLS and recalculate==false and
               rebuild_graphics==true and erase_units_if_water==nil then
                -- Stock FeatureGenerator removes coastal mountains before
                -- placing features. The native callback checks its phase.
                plot_set_type(identity(self), new_type, false, true, true)
                return
            end
            if type(new_type)~='number' or new_type%1~=0 or
               type(recalculate)~='boolean' or type(rebuild_graphics)~='boolean' or
               recalculate or rebuild_graphics or erase_units_if_water==false or
               (erase_units_if_water~=nil and erase_units_if_water~=true) then
                error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetPlotType:outside fresh stock contract', 0)
            end
            plot_set_type(identity(self), new_type, recalculate, rebuild_graphics, true)
        end
        if plot_set_area then
            methods.SetArea=function(self, new_area)
                if type(new_area)~='number' or new_area%1~=0 or new_area~=-1 then
                    error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetArea:outside fresh AddLakes contract', 0)
                end
                plot_set_area(identity(self), new_area)
            end
        end
        methods.SetTerrainType=function(self, terrain, recalculate, rebuild_graphics)
            if not plot_set_terrain then
                error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetTerrainType', 0)
            end
            if type(terrain)~='number' or terrain%1~=0 or
               recalculate~=false or type(rebuild_graphics)~='boolean' then
                error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.SetTerrainType:outside fresh stock contract', 0)
            end
            plot_set_terrain(identity(self), terrain, recalculate, rebuild_graphics)
        end
        env.Plot=methods
        env.Map.GetPlotByIndex=function(index)
            local id=plot_by_index(index)
            if id==nil then return end
            local object=instances[id]
            if object==nil then
                object=setmetatable({}, {
                    __index=function(_, key)
                        local method=methods[key]
                        if method then return method end
                        error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Plot.' .. tostring(key), 0)
                    end,
                    __newindex=function() error('BNW setup plot handle is read-only', 0) end,
                    __metatable='BNW setup plot handle'
                })
                handles[object]=id
                instances[id]=object
            end
            return object
        end
        env.Map.GetPlot=function(x,y)
            local width,height=grid()
            if type(x)~='number' or type(y)~='number' or
               x%1~=0 or y%1~=0 or x<0 or y<0 or x>=width or y>=height then
                error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Map.GetPlot:outside verified grid', 0)
            end
            return env.Map.GetPlotByIndex(y*width+x)
        end
        if plot_direction then
            env.Map.PlotDirection=function(x,y,direction)
                local index=plot_direction(x,y,direction)
                if index==nil then return nil end
                return env.Map.GetPlotByIndex(index)
            end
        end
        if find_water then
            env.Map.FindWater=function(plot,range,fresh_water)
                return find_water(identity(plot),range,fresh_water)
            end
        end
    end
    if random then env.Map.Rand=random end
    if calculate_areas then env.Map.CalculateAreas=calculate_areas end
    if recalculate_areas then env.Map.RecalculateAreas=recalculate_areas end
    if world_size then env.Map.GetWorldSize=world_size end
    if custom_option then env.Map.GetCustomOption=custom_option end
    if fractal_wrap then
        env.Map.IsWrapX=function() local x=fractal_wrap();return x end
        env.Map.IsWrapY=function() local _,y=fractal_wrap();return y end
        env.Map.GetFractalFlags=function()
            local wrap_x, wrap_y = fractal_wrap()
            local flags={}
            if wrap_x then flags.FRAC_WRAP_X=true end
            if wrap_y then flags.FRAC_WRAP_Y=true end
            return flags
        end
    end
    if fractal_create and fractal_height then
        local handles=setmetatable({}, {__mode='k'})
        local methods={}
        methods.GetHeight=function(self, first, second)
            local id=handles[self]
            if not id then error('Invalid BNW fractal object', 2) end
            return fractal_height(id, first, second)
        end
        local function flag_bits(flags)
            if type(flags)~='table' then error('Fractal flags must be a table', 2) end
            local bits=0
            for _,entry in ipairs({{'FRAC_WRAP_X',1},{'FRAC_WRAP_Y',2},
                                    {'FRAC_PERCENT',4},{'FRAC_POLAR',8},
                                    {'FRAC_CENTER_RIFT',16},{'FRAC_INVERT_HEIGHTS',32}}) do
                -- CvLuaFractal::HasFlag uses lua_getfield: false is present,
                -- and a flag table's __index is observable.
                if flags[entry[1]]~=nil then bits=bits+entry[2] end
            end
            return bits
        end
        methods.BuildRidges=function(self, seeds, flags, ridge_blend, fractal_blend)
            local id=handles[self]
            if not id then error('Invalid BNW fractal object', 2) end
            if not fractal_ridges then
                error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Fractal.BuildRidges', 0)
            end
            return fractal_ridges(id, seeds, flag_bits(flags), ridge_blend,
                                  fractal_blend)
        end
        env.Fractal={}
        env.Fractal.Create=function(width,height,grain,flags,x_exponent,y_exponent)
            local id=fractal_create(width,height,grain,flag_bits(flags),
                                    x_exponent or -1,y_exponent or -1)
            local object=setmetatable({}, {__index=methods})
            handles[object]=id
            return object
        end
        env.Fractal.CreateRifts=function(width,height,grain,flags,rifts,x_exponent,y_exponent)
            if not fractal_rifts then
                error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:Fractal.CreateRifts', 0)
            end
            local rift_id=handles[rifts]
            if not rift_id then error('Invalid BNW rift fractal object', 2) end
            local id=fractal_rifts(width,height,grain,flag_bits(flags),rift_id,
                                   x_exponent or -1,y_exponent or -1)
            local object=setmetatable({}, {__index=methods})
            handles[object]=id
            return object
        end
    end
    if world_rows then
        local by_rowid, by_type, ordered = {}, {}, {}
        local function readonly(fields, label)
            return setmetatable({}, {
                __index=function(_, key)
                    local value=fields[key]
                    if value ~= nil then return value end
                    error('CIVIS_UNSUPPORTED_BNW_MAP_HELPER:' .. label .. '.' .. tostring(key), 0)
                end,
                __newindex=function() error('BNW map ' .. label .. ' is read-only', 0) end,
                __metatable='read-only'
            })
        end
        for i=1,#world_rows do
            local data=world_rows[i]
            local row=readonly({ID=data.ID,Type=data.Type,
                                FeatureGrainChange=data.FeatureGrainChange}, 'GameInfo.Worlds row')
            by_rowid[data.ROWID]=row
            by_type[data.Type]=row
            ordered[i]=row
        end
        local worlds=setmetatable({}, {
            __index=function(_, key)
                if type(key)=='number' then return by_rowid[key] end
                if type(key)=='string' then return by_type[key] end
                return nil
            end,
            __call=function()
                local i=0
                return function() i=i+1; return ordered[i] end
            end,
            __newindex=function() error('BNW map GameInfo.Worlds is read-only', 0) end,
            __metatable='read-only'
        })
        local climates
        if climate_rows then
            local climate_by_rowid, climate_by_type, climate_ordered = {}, {}, {}
            for i=1,#climate_rows do
                local data=climate_rows[i]
                local row=readonly({ID=data.ID,Type=data.Type},
                                   'GameInfo.Climates row')
                climate_by_rowid[data.ROWID]=row
                climate_by_type[data.Type]=row
                climate_ordered[i]=row
            end
            climates=setmetatable({}, {
                __index=function(_, key)
                    if type(key)=='number' then return climate_by_rowid[key] end
                    if type(key)=='string' then return climate_by_type[key] end
                    return nil
                end,
                __call=function()
                    local i=0
                    return function() i=i+1; return climate_ordered[i] end
                end,
                __newindex=function() error('BNW map GameInfo.Climates is read-only',0) end,
                __metatable='read-only'
            })
        end
        local features
        if feature_rows then
            local by_id,by_type,ordered_features={},{},{}
            for i=1,#feature_rows do
                local data=feature_rows[i]
                local row=readonly({ID=data.ID,Type=data.Type},'GameInfo.Features row')
                by_id[data.ROWID]=row;by_type[data.Type]=row;ordered_features[i]=row
            end
            features=setmetatable({}, {
                __index=function(_,key)
                    if type(key)=='number' then return by_id[key] end
                    if type(key)=='string' then return by_type[key] end
                    return nil
                end,
                __call=function()
                    local i=0;return function() i=i+1;return ordered_features[i] end
                end,
                __newindex=function() error('BNW map GameInfo.Features is read-only',0) end,
                __metatable='read-only'
            })
        end
        env.GameInfo=readonly({Worlds=worlds,Climates=climates,Features=features}, 'GameInfo')
    end
    env.PlotTypes={NO_PLOT=-1,PLOT_MOUNTAIN=0,PLOT_HILLS=1,PLOT_LAND=2,PLOT_OCEAN=3,NUM_PLOT_TYPES=4}
    env.DirectionTypes={
        NO_DIRECTION=-1,
        DIRECTION_NORTHEAST=0, DIRECTION_EAST=1, DIRECTION_SOUTHEAST=2,
        DIRECTION_SOUTHWEST=3, DIRECTION_WEST=4, DIRECTION_NORTHWEST=5,
        NUM_DIRECTION_TYPES=6
    }
    -- CvLuaEnums registers CvEnums.h FlowDirectionTypes in the gameplay Lua
    -- state before stock MapGenerator.lua is included. Its ordering differs
    -- from DirectionTypes and its masks use a separate bit order.
    env.FlowDirectionTypes={
        NO_FLOWDIRECTION=-1,
        FLOWDIRECTION_NORTH=0, FLOWDIRECTION_NORTHEAST=1,
        FLOWDIRECTION_SOUTHEAST=2, FLOWDIRECTION_SOUTH=3,
        FLOWDIRECTION_SOUTHWEST=4, FLOWDIRECTION_NORTHWEST=5,
        NUM_FLOWDIRECTION_TYPES=6,
        FLOWDIRECTION_NORTH_MASK=1, FLOWDIRECTION_SOUTH_MASK=2,
        FLOWDIRECTION_SOUTHEAST_MASK=4, FLOWDIRECTION_NORTHWEST_MASK=8,
        FLOWDIRECTION_SOUTHWEST_MASK=16, FLOWDIRECTION_NORTHEAST_MASK=32
    }
end
)lua";
