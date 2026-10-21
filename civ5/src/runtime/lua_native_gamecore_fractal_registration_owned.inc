// CvLuaFractal.cpp: the two namespace functions have zero upvalues. A returned
// userdata has GetHeight/BuildRidges closures with one retaining userdata
// upvalue; the SDK bodies read the receiver at argument1, not that upvalue.
struct NativeGameCoreFractalPayload {
    static constexpr std::uint64_t tag=0x4349564652414335ull;
    std::uint64_t magic=tag; Impl* issuer=nullptr; lua_State* vm=nullptr;
    std::uint64_t epoch=0; std::int64_t id=0;
    std::function<void(std::int64_t)> release;
};
static NativeGameCoreFractalPayload& native_gamecore_fractal_receiver_owned(lua_State*L,int index){
    native_db_query_current(L);auto&p=owner(L);
    luaL_checktype(L,index,LUA_TUSERDATA);
    if(lua_objlen(L,index)!=sizeof(NativeGameCoreFractalPayload))
        return (luaL_error(L,"Fractal receiver is not its actual owned userdata"),*static_cast<NativeGameCoreFractalPayload*>(nullptr));
    auto*q=static_cast<NativeGameCoreFractalPayload*>(lua_touserdata(L,index));
    if(!q||q->magic!=NativeGameCoreFractalPayload::tag||q->issuer!=&p||q->vm!=p.state||q->epoch!=p.failure_epoch||q->id<=0||!q->release)
        return (luaL_error(L,"Fractal receiver lifetime/source changed"),*static_cast<NativeGameCoreFractalPayload*>(nullptr));
    return *q;
}
static int native_gamecore_fractal_gc_owned(lua_State*L){
    if(lua_type(L,1)!=LUA_TUSERDATA||lua_objlen(L,1)!=sizeof(NativeGameCoreFractalPayload))return luaL_error(L,"Fractal destructor receiver changed");
    auto*q=static_cast<NativeGameCoreFractalPayload*>(lua_touserdata(L,1));
    if(!q||q->magic!=NativeGameCoreFractalPayload::tag)return 0;
    const auto id=q->id;q->id=0;q->magic=0;
    try{if(id>0)q->release(id);}catch(...){auto&p=owner(L);p.source_exception=std::current_exception();p.source_error="Actual Fractal release failed";p.failed=true;p.ready=false;}
    q->~NativeGameCoreFractalPayload();return 0;
}
static int native_gamecore_fractal_flags_owned(lua_State*L,int index){
    luaL_checktype(L,index,LUA_TTABLE);const int top=lua_gettop(L);int bits=0;
    static constexpr const char*keys[]={"FRAC_WRAP_X","FRAC_WRAP_Y","FRAC_PERCENT","FRAC_POLAR","FRAC_CENTER_RIFT","FRAC_INVERT_HEIGHTS"};
    for(int i=0;i<6;++i){lua_getfield(L,index,keys[i]);if(!lua_isnil(L,-1))bits|=1<<i;}
    // false is present; getfield's __index effect remains observable.
    lua_settop(L,top);return bits;
}
static int native_gamecore_fractal_getheight_owned(lua_State*L){
    auto&q=native_gamecore_fractal_receiver_owned(L,1);native_db_query_current(L);auto&p=owner(L);
    const int n=lua_gettop(L)-1;if(n!=1&&n!=2)return luaL_error(L,"Invalid number of arguments.");
    if(!p.map.setup_fractal_height)return luaL_error(L,"Actual Fractal height provider unavailable");
    lua_pushcfunction(L,map_setup_fractal_height);lua_pushinteger(L,q.id);
    lua_pushinteger(L,lua_tointeger(L,2));if(n==2)lua_pushinteger(L,lua_tointeger(L,3));
    lua_call(L,n+1,1);native_db_query_current(L);return 1;
}
static int native_gamecore_fractal_buildridges_owned(lua_State*L){
    auto&q=native_gamecore_fractal_receiver_owned(L,1);native_db_query_current(L);auto&p=owner(L);
    if(!p.map.setup_fractal_ridges)return luaL_error(L,"Actual Fractal ridge provider unavailable");
    const int flags=native_gamecore_fractal_flags_owned(L,3);
    lua_pushcfunction(L,map_setup_fractal_ridges);lua_pushinteger(L,q.id);
    lua_pushinteger(L,lua_tointeger(L,2));lua_pushinteger(L,flags);
    lua_pushinteger(L,lua_tointeger(L,4));lua_pushinteger(L,lua_tointeger(L,5));
    lua_call(L,5,0);native_db_query_current(L);return 0;
}
static NativeGameCoreFractalPayload& native_gamecore_fractal_new_userdata_owned(lua_State*L){
    native_db_query_current(L);auto&p=owner(L);
    if(!p.map.setup_fractal_release)return (luaL_error(L,"Actual Fractal allocation/release pair unavailable"),*static_cast<NativeGameCoreFractalPayload*>(nullptr));
    auto*q=new(lua_newuserdata(L,sizeof(NativeGameCoreFractalPayload))) NativeGameCoreFractalPayload;
    q->issuer=&p;q->vm=p.state;q->epoch=p.failure_epoch;
    lua_newtable(L);lua_newtable(L);
    lua_pushvalue(L,-3);lua_pushcclosure(L,native_gamecore_fractal_getheight_owned,1);lua_setfield(L,-2,"GetHeight");
    lua_pushvalue(L,-3);lua_pushcclosure(L,native_gamecore_fractal_buildridges_owned,1);lua_setfield(L,-2,"BuildRidges");
    lua_setfield(L,-2,"__index");lua_pushcfunction(L,native_gamecore_fractal_gc_owned);lua_setfield(L,-2,"__gc");lua_setmetatable(L,-2);
    // Install the GC path before acquiring heap-owning callback state. If a
    // real copy allocation fails, the still-live zero-ID payload is collected.
    try{q->release=p.map.setup_fractal_release;}catch(...){p.source_exception=std::current_exception();return (luaL_error(L,"Fractal release retention allocation failed"),*static_cast<NativeGameCoreFractalPayload*>(nullptr));}
    return *q;
}
static int native_gamecore_fractal_create_owned(lua_State*L){
    // Match SDK userdata construction before reading flags/performing random
    // draws. The real existing provider takes six numeric arguments.
    auto&q=native_gamecore_fractal_new_userdata_owned(L);native_db_query_current(L);auto&p=owner(L);
    if(!p.map.setup_fractal_create)return luaL_error(L,"Actual Fractal creation provider unavailable");
    const int flags=native_gamecore_fractal_flags_owned(L,4);
    const auto x=lua_isnil(L,5)?7:lua_tointeger(L,5),y=lua_isnil(L,6)?6:lua_tointeger(L,6);
    lua_pushcfunction(L,map_setup_fractal_create);
    for(int i=1;i<=3;++i)lua_pushinteger(L,lua_tointeger(L,i));
    lua_pushinteger(L,flags);lua_pushinteger(L,x);lua_pushinteger(L,y);lua_call(L,6,1);
    native_db_query_current(L);q.id=lua_tointeger(L,-1);lua_pop(L,1);
    if(q.id<=0)return luaL_error(L,"Actual Fractal provider returned invalid handle");return 1;
}
static int native_gamecore_fractal_create_rifts_owned(lua_State*L){
    auto&q=native_gamecore_fractal_new_userdata_owned(L);native_db_query_current(L);auto&p=owner(L);
    if(!p.map.setup_fractal_create_rifts)return luaL_error(L,"Actual Fractal rift provider unavailable");
    const int flags=native_gamecore_fractal_flags_owned(L,4);auto&rift=native_gamecore_fractal_receiver_owned(L,5);
    const int x=luaL_optint(L,6,7),y=luaL_optint(L,7,6);
    lua_pushcfunction(L,map_setup_fractal_create_rifts);for(int i=1;i<=3;++i)lua_pushinteger(L,lua_tointeger(L,i));
    lua_pushinteger(L,flags);lua_pushinteger(L,rift.id);lua_pushinteger(L,x);lua_pushinteger(L,y);lua_call(L,7,1);
    native_db_query_current(L);q.id=lua_tointeger(L,-1);lua_pop(L,1);
    if(q.id<=0)return luaL_error(L,"Actual Fractal rift provider returned invalid handle");return 1;
}
static std::pair<std::size_t,std::size_t> native_gamecore_register_fractal_owned(lua_State*L){
    native_gamecore_export_current_owned(L);lua_getfield(L,LUA_GLOBALSINDEX,"Fractal");
    if(lua_isnil(L,-1)){lua_pop(L,1);lua_createtable(L,0,3);lua_pushvalue(L,-1);lua_setfield(L,LUA_GLOBALSINDEX,"Fractal");}
    lua_pushcfunction(L,native_gamecore_fractal_create_owned);lua_setfield(L,-2,"Create");
    native_gamecore_export_current_owned(L);
    lua_pushcfunction(L,native_gamecore_fractal_create_rifts_owned);lua_setfield(L,-2,"CreateRifts");
    native_gamecore_export_current_owned(L);
    // The protected zero-result pRegister call discards the offered table.
    // Creating these closures does not invoke any allocation/random callback.
    return {1,2};
}
