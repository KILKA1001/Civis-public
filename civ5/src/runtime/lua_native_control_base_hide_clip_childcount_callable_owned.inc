// Inside the actual Host::Impl after the current Source13 Base/CData kernels.
// One finite ordinal8..11 candidate; parent owns all four physical seams.
static const NativeContextDefinition::Record*native_control_base_hide_clip_childcount_record_current_owned(
    lua_State*L,std::size_t ordinal){
    native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
    struct Recipe {std::uint32_t writer,key,primary,secondary;unsigned char flag;const char*name;};
    static constexpr Recipe exact[]={
        {0x9be100u,0xa3f5acu,0x7a54b0u,0x741e00u,0,"SetHide"},
        {0x9be160u,0xa3f5d8u,0x723910u,0x740110u,0,"SetNoClip"},
        {0x9be1c0u,0xa3f608u,0x4c4bd0u,0x4b0730u,0,"IsHidden"},
        {0x9be220u,0xa3f640u,0x47e630u,0x687190u,1,"GetNumChildren"},
    };
    if(ordinal<8||ordinal>11)
        return (luaL_error(L,"ControlBase hide/clip/childcount requires actual internal ordinal8..11"),nullptr);
    const auto&proof=exact[ordinal-8];
    const auto&record=p.native_control_base_constructor->definition->records[ordinal];
    if(!record.recipe||!record.key||record.key!=record.recipe->key||
        record.recipe->writer!=proof.writer||record.recipe->key_address!=proof.key||
        std::strcmp(record.key,proof.name)||record.primary!=proof.primary||
        record.secondary!=proof.secondary||record.primary!=record.recipe->primary||
        record.secondary!=record.recipe->secondary||record.flag!=proof.flag||
        record.flag!=record.recipe->flag||record.reference!=-1)
        return (luaL_error(L,"ControlBase hide/clip/childcount actual ordinal/writer/key/functions/flag recipe changed"),nullptr);
    return &record;
}

// Agreed parent seams, DEFINED by the parent before this INC. These comments
// provide their contracts; no accepting declaration/definition is supplied.
// static void native_control_base_sethide_member_6545c0_owned(lua_State*,const void*,bool);
// static void native_control_base_setnoclip_member_72c700_owned(lua_State*,const void*,bool);
// static bool native_control_base_ishidden_flag40_owned(lua_State*,const void*);
// static std::uint32_t native_control_base_numchildren_78_08_owned(lua_State*,const void*);
// Every seam binds the SAME current physical CData/receiver/Source. Read
// results are POD after ALL strong C++ borrowers have ended before Lua APIs.

