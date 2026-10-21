//Inside the actual Host::Impl, after Source9 and frozen Source1's
//native_control_base_setoffset_record_current_owned/signature body.
//Raw correction to frozen Source1 wording:89E530 RETAINS key y then x.
//8F8B10 creates references only. Lazy RAW field lookup and conversion are
//x then y at57AC40->8F8C60. Neither lookup invokes __index.
struct NativeControlBaseSetOffsetDecodeCall final {
    struct Reference {lua_State*state=nullptr;int reference=LUA_NOREF;};
    enum class Stage {entered,argument_retained,table_checked,keys_retained,
        x_converted,y_converted,released,member_returned,retired};
    const Impl*const host;
    const std::thread::id creator=std::this_thread::get_id();
    lua_State*const thread;
    lua_State*const master;
    const BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t epoch,writes,uncertainty;
    const void*const cdata;
    bool alive=true;
    Reference argument,key_y,key_x,temporary;
    std::optional<std::array<float,2>>value;
    Stage stage=Stage::entered;
    NativeControlBaseSetOffsetDecodeCall(Impl*p,lua_State*L,const void*actual)
        :host(p),thread(L),master(p->state),operation(p->worldview_operation),
        epoch(p->failure_epoch),writes(p->failed.writes_),uncertainty(p->execution_uncertainty),cdata(actual){
        if(!host||!thread||!master||!operation||!cdata)
            throw ScriptError("SetOffset actual decoder entry/Source/CData missing");
    }
};

[[noreturn]] static void native_control_base_setoffset_diagnostic_8f7960_required_owned(
    lua_State*L,const char*actual_site){
    native_control_base_setoffset_record_current_owned(L);
    //Complete8F7960: vsprintf_s1024 viaIAT9C968C ->actual73B280 ->real sink,
    //then NORMAL return. No genuine sink owner is supplied in this candidate.
    //This void requirement is not a diagnostic implementation/CFunction.
    luaL_error(L,"required genuine returning native8F7960/73B280 diagnostic sink at %s; its normal return cannot be replaced by a fabricated success",actual_site);
    std::terminate();
}

static void native_control_base_setoffset_decode_full_owned(
    lua_State*L,const NativeControlBaseSetOffsetDecodeCall&r){
    native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
    if(r.host!=&p||r.creator!=std::this_thread::get_id()||!r.alive||
        r.thread!=L||r.master!=p.state||r.operation!=p.worldview_operation||
        r.epoch!=p.failure_epoch||r.writes!=p.failed.writes_||
        r.uncertainty!=p.execution_uncertainty||!r.cdata||
        r.stage==NativeControlBaseSetOffsetDecodeCall::Stage::retired)
        return (void)luaL_error(L,"SetOffset decoder SAME VM/Source/CData occurrence changed");
}

