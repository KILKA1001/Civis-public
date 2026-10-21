// Inside the actual Host::Impl after Source14's current Base/CData kernels
// and Source2's genuine SAME Host/L/Source/CData FLua reference custody.
// Finite ordinals12..16 only; parent owns every physical member/read seam.
static const NativeContextDefinition::Record*native_control_base_reprocess_size_record_current_owned(
    lua_State*L,std::size_t ordinal){
    native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
    struct Recipe {std::uint32_t writer,key,primary,secondary;const char*name;};
    static constexpr Recipe exact[]={
        {0x9be280u,0xa3f684u,0x4f7570u,0x43f790u,"ReprocessAnchoring"},
        {0x9be2e0u,0xa3f6b8u,0x537c60u,0x782fb0u,"GetSize"},
        {0x9be340u,0xa3f6e4u,0x600630u,0x69fec0u,"GetSizeX"},
        {0x9be3a0u,0xa3f714u,0x421820u,0x570d40u,"GetSizeY"},
        {0x9be400u,0xa3f744u,0x49f370u,0x75b3c0u,"GetSizeVal"},
    };
    if(ordinal<12||ordinal>16)
        return (luaL_error(L,"ControlBase reprocess/size requires actual internal ordinal12..16"),nullptr);
    const auto&proof=exact[ordinal-12];
    const auto&record=p.native_control_base_constructor->definition->records[ordinal];
    if(!record.recipe||!record.key||record.key!=record.recipe->key||
        record.recipe->writer!=proof.writer||record.recipe->key_address!=proof.key||
        std::strcmp(record.key,proof.name)||record.primary!=proof.primary||
        record.secondary!=proof.secondary||record.primary!=record.recipe->primary||
        record.secondary!=record.recipe->secondary||record.flag!=0||
        record.flag!=record.recipe->flag||record.reference!=-1)
        return (luaL_error(L,"ControlBase reprocess/size actual ordinal/writer/key/functions/flag recipe changed"),nullptr);
    return &record;
}

// Agreed parent seams, DEFINED before this INC. No accepting definitions:
// static void native_control_base_reprocessanchoring_member_4a7f10_owned(lua_State*,const void*);
// static std::array<float,2> native_control_base_getsize_member_5f4190_owned(lua_State*,const void*);
// static float native_control_base_getsize_x_member_65b6c0_owned(lua_State*,const void*);
// static float native_control_base_getsize_y_member_5bf320_owned(lua_State*,const void*);
// static float native_control_base_getsize_val_axis_owned(lua_State*,const void*,bool x_axis);
// Every physical borrower ends before any allocating Lua API or luaL_error.
// Val bool selects only actual READ30/READ34; it grants no cached pair/ID.

