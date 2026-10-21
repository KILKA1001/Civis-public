// Actual native master extensions from5CE740, inside canonical Host::Impl.
// Ordinary Lua calls remain inside Host::call's protected interpreter boundary.
// Only the finite original argument subset proven below is accepted.
static int native_master_table_count(lua_State* L){
    luaL_checktype(L,1,LUA_TTABLE);std::uint32_t count=0;
    lua_pushnil(L);
    while(lua_next(L,1)){++count;lua_pop(L,1);}
    // Original INC EDI wraps32, then signed lua_pushinteger argument EDI.
    const auto signed_count=count<=0x7fffffffU?static_cast<std::int64_t>(count):static_cast<std::int64_t>(count)-4294967296LL;
    lua_pushinteger(L,static_cast<lua_Integer>(signed_count));return 1;
}
static std::int32_t native_master_checked_integer32(lua_State* L,int slot){
    // Before bundled Lua51 double->lua_Integer64 conversion, undefined outside range.
    const double number=luaL_checknumber(L,slot);
    if(!std::isfinite(number)||std::trunc(number)!=number||number<static_cast<double>(std::numeric_limits<std::int32_t>::min())||number>=2147483648.0)
        luaL_error(L,"Civis native master numeric integer32 admission required unsupported branch");
    const auto value=luaL_checkinteger(L,slot);
    if(value<std::numeric_limits<std::int32_t>::min()||value>std::numeric_limits<std::int32_t>::max())
        luaL_error(L,"Civis native master integer32 conversion required unsupported branch");
    return static_cast<std::int32_t>(value);
}
static int native_master_table_fill(lua_State* L){
    const int argc=lua_gettop(L);int target=0,value=0;std::int32_t count=0;
    if(argc==2){
        count=native_master_checked_integer32(L,2);
        // Before any array-size conversion, n+1 arithmetic or table allocation.
        if(count<0||count==std::numeric_limits<std::int32_t>::max())
            return luaL_error(L,"Civis native table.fill allocation size required unsupported branch");
        // Genuine positive/zero native lua_createtable(count,0) subset only.
        lua_createtable(L,count,0);target=lua_gettop(L);value=1;
    }else if(argc==3){
        count=native_master_checked_integer32(L,3);target=1;value=2;
        // Native rawseti assumes table; invalid non-table is not a successful branch.
        if(!lua_istable(L,target))return luaL_error(L,"Civis native table.fill target type required unsupported branch");
    }else{
        return luaL_error(L,"Invalid arguments.");
    }
    if(count==std::numeric_limits<std::int32_t>::max())
        return luaL_error(L,"Civis native table.fill signed loop overflow required unsupported branch");
    for(std::int32_t i=1;i<=count;++i){lua_pushvalue(L,value);lua_rawseti(L,target,i);}
    lua_pushvalue(L,target);return 1;
}
static int native_master_where(lua_State* L){
    // Native default0 is the actual none/nil optinteger branch.
    // Numeric input must pass finite32 admission BEFORE optinteger conversion.
    if(!lua_isnoneornil(L,1))(void)native_master_checked_integer32(L,1);
    const auto level=luaL_optinteger(L,1,0);
    if(level<std::numeric_limits<std::int32_t>::min()||level>std::numeric_limits<std::int32_t>::max())
        return luaL_error(L,"Civis native where level32 conversion required unsupported branch");
    luaL_where(L,static_cast<int>(level));return 1;
}
static int native_master_math_clamp(lua_State* L){
    const double value=luaL_checknumber(L,1),lower=luaL_checknumber(L,2),upper=luaL_checknumber(L,3);
    if(!std::isfinite(value)||!std::isfinite(lower)||!std::isfinite(upper))
        return luaL_error(L,"Civis native clamp nonfinite x87 branch required unsupported proof");
    // Native7A9710/24 JA; do not use std::clamp's reversed-bound precondition.
    const double first=lower>value?lower:value;
    const double result=first>upper?upper:first;
    lua_pushnumber(L,result);return 1;
}
// Native588A80 exports the actual created iterator as its one captured upvalue.
static int native_master_ipairs_iterator(lua_State* L){
    luaL_checktype(L,1,LUA_TTABLE);
    const auto previous=native_master_checked_integer32(L,2);
    const auto bits=static_cast<std::uint32_t>(previous)+std::uint32_t{1};
    const auto signed_next=bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-4294967296LL;
    const auto next=static_cast<std::int32_t>(signed_next);
    lua_pushinteger(L,static_cast<lua_Integer>(next));lua_rawgeti(L,1,next);
    return lua_isnil(L,-1)?0:2;
}
static int native_master_ipairs(lua_State* L){
    luaL_checktype(L,1,LUA_TTABLE);
    lua_pushvalue(L,lua_upvalueindex(1));lua_pushvalue(L,1);lua_rawgeti(L,1,0);
    const int initial=lua_isnil(L,-1)?0:-1;lua_pop(L,1);lua_pushinteger(L,initial);return 3;
}
static void install_native_master_extensions(lua_State* L){
    // Genuine actual namespace/table lookup and direct closure stores, not DTO.
    lua_getglobal(L,"math");
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Native master math namespace absent");
    lua_pushcfunction(L,native_master_math_clamp);lua_setfield(L,-2,"clamp");lua_pop(L,1);
    lua_getglobal(L,"table");
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Native master table namespace absent");
    lua_pushcfunction(L,native_master_table_count);lua_setfield(L,-2,"count");
    lua_pushcfunction(L,native_master_table_fill);lua_setfield(L,-2,"fill");lua_pop(L,1);
    install_native_master_print(L);
    lua_pushcfunction(L,native_master_ipairs_iterator);lua_pushcclosure(L,native_master_ipairs,1);lua_setglobal(L,"ipairs");
    lua_pushcfunction(L,native_master_where);lua_setglobal(L,"where");
}
