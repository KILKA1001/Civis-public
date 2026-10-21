#pragma once
// Database DLL GameInfo: physical tables, ROWID lookup, per-table row caches,
// one-shot iterators, and lazy prepared __index/__len closures.
static constexpr char lua_gameinfo_bootstrap[]=R"lua(
local prepare, object, info_types = ...
local type, next, select, tostring = type, next, select, tostring
local rawget, rawset, setmetatable, error = rawget, rawset, setmetatable, error
local concat, find, sub = table.concat, string.find, string.sub
local function cstring(s)
    local zero=find(s,'\0',1,true)
    return zero and sub(s,1,zero-1) or s
end
local function store_row(q,cache,row)
    local has_id,id=q('id')
    if has_id then rawset(cache,id,row) end
    local has_type,name=q('type')
    if has_type then rawset(cache,name,row) end
    return row
end
-- Contexts share one database identity, while their table/row caches differ.
-- This opaque host-owned value replaces the original connection pointer.
local database=object({})
return function(env,context)
    local function query(sql) return prepare(context,sql) end
    local function table_object(name)
        local meta={__tablename=name,__db=database,__rowdata={}}
        meta.__newindex=function() error('Database table is read-only from Lua.') end
        meta.__index=function(self,key)
            local name,cache=meta.__tablename,meta.__rowdata
            local by_id=query('select * from '..cstring(name)..' where ROWID = ? LIMIT 1')
            if not by_id('valid') then error('Failed to generate id query - invalid database query') end
            local by_type=query('select * from '..cstring(name)..' where Type = ? LIMIT 1')
            local function index(_,key)
                local row=rawget(cache,key)
                if row~=nil then return row end
                local kind=type(key)
                if kind~='number' and kind~='string' then error("Key must be of type 'number' or 'string'.") end
                local q=kind=='number' and by_id or by_type
                if not q('valid') then error('Indexing by type is not available for this table.') end
                q('bind',{key},1)
                if q('step') then row=store_row(q,cache,q('row')) end
                q('reset')
                return row
            end
            meta.__index=index
            return index(self,key)
        end
        meta.__len=function(self)
            local q=query('select count(*) from '..cstring(meta.__tablename))
            local function length()
                if not q('step') then error('Cannot count database table rows.') end
                local row=q('row')
                q('reset')
                return row['count(*)']
            end
            meta.__len=length
            return length(self)
        end
        meta.__call=function(self,...)
            local sql='select * from '..cstring(meta.__tablename)
            local filter=...
            local bindings,count={},0
            if select('#',...)~=0 then
                local kind=type(filter)
                if kind=='string' then sql=sql..' where '..cstring(filter)
                elseif kind=='table' then
                    local conditions={}
                    for key,value in next,filter do
                        count=count+1 -- Includes skipped pairs, matching native bind positions.
                        local value_type=type(value)
                        if type(key)=='string' and (value_type=='number' or value_type=='boolean' or value_type=='string') then
                            conditions[#conditions+1]=cstring(key)..' = ?'
                            bindings[count]=value
                        end
                    end
                    sql=sql..' where '..concat(conditions,' and ')
                else error("Filter must be of type 'string' or 'table'") end
            end
            local q,cache=query(sql),meta.__rowdata
            if q('valid') and type(filter)=='table' then q('bind',bindings,count) end
            return function()
                if not q('step') then q('reset');q('release');return nil end
                local has_id,id=q('id')
                if has_id then local row=rawget(cache,id);if row~=nil then return row end end
                local has_type,name=q('type')
                if has_type then local row=rawget(cache,name);if row~=nil then return row end end
                return store_row(q,cache,q('row'))
            end
        end
        return object(meta)
    end
    local tables=query("SELECT rowid FROM sqlite_master WHERE (name NOT LIKE 'sqlite%') and (type = 'table') and (tbl_name = ?) LIMIT 1;")
    env.GameInfo=setmetatable({}, {__db=database,__index=function(root,key)
        local kind=type(key)
        local name=(kind=='string' or kind=='number') and tostring(key) or nil
        tables('bind',{name},1)
        local value
        if tables('step') then
            value=table_object(cstring(name))
            rawset(root,name,value)
        end
        tables('reset')
        return value
    end})
    env.GameInfoTypes=info_types()
end
)lua";