static void native_control_base_getsize_result_current_owned(
    lua_State*L,const NativeControlBaseSetOffsetDecodeCall&r){
    native_control_base_reprocess_size_record_current_owned(L,13u);
    // This existing PURE guard checks SAME Host/L/master/Source operation,
    // thread/epoch/writes/uncertainty/CData/alive. It certifies no ordinal0
    // argument conversion, native SetOffset call, or old decoder return.
    native_control_base_setoffset_decode_full_owned(L,r);
    native_control_base_reprocess_size_record_current_owned(L,13u);
}
static void native_control_base_getsize_field_4aaec0_owned(lua_State*L,
    NativeControlBaseSetOffsetDecodeCall&r,
    const NativeControlBaseSetOffsetDecodeCall::Reference&key,float value){
    native_control_base_getsize_result_current_owned(L,r);
    if(r.argument.state!=key.state)
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"4AAED1 FLua::Table::SetField foreign key state");
    if(!native_control_base_setoffset_reference_table_push_8f88f0_owned(L,r)){
        native_control_base_getsize_result_current_owned(L,r);return; // Actual4AAF72 normal non-table return.
    }
    native_control_base_getsize_result_current_owned(L,r);
    if(!native_control_base_setoffset_reference_push_8f86d0_owned(L,r,key))
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"4AAF5F FLua::Table::SetField invalid key");
    native_control_base_getsize_result_current_owned(L,r);
    const bool nil=lua_type(L,-1)==LUA_TNIL;native_control_base_getsize_result_current_owned(L,r);
    if(nil){
        lua_settop(L,-2);native_control_base_getsize_result_current_owned(L,r); // Actual4AAF08 pops key before diagnostic.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"4AAF12 nil key sent to FLua::Table::SetField");
    }
    (void)lua_checkstack(L,1);native_control_base_getsize_result_current_owned(L,r);
    lua_pushnumber(L,static_cast<lua_Number>(value));native_control_base_getsize_result_current_owned(L,r);
    lua_rawset(L,-3);native_control_base_getsize_result_current_owned(L,r); // Actual4AAF42, no __newindex.
    lua_settop(L,-2);native_control_base_getsize_result_current_owned(L,r); // Actual4AAF4D pops extra table.
}
static void native_control_base_getsize_vector_89e640_owned(
    lua_State*L,const void*cdata,const std::array<float,2>&value){
    native_control_base_reprocess_size_record_current_owned(L,13u);
    // Allocate a NEW existing custody capsule. No prior reference/value or
    // completed call is reused; this allocation owns only current FLua refs.
    auto*r=native_control_base_setoffset_begin_decode_owned(L,cdata);
    native_control_base_getsize_result_current_owned(L,*r);
    if(r->argument.state||r->key_x.state||r->key_y.state||r->temporary.state||r->value||
        r->stage!=NativeControlBaseSetOffsetDecodeCall::Stage::entered)
        return (void)luaL_error(L,"GetSize vector output requires new unbound FLua reference custody");
    lua_createtable(L,0,2);native_control_base_getsize_result_current_owned(L,*r); // Actual89E64B.
    // Actual8F8540 unbound/currentL constructor has the same checkstack1,
    // pushvalue(-1), luaL_ref effects as the owned8F8B10 unbound kernel.
    native_control_base_setoffset_reference_from_top_8f8b10_owned(L,*r,r->argument);
    native_control_base_getsize_result_current_owned(L,*r);
    (void)lua_checkstack(L,1);native_control_base_getsize_result_current_owned(L,*r);
    lua_pushstring(L,"x");native_control_base_getsize_result_current_owned(L,*r);
    native_control_base_setoffset_reference_from_top_8f8b10_owned(L,*r,r->key_x);
    native_control_base_getsize_result_current_owned(L,*r);
    lua_settop(L,-2);native_control_base_getsize_result_current_owned(L,*r);
    native_control_base_getsize_field_4aaec0_owned(L,*r,r->key_x,value[0]);
    native_control_base_setoffset_reference_release_8f8820_owned(L,*r,r->key_x); // Actual89E6BD: unref x BEFORE y construction.
    native_control_base_getsize_result_current_owned(L,*r);
    (void)lua_checkstack(L,1);native_control_base_getsize_result_current_owned(L,*r);
    lua_pushstring(L,"y");native_control_base_getsize_result_current_owned(L,*r);
    native_control_base_setoffset_reference_from_top_8f8b10_owned(L,*r,r->key_y);
    native_control_base_getsize_result_current_owned(L,*r);
    lua_settop(L,-2);native_control_base_getsize_result_current_owned(L,*r);
    native_control_base_getsize_field_4aaec0_owned(L,*r,r->key_y,value[1]);
    native_control_base_setoffset_reference_release_8f8820_owned(L,*r,r->key_y); // Actual89E721.
    native_control_base_setoffset_reference_release_8f8820_owned(L,*r,r->argument); // Actual89E72A.
    native_control_base_getsize_result_current_owned(L,*r);
    r->stage=NativeControlBaseSetOffsetDecodeCall::Stage::released; // Ref custody only, no member/ordinal0 completion.
    // Partial refs on Lua longjmp remain owned by this SAME-VM capsule and
    // existing decoder_retire-before-lua_close; normal return holds no refs.
}