static void native_control_base_setoffset_push_expected_metatype_owned(lua_State*L){
    native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
    //Actual89DE2A/89DECB consume the real current CA8208 callback cell.
    //Source9's SAME returned constructor supplies its executable6A6920 body.
    p.native_control_base_constructor->cdata_cell_ca8208(L);
    native_control_base_setoffset_record_current_owned(L);
}
static bool native_control_base_setoffset_cast_89de20_owned(lua_State*L,int offered){
    native_control_base_setoffset_push_expected_metatype_owned(L);
    const bool has_metatype=lua_getmetatable(L,offered)!=0;
    native_control_base_setoffset_record_current_owned(L);
    if(!has_metatype){lua_settop(L,-2);native_control_base_setoffset_record_current_owned(L);return false;}
    bool equal=lua_equal(L,-1,-2)!=0;native_control_base_setoffset_record_current_owned(L);
    while(!equal){
        lua_getfield(L,-1,"BaseType");native_control_base_setoffset_record_current_owned(L);
        const bool table=lua_type(L,-1)==LUA_TTABLE;native_control_base_setoffset_record_current_owned(L);
        if(!table){lua_settop(L,-4);native_control_base_setoffset_record_current_owned(L);return false;}
        lua_remove(L,-2);native_control_base_setoffset_record_current_owned(L);
        equal=lua_equal(L,-1,-2)!=0;native_control_base_setoffset_record_current_owned(L);
    }
    lua_settop(L,-3);native_control_base_setoffset_record_current_owned(L);return true;
}
static const char*native_control_base_setoffset_expected_name_89dec0_owned(lua_State*L){
    native_control_base_setoffset_push_expected_metatype_owned(L);
    lua_getfield(L,-1,"CTypeName");native_control_base_setoffset_record_current_owned(L);
    const char*const name=lua_tolstring(L,-1,nullptr);native_control_base_setoffset_record_current_owned(L);
    lua_settop(L,-3);native_control_base_setoffset_record_current_owned(L);
    return name?name:"Unkown_Type";
}
static const char*native_control_base_setoffset_offered_name_89e000_owned(lua_State*L,int offered){
    const bool has_metatype=lua_getmetatable(L,offered)!=0;native_control_base_setoffset_record_current_owned(L);
    if(!has_metatype)return "Unkown_Type";
    lua_getfield(L,-1,"CTypeName");native_control_base_setoffset_record_current_owned(L);
    const char*const name=lua_tolstring(L,-1,nullptr);native_control_base_setoffset_record_current_owned(L);
    lua_settop(L,-3);native_control_base_setoffset_record_current_owned(L);
    return name?name:"Unkown_Type";
}
static const void*native_control_base_setoffset_cdata_89e050_owned(lua_State*L,int offered){
    native_control_base_setoffset_record_current_owned(L);
    const int type=lua_type(L,offered);native_control_base_setoffset_record_current_owned(L);
    if(type==LUA_TTABLE){
        if(native_control_base_setoffset_cast_89de20_owned(L,offered)){
            lua_pushvalue(L,offered);native_control_base_setoffset_record_current_owned(L);
            lua_getfield(L,-1,"CData");native_control_base_setoffset_record_current_owned(L);
            const void*const result=lua_touserdata(L,-1);native_control_base_setoffset_record_current_owned(L);
            lua_settop(L,-3);native_control_base_setoffset_record_current_owned(L);return result;
        }
        //Actual expected-name89DEC0 then offered-name89E000 effects precede
        //89E0B3's diagnostic. Popped string pointers are never dereferenced
        //after another allocating API; the real diagnostic remains required.
        (void)native_control_base_setoffset_expected_name_89dec0_owned(L);
        (void)native_control_base_setoffset_offered_name_89e000_owned(L,offered);
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E0B3 failed reflection cast");
    }
    //Original repeats lua_type on this branch and accepts ONLY lightuserdata.
    const int repeated=lua_type(L,offered);native_control_base_setoffset_record_current_owned(L);
    if(repeated==LUA_TLIGHTUSERDATA){
        const void*const result=lua_touserdata(L,offered);native_control_base_setoffset_record_current_owned(L);return result;
    }
    native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E0E3 incorrect pointer argument type");
}

static NativeControlBaseSetOffsetDecodeCall*native_control_base_setoffset_begin_decode_owned(
    lua_State*L,const void*cdata){
    native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        p.native_control_base_setoffset_decode_calls.push_back(
            std::make_unique<NativeControlBaseSetOffsetDecodeCall>(&p,L,cdata));
    }catch(...){p.source_exception=std::current_exception();p.source_error="SetOffset decoder custody allocation failed";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);return nullptr;}
    auto*r=p.native_control_base_setoffset_decode_calls.back().get();
    native_control_base_setoffset_decode_full_owned(L,*r);return r;
}