[[noreturn]] static void native_control_base_numchildren_const_ca8d24_required_owned(
    lua_State*L,const char*actual_site){
    native_control_base_hide_clip_childcount_record_current_owned(L,11u);
    // Actual89DF0F/89DFC0 load the LIVE CA8D24 callback cell. Its zero image
    // initializer cannot prove the executing cell NULL. Neither a fabricated
    // callback nor an assumed NULL-cell branch is a valid continuation.
    luaL_error(L,"required genuine live CA8D24 callback/state and actual89DF00/89DFC0 fallback owner at %s",actual_site);
    std::terminate();
}
static bool native_control_base_numchildren_cast_89df00_owned(lua_State*L,int offered){
    native_control_base_hide_clip_childcount_record_current_owned(L,11u);
    // Exact89DF06 operands: SAME Lua state and offered positiveindex1 reach
    // actual89DE20 FIRST. Its real CA8208 Base caster is already owned.
    const bool base=native_control_base_setoffset_cast_89de20_owned(L,offered);
    native_control_base_hide_clip_childcount_record_current_owned(L,11u);
    if(base)return true; // Actual89DF0D ->89DF81 returnsAL1 immediately.
    native_control_base_numchildren_const_ca8d24_required_owned(L,
        "89DF0F failed Base89DE20 cast before live CA8D24 fallback");
}
static const void*native_control_base_numchildren_cdata_89e480_owned(lua_State*L,int offered){
    native_control_base_hide_clip_childcount_record_current_owned(L,11u);
    const int type=lua_type(L,offered);
    native_control_base_hide_clip_childcount_record_current_owned(L,11u);
    if(type==LUA_TTABLE){
        if(native_control_base_numchildren_cast_89df00_owned(L,offered)){
            lua_pushvalue(L,offered);native_control_base_hide_clip_childcount_record_current_owned(L,11u);
            lua_getfield(L,-1,"CData");native_control_base_hide_clip_childcount_record_current_owned(L,11u);
            const void*const result=lua_touserdata(L,-1);
            native_control_base_hide_clip_childcount_record_current_owned(L,11u);
            lua_settop(L,-3);native_control_base_hide_clip_childcount_record_current_owned(L,11u);return result;
        }
        // Current candidate stops at the first live-cell supplier in89DF00.
        // A future false fallback MUST perform89DFC0(expected CTypeName),
        // then89E000(offered CTypeName), then real8F7960 at89E4E7. Do not skip
        // those effects by returningNULL or by logging an invented message.
        native_control_base_numchildren_const_ca8d24_required_owned(L,
            "89E4D4 expected-name89DFC0 before offered-name89E000/diagnostic89E4E7");
    }
    const int repeated=lua_type(L,offered);
    native_control_base_hide_clip_childcount_record_current_owned(L,11u);
    if(repeated==LUA_TLIGHTUSERDATA){
        const void*const result=lua_touserdata(L,offered);
        native_control_base_hide_clip_childcount_record_current_owned(L,11u);return result;
    }
    native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
        "89E517 GetNumChildren incorrect pointer argument type");
}