static int native_control_base_reprocessanchoring_primary_4f7570_owned(lua_State*L){
    native_control_base_reprocess_size_record_current_owned(L,12u);
    const int count=lua_gettop(L);native_control_base_reprocess_size_record_current_owned(L,12u);
    if(count!=1){
        (void)lua_gettop(L);native_control_base_reprocess_size_record_current_owned(L,12u); // Actual4F75B0.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"4F75C0 ControlBase::ReprocessAnchoring receiver-only count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_reprocess_size_record_current_owned(L,12u);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"4F75A3 ControlBase::ReprocessAnchoring NULL receiver");
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase ReprocessAnchoring requires actual Lua stack reserve32");
    native_control_base_reprocess_size_record_current_owned(L,12u);
    if(lua_gettop(L)!=1)return luaL_error(L,"ControlBase ReprocessAnchoring receiver stack changed");
    native_control_base_reprocessanchoring_member_4a7f10_owned(L,cdata); // Actual4F7590 thiscall, no member arguments.
    native_control_base_reprocess_size_record_current_owned(L,12u);return 0;
}
static int native_control_base_getsize_primary_537c60_owned(lua_State*L){
    native_control_base_reprocess_size_record_current_owned(L,13u);
    const int count=lua_gettop(L);native_control_base_reprocess_size_record_current_owned(L,13u);
    if(count!=1){
        (void)lua_gettop(L);native_control_base_reprocess_size_record_current_owned(L,13u); // Actual89EA2A.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89EA36 ControlBase::GetSize receiver-only count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_reprocess_size_record_current_owned(L,13u);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89EA1A ControlBase::GetSize NULL receiver");
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase GetSize requires actual Lua stack reserve32");
    native_control_base_reprocess_size_record_current_owned(L,13u);
    if(lua_gettop(L)!=1)return luaL_error(L,"ControlBase89E9C0 receiver stack changed");
    // Original89E9DF hidden8byte member output, then89E9EA/EE copy BOTH
    // returned FP32 components BEFORE allocating the Lua vector table.
    const std::array<float,2>value=native_control_base_getsize_member_5f4190_owned(L,cdata);
    native_control_base_reprocess_size_record_current_owned(L,13u);
    native_control_base_getsize_vector_89e640_owned(L,cdata,value);
    native_control_base_reprocess_size_record_current_owned(L,13u);return 1;
}
static int native_control_base_getsize_scalar_dispatch_89e200_owned(lua_State*L,bool x_axis){
    const std::size_t ordinal=x_axis?14u:15u;
    native_control_base_reprocess_size_record_current_owned(L,ordinal);
    const int count=lua_gettop(L);native_control_base_reprocess_size_record_current_owned(L,ordinal);
    if(count!=1){
        (void)lua_gettop(L);native_control_base_reprocess_size_record_current_owned(L,ordinal); // Actual89E246.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
            x_axis?"89E252 ControlBase::GetSizeX receiver-only count":"89E252 ControlBase::GetSizeY receiver-only count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_reprocess_size_record_current_owned(L,ordinal);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
        x_axis?"89E23A ControlBase::GetSizeX NULL receiver":"89E23A ControlBase::GetSizeY NULL receiver");
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase size scalar getter requires actual Lua stack reserve32");
    native_control_base_reprocess_size_record_current_owned(L,ordinal);
    if(lua_gettop(L)!=1)return luaL_error(L,"ControlBase89E200 size receiver stack changed");
    const float value=x_axis?native_control_base_getsize_x_member_65b6c0_owned(L,cdata):
                             native_control_base_getsize_y_member_5bf320_owned(L,cdata);
    native_control_base_reprocess_size_record_current_owned(L,ordinal);
    lua_pushnumber(L,static_cast<lua_Number>(value));
    native_control_base_reprocess_size_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_getsize_x_primary_600630_owned(lua_State*L){
    return native_control_base_getsize_scalar_dispatch_89e200_owned(L,true);
}
static int native_control_base_getsize_y_primary_421820_owned(lua_State*L){
    return native_control_base_getsize_scalar_dispatch_89e200_owned(L,false);
}
static int native_control_base_getsize_val_primary_49f370_owned(lua_State*L){
    native_control_base_reprocess_size_record_current_owned(L,16u);
    // Actual49F370 tailJMP467420. Positive1 view8F77F0 adds NO arity check,
    // reference or cleanup. Extra Lua arguments remain ignored.
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_reprocess_size_record_current_owned(L,16u);
    if(!cdata)return 0; // Actual467473, no invented NULL diagnostic.
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase GetSizeVal requires actual Lua stack reserve32");
    native_control_base_reprocess_size_record_current_owned(L,16u);
    const float x=native_control_base_getsize_val_axis_owned(L,cdata,true); // Actual467447 READ30.
    native_control_base_reprocess_size_record_current_owned(L,16u);
    lua_pushnumber(L,static_cast<lua_Number>(x));native_control_base_reprocess_size_record_current_owned(L,16u); // Actual467451.
    const float y=native_control_base_getsize_val_axis_owned(L,cdata,false); // Actual467456 FRESH READ34 AFTER pushX.
    native_control_base_reprocess_size_record_current_owned(L,16u);
    lua_pushnumber(L,static_cast<lua_Number>(y));native_control_base_reprocess_size_record_current_owned(L,16u);return 2;
}

static int native_control_base_reprocess_size_signature_owned(lua_State*L,std::size_t ordinal){
    const auto*const record=native_control_base_reprocess_size_record_current_owned(L,ordinal);
    char buffer[0x100]{};
    if(ordinal==16u)(void)::sprintf_s(buffer,sizeof(buffer),"CFunction: %s",record->key);
    else (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )",
        ordinal==12u?"void":(ordinal==13u?"FGXVector2":"float"),"",record->key);
    native_control_base_reprocess_size_record_current_owned(L,ordinal);
    lua_pushstring(L,buffer);native_control_base_reprocess_size_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_reprocessanchoring_signature_43f790_owned(lua_State*L){
    return native_control_base_reprocess_size_signature_owned(L,12u);
}
static int native_control_base_getsize_signature_782fb0_owned(lua_State*L){
    return native_control_base_reprocess_size_signature_owned(L,13u);
}
static int native_control_base_getsize_x_signature_69fec0_owned(lua_State*L){
    return native_control_base_reprocess_size_signature_owned(L,14u);
}
static int native_control_base_getsize_y_signature_570d40_owned(lua_State*L){
    return native_control_base_reprocess_size_signature_owned(L,15u);
}
static int native_control_base_getsize_val_signature_75b3c0_owned(lua_State*L){
    return native_control_base_reprocess_size_signature_owned(L,16u);
}