static void native_control_base_setoffset_reference_release_8f8820_owned(
    lua_State*L,NativeControlBaseSetOffsetDecodeCall&r,NativeControlBaseSetOffsetDecodeCall::Reference&value){
    native_control_base_setoffset_decode_full_owned(L,r);
    if(value.state&&value.reference!=LUA_NOREF){
        if(value.state!=L)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"FLua reference foreign state");
        luaL_unref(L,LUA_REGISTRYINDEX,value.reference);native_control_base_setoffset_decode_full_owned(L,r);
    }
    value.state=nullptr;value.reference=LUA_NOREF;
}
static void native_control_base_setoffset_reference_from_top_8f8b10_owned(
    lua_State*L,NativeControlBaseSetOffsetDecodeCall&r,NativeControlBaseSetOffsetDecodeCall::Reference&value){
    native_control_base_setoffset_reference_release_8f8820_owned(L,r,value);
    value.state=L;
    (void)lua_checkstack(L,1);native_control_base_setoffset_decode_full_owned(L,r);
    lua_pushvalue(L,-1);native_control_base_setoffset_decode_full_owned(L,r);
    value.reference=luaL_ref(L,LUA_REGISTRYINDEX);native_control_base_setoffset_decode_full_owned(L,r);
}
static bool native_control_base_setoffset_reference_push_8f86d0_owned(
    lua_State*L,NativeControlBaseSetOffsetDecodeCall&r,const NativeControlBaseSetOffsetDecodeCall::Reference&value){
    native_control_base_setoffset_decode_full_owned(L,r);
    if(!value.state||value.reference==LUA_NOREF)return false;
    if(value.state!=L)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"FLua reference foreign state");
    (void)lua_checkstack(L,1);native_control_base_setoffset_decode_full_owned(L,r);
    lua_rawgeti(L,LUA_REGISTRYINDEX,value.reference);native_control_base_setoffset_decode_full_owned(L,r);return true;
}
static bool native_control_base_setoffset_reference_table_8f88a0_owned(
    lua_State*L,NativeControlBaseSetOffsetDecodeCall&r){
    if(!native_control_base_setoffset_reference_push_8f86d0_owned(L,r,r.argument))return false;
    const bool table=lua_type(L,-1)==LUA_TTABLE;native_control_base_setoffset_decode_full_owned(L,r);
    lua_settop(L,-2);native_control_base_setoffset_decode_full_owned(L,r);return table;
}
static bool native_control_base_setoffset_reference_table_push_8f88f0_owned(
    lua_State*L,NativeControlBaseSetOffsetDecodeCall&r){
    if(!native_control_base_setoffset_reference_push_8f86d0_owned(L,r,r.argument))return false;
    const bool table=lua_type(L,-1)==LUA_TTABLE;native_control_base_setoffset_decode_full_owned(L,r);
    if(!table){lua_settop(L,-2);native_control_base_setoffset_decode_full_owned(L,r);}return table;
}
static void native_control_base_setoffset_field_8f8c60_owned(lua_State*L,
    NativeControlBaseSetOffsetDecodeCall&r,const NativeControlBaseSetOffsetDecodeCall::Reference&key){
    //Output8F8C6D/73 is genuinely unbound before its conditional lookup.
    native_control_base_setoffset_reference_release_8f8820_owned(L,r,r.temporary);
    if(r.argument.state!=key.state)
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"8F8C83 key belongs to different Lua state");
    if(!native_control_base_setoffset_reference_table_push_8f88f0_owned(L,r))return;
    if(!native_control_base_setoffset_reference_push_8f86d0_owned(L,r,key))
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"8F8D39 invalid key");
    const bool nil=lua_type(L,-1)==LUA_TNIL;native_control_base_setoffset_decode_full_owned(L,r);
    if(nil){
        lua_settop(L,-2);native_control_base_setoffset_decode_full_owned(L,r);
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"8F8D19 nil key");
    }
    lua_rawget(L,-2);native_control_base_setoffset_decode_full_owned(L,r);
    native_control_base_setoffset_reference_from_top_8f8b10_owned(L,r,r.temporary);
    lua_settop(L,-2);native_control_base_setoffset_decode_full_owned(L,r);
    lua_settop(L,-2);native_control_base_setoffset_decode_full_owned(L,r);
}
static float native_control_base_setoffset_float_57ac40_owned(lua_State*L,
    NativeControlBaseSetOffsetDecodeCall&r,const NativeControlBaseSetOffsetDecodeCall::Reference&key){
    native_control_base_setoffset_field_8f8c60_owned(L,r,key);
    //57AC54/5B actually initializes its numeric local to FP32 +0. This is
    //original argument conversion, not invented control/geometry storage.
    float result=0.0f;
    if(native_control_base_setoffset_reference_push_8f86d0_owned(L,r,r.temporary)){
        const int index=lua_gettop(L);native_control_base_setoffset_decode_full_owned(L,r);
        result=static_cast<float>(lua_tonumber(L,index));native_control_base_setoffset_decode_full_owned(L,r);
        //No luaL_checknumber/isnumber, bool rejection or added finite rule.
        lua_settop(L,-2);native_control_base_setoffset_decode_full_owned(L,r);
    }
    native_control_base_setoffset_reference_release_8f8820_owned(L,r,r.temporary);return result;
}
static std::array<float,2>native_control_base_setoffset_vector_89e530_owned(
    lua_State*L,NativeControlBaseSetOffsetDecodeCall&r,int offered){
    native_control_base_setoffset_decode_full_owned(L,r);
    r.argument.state=L;r.argument.reference=LUA_NOREF;
    (void)lua_checkstack(L,1);native_control_base_setoffset_decode_full_owned(L,r);
    lua_pushvalue(L,offered);native_control_base_setoffset_decode_full_owned(L,r);
    r.argument.reference=luaL_ref(L,LUA_REGISTRYINDEX);native_control_base_setoffset_decode_full_owned(L,r);
    r.stage=NativeControlBaseSetOffsetDecodeCall::Stage::argument_retained;
    const bool table=native_control_base_setoffset_reference_table_8f88a0_owned(L,r);
    r.stage=NativeControlBaseSetOffsetDecodeCall::Stage::table_checked;
    if(!table){
        r.value=std::array<float,2>{0.0f,0.0f}; //Actual89E620/624/628 stores.
        native_control_base_setoffset_reference_release_8f8820_owned(L,r,r.argument);
        r.stage=NativeControlBaseSetOffsetDecodeCall::Stage::released;return *r.value;
    }
    (void)lua_checkstack(L,1);native_control_base_setoffset_decode_full_owned(L,r);
    lua_pushstring(L,"y");native_control_base_setoffset_decode_full_owned(L,r);
    native_control_base_setoffset_reference_from_top_8f8b10_owned(L,r,r.key_y);
    lua_settop(L,-2);native_control_base_setoffset_decode_full_owned(L,r);
    (void)lua_checkstack(L,1);native_control_base_setoffset_decode_full_owned(L,r);
    lua_pushstring(L,"x");native_control_base_setoffset_decode_full_owned(L,r);
    native_control_base_setoffset_reference_from_top_8f8b10_owned(L,r,r.key_x);
    lua_settop(L,-2);native_control_base_setoffset_decode_full_owned(L,r);
    r.stage=NativeControlBaseSetOffsetDecodeCall::Stage::keys_retained;
    const float x=native_control_base_setoffset_float_57ac40_owned(L,r,r.key_x);
    r.stage=NativeControlBaseSetOffsetDecodeCall::Stage::x_converted;
    const float y=native_control_base_setoffset_float_57ac40_owned(L,r,r.key_y);
    r.stage=NativeControlBaseSetOffsetDecodeCall::Stage::y_converted;r.value=std::array<float,2>{x,y};
    //Actual89E5FF/608/611 destroys x key, y key, then argument reference.
    native_control_base_setoffset_reference_release_8f8820_owned(L,r,r.key_x);
    native_control_base_setoffset_reference_release_8f8820_owned(L,r,r.key_y);
    native_control_base_setoffset_reference_release_8f8820_owned(L,r,r.argument);
    r.stage=NativeControlBaseSetOffsetDecodeCall::Stage::released;return *r.value;
}

void native_control_base_setoffset_decoder_retire()noexcept{
    for(const auto&r:native_control_base_setoffset_decode_calls)if(r){
        if(r->host!=this||r->creator!=std::this_thread::get_id())std::terminate();
        r->alive=false;r->stage=NativeControlBaseSetOffsetDecodeCall::Stage::retired;
        //Uncertain partial registry refs are reclaimed by SAME VM lua_close.
    }
}
