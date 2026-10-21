//Inside the actual Host::Impl after Source10/11 generic current Base/CData
//and required returning diagnostic helpers. Parent owns Host/UI/47 consumer.
//Finite ordinals4..6 ONLY; SetAnchor ordinal7 is unresearched/unissued.
static const NativeContextDefinition::Record*native_control_base_offset_getter_record_current_owned(
    lua_State*L,std::size_t ordinal){
    native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
    struct Recipe {std::uint32_t writer,key,primary,secondary;const char*name;};
    static constexpr Recipe exact[]={
        {0x9bdf70u,0xa3f4e4u,0x51a6a0u,0x581a50u,"GetOffsetX"},
        {0x9bdfe0u,0xa3f514u,0x6bb2e0u,0x5b2660u,"GetOffsetY"},
        {0x9be040u,0xa3f54cu,0x6cba70u,0x4161c0u,"GetOffsetVal"},
    };
    if(ordinal<4||ordinal>6)
        return (luaL_error(L,"ControlBase offset getter requires actual internal ordinal4..6"),nullptr);
    const auto&proof=exact[ordinal-4];
    const auto&record=p.native_control_base_constructor->definition->records[ordinal];
    if(!record.recipe||!record.key||record.key!=record.recipe->key||
        record.recipe->writer!=proof.writer||record.recipe->key_address!=proof.key||
        std::strcmp(record.key,proof.name)||record.primary!=proof.primary||
        record.secondary!=proof.secondary||record.primary!=record.recipe->primary||
        record.secondary!=record.recipe->secondary||record.flag!=0||
        record.flag!=record.recipe->flag||record.reference!=-1)
        return (luaL_error(L,"ControlBase offset getter actual ordinal/writer/key/function recipe changed"),nullptr);
    return &record;
}

//Parent supplies three typed current read seams. Each returns only a POD
//float after ALL strong C++ receiver/tree/Source borrowers are destroyed:
//static float native_control_base_getoffset_x_b0_owned(lua_State*,const void*);
//static float native_control_base_getoffset_y_b4_owned(lua_State*,const void*);
//static float native_control_base_getoffset_val_axis_owned(lua_State*,const void*,bool x_axis);
//X5AF940/Y58D230 tail-dispatch actualB0/B4. Selected SIX proved subtypes
//have nativeB0=5B5B50(FLD70;RET),B4=749A00(FLD74;RET). Parent must select
//the genuine current subtype; unqualified virtual targets remain REQUIRED.
//Val6486F0 reads raw70 then74 without virtual dispatch. Bool is only the
//internal raw-axis identity; no pointer/ID/default/cached-pair authority.
static int native_control_base_getoffset_scalar_dispatch_89e200_owned(lua_State*L,bool x_axis){
    const std::size_t ordinal=x_axis?4u:5u;
    native_control_base_offset_getter_record_current_owned(L,ordinal);
    const int count=lua_gettop(L);native_control_base_offset_getter_record_current_owned(L,ordinal);
    if(count!=1){
        (void)lua_gettop(L);native_control_base_offset_getter_record_current_owned(L,ordinal); //Actual89E246.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
            x_axis?"89E252 ControlBase::GetOffsetX exact receiver-only count":
                   "89E252 ControlBase::GetOffsetY exact receiver-only count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_offset_getter_record_current_owned(L,ordinal);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
        x_axis?"89E23A ControlBase::GetOffsetX NULL receiver":"89E23A ControlBase::GetOffsetY NULL receiver");
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase offset getter requires actual Lua stack reserve32");
    native_control_base_offset_getter_record_current_owned(L,ordinal);
    if(lua_gettop(L)!=1)return luaL_error(L,"ControlBase89E200 receiver stack changed");
    const float value=x_axis?native_control_base_getoffset_x_b0_owned(L,cdata):
                              native_control_base_getoffset_y_b4_owned(L,cdata);
    native_control_base_offset_getter_record_current_owned(L,ordinal);
    //Actual member's FP32 x87 return is expanded by89E222 FSTPdouble before
    //89E226 lua_pushnumber. No integer conversion, clamp or finite rule.
    lua_pushnumber(L,static_cast<lua_Number>(value));
    native_control_base_offset_getter_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_getoffset_x_primary_51a6a0_owned(lua_State*L){
    return native_control_base_getoffset_scalar_dispatch_89e200_owned(L,true);
}
static int native_control_base_getoffset_y_primary_6bb2e0_owned(lua_State*L){
    return native_control_base_getoffset_scalar_dispatch_89e200_owned(L,false);
}
static int native_control_base_getoffset_val_primary_6cba70_owned(lua_State*L){
    native_control_base_offset_getter_record_current_owned(L,6u);
    //Actual6CBA70 is a5-byte JMP6486F0.8F77F0(L,1) creates ONLY a POD
    //{state,index1} view; positive1 causes no API/ref/count check. Preserve
    //that ABI: extra arguments are ignored; no scalar receiver-only check.
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_offset_getter_record_current_owned(L,6u);
    if(!cdata)return 0; //Actual648743, NO added89E200 NULL diagnostic.
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase offset pair getter requires actual Lua stack reserve32");
    native_control_base_offset_getter_record_current_owned(L,6u);
    //No live C++ borrower or native FP32-pair pointer crosses Lua. The
    //opaque CData identity is reborrowed fresh by EACH parent read seam.
    const float x=native_control_base_getoffset_val_axis_owned(L,cdata,true); //Actual648717 READ70.
    native_control_base_offset_getter_record_current_owned(L,6u);
    lua_pushnumber(L,static_cast<lua_Number>(x));native_control_base_offset_getter_record_current_owned(L,6u); //Actual648721.
    const float y=native_control_base_getoffset_val_axis_owned(L,cdata,false); //Actual648726 fresh READ74 AFTER pushX.
    native_control_base_offset_getter_record_current_owned(L,6u);
    lua_pushnumber(L,static_cast<lua_Number>(y));native_control_base_offset_getter_record_current_owned(L,6u); //Actual648730.
    return 2; //Actual648739; real two Lua numbers, no table or registry refs.
}

static int native_control_base_getoffset_x_signature_581a50_owned(lua_State*L){
    native_control_base_offset_getter_record_current_owned(L,4u);
    char buffer[0x100]{};
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","float","","GetOffsetX");
    native_control_base_offset_getter_record_current_owned(L,4u);
    lua_pushstring(L,buffer);native_control_base_offset_getter_record_current_owned(L,4u);return 1;
}
static int native_control_base_getoffset_y_signature_5b2660_owned(lua_State*L){
    native_control_base_offset_getter_record_current_owned(L,5u);
    char buffer[0x100]{};
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","float","","GetOffsetY");
    native_control_base_offset_getter_record_current_owned(L,5u);
    lua_pushstring(L,buffer);native_control_base_offset_getter_record_current_owned(L,5u);return 1;
}
static int native_control_base_getoffset_val_signature_4161c0_owned(lua_State*L){
    native_control_base_offset_getter_record_current_owned(L,6u);
    char buffer[0x100]{};
    //Actual4161DA format is CFunction:%s, not an inferred typed-pair signature.
    (void)::sprintf_s(buffer,sizeof(buffer),"CFunction: %s","GetOffsetVal");
    native_control_base_offset_getter_record_current_owned(L,6u);
    lua_pushstring(L,buffer);native_control_base_offset_getter_record_current_owned(L,6u);return 1;
}