static int native_control_base_hide_clip_dispatch_89e2d0_owned(lua_State*L,std::size_t ordinal){
    native_control_base_hide_clip_childcount_record_current_owned(L,ordinal);
    if(ordinal!=8u&&ordinal!=9u)return luaL_error(L,"ControlBase89E2D0 requires actual SetHide/SetNoClip identity");
    const int count=lua_gettop(L);native_control_base_hide_clip_childcount_record_current_owned(L,ordinal);
    if(count!=2){
        (void)lua_gettop(L);native_control_base_hide_clip_childcount_record_current_owned(L,ordinal); // Actual89E331.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
            ordinal==8u?"89E33D ControlBase::SetHide exact receiver-plus-one-argument count":
                        "89E33D ControlBase::SetNoClip exact receiver-plus-one-argument count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_hide_clip_childcount_record_current_owned(L,ordinal);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
        ordinal==8u?"89E322 ControlBase::SetHide NULL receiver":"89E322 ControlBase::SetNoClip NULL receiver");
    // Actual89E2FA lua_toboolean, then SETNZ. Lua0/emptystring are true;
    // nil/false are false. No argument type check or numeric conversion.
    const bool value=lua_toboolean(L,2)!=0;
    native_control_base_hide_clip_childcount_record_current_owned(L,ordinal);
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase hide/clip requires actual Lua stack reserve32");
    native_control_base_hide_clip_childcount_record_current_owned(L,ordinal);
    if(lua_gettop(L)!=2)return luaL_error(L,"ControlBase89E2D0 receiver/argument stack changed");
    if(ordinal==8u)native_control_base_sethide_member_6545c0_owned(L,cdata,value);
    else native_control_base_setnoclip_member_72c700_owned(L,cdata,value);
    native_control_base_hide_clip_childcount_record_current_owned(L,ordinal);return 0;
}
static int native_control_base_sethide_primary_7a54b0_owned(lua_State*L){
    return native_control_base_hide_clip_dispatch_89e2d0_owned(L,8u);
}
static int native_control_base_setnoclip_primary_723910_owned(lua_State*L){
    return native_control_base_hide_clip_dispatch_89e2d0_owned(L,9u);
}
static int native_control_base_ishidden_primary_4c4bd0_owned(lua_State*L){
    native_control_base_hide_clip_childcount_record_current_owned(L,10u);
    const int count=lua_gettop(L);native_control_base_hide_clip_childcount_record_current_owned(L,10u);
    if(count!=1){
        (void)lua_gettop(L);native_control_base_hide_clip_childcount_record_current_owned(L,10u); // Actual4C4C1E.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
            "4C4C2E ControlBase::IsHidden exact receiver-only count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_hide_clip_childcount_record_current_owned(L,10u);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
        "4C4C11 ControlBase::IsHidden NULL receiver");
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase IsHidden requires actual Lua stack reserve32");
    native_control_base_hide_clip_childcount_record_current_owned(L,10u);
    if(lua_gettop(L)!=1)return luaL_error(L,"ControlBase IsHidden receiver stack changed");
    const bool hidden=native_control_base_ishidden_flag40_owned(L,cdata); // Actual READ40 ->SHR1 ->AND1.
    native_control_base_hide_clip_childcount_record_current_owned(L,10u);
    lua_pushboolean(L,hidden?1:0);native_control_base_hide_clip_childcount_record_current_owned(L,10u);return 1;
}
static int native_control_base_getnumchildren_primary_47e630_owned(lua_State*L){
    native_control_base_hide_clip_childcount_record_current_owned(L,11u);
    const int count=lua_gettop(L);native_control_base_hide_clip_childcount_record_current_owned(L,11u);
    if(count!=1){
        (void)lua_gettop(L);native_control_base_hide_clip_childcount_record_current_owned(L,11u); // Actual47E69E.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
            "47E6AE ControlBase::GetNumChildren exact receiver-only count");
    }
    const void*const cdata=native_control_base_numchildren_cdata_89e480_owned(L,1);
    native_control_base_hide_clip_childcount_record_current_owned(L,11u);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
        "47E690 ControlBase::GetNumChildren NULL receiver");
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase GetNumChildren requires actual Lua stack reserve32");
    native_control_base_hide_clip_childcount_record_current_owned(L,11u);
    if(lua_gettop(L)!=1)return luaL_error(L,"ControlBase GetNumChildren receiver stack changed");
    // Genuine actual storage78/list08 ownership is REQUIRED from the parent.
    // Its NULL-storage branch returns0; no synthetic empty child collection.
    const std::uint32_t children=native_control_base_numchildren_78_08_owned(L,cdata);
    native_control_base_hide_clip_childcount_record_current_owned(L,11u);
    // Original FILDsigned32 adds2^32 for a negative signed interpretation.
    // Exact uint32-to-double expansion preserves every0..UINT32_MAX value.
    lua_pushnumber(L,static_cast<lua_Number>(children));
    native_control_base_hide_clip_childcount_record_current_owned(L,11u);return 1;
}

static int native_control_base_hide_clip_childcount_signature_owned(lua_State*L,std::size_t ordinal){
    const auto*const record=native_control_base_hide_clip_childcount_record_current_owned(L,ordinal);
    char buffer[0x100]{};
    if(ordinal==8u||ordinal==9u)
        (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","",record->key,"bool","");
    else if(ordinal==10u)
        (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","bool","",record->key);
    else
        (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void ) const","uint","",record->key);
    native_control_base_hide_clip_childcount_record_current_owned(L,ordinal);
    lua_pushstring(L,buffer);native_control_base_hide_clip_childcount_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_sethide_signature_741e00_owned(lua_State*L){
    return native_control_base_hide_clip_childcount_signature_owned(L,8u);
}
static int native_control_base_setnoclip_signature_740110_owned(lua_State*L){
    return native_control_base_hide_clip_childcount_signature_owned(L,9u);
}
static int native_control_base_ishidden_signature_4b0730_owned(lua_State*L){
    return native_control_base_hide_clip_childcount_signature_owned(L,10u);
}
static int native_control_base_getnumchildren_signature_687190_owned(lua_State*L){
    return native_control_base_hide_clip_childcount_signature_owned(L,11u);
}
