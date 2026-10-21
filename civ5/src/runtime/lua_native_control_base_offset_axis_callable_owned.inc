//Inside the actual Host::Impl, after Source10's genuine generic current
//ControlBase record/CData extractor/returning-diagnostic requirement.
//Only the proved contiguous ordinals1..3 are ported here. Parent owns the
//Host borrower, UI69CD90 actor and47C500 library consumer.
static const NativeContextDefinition::Record*native_control_base_offset_record_current_owned(
    lua_State*L,std::size_t ordinal){
    native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
    struct Recipe {std::uint32_t writer,key,primary,secondary;const char*name;};
    static constexpr Recipe exact[]={
        {0x9bde50u,0xa3f448u,0x6c44b0u,0x49d160u,"SetOffsetX"},
        {0x9bdeb0u,0xa3f478u,0x410610u,0x5775b0u,"SetOffsetY"},
        {0x9bdf10u,0xa3f4b0u,0x688850u,0x53f760u,"SetOffsetVal"},
    };
    if(ordinal<1||ordinal>3)
        return (luaL_error(L,"ControlBase offset callback requires its actual internal ordinal1..3"),nullptr);
    const auto&proof=exact[ordinal-1];
    const auto&record=p.native_control_base_constructor->definition->records[ordinal];
    if(!record.recipe||!record.key||record.key!=record.recipe->key||
        record.recipe->writer!=proof.writer||record.recipe->key_address!=proof.key||
        std::strcmp(record.key,proof.name)||record.primary!=proof.primary||
        record.secondary!=proof.secondary||record.primary!=record.recipe->primary||
        record.secondary!=record.recipe->secondary||record.flag!=0||
        record.flag!=record.recipe->flag||record.reference!=-1)
        return (luaL_error(L,"ControlBase offset actual ordinal/writer/key/function recipe changed"),nullptr);
    return &record;
}

//Parent supplies two genuine typed operation seams; no Host fields are added:
//static void native_control_base_offset_axis_member_owned(
//    lua_State*,const void*resolved_cdata,bool x_axis,float value);
//static void native_control_base_offset_pair_69cd90_owned(
//    lua_State*,const void*resolved_cdata,const std::array<float,2>& value);
//x_axis is ONLY the internal identity of two actual named native callbacks.
//X4A8380 reads current74 before incomingX; Y6158F0 reads incomingY before
//current70. Both pass FP32 pairs to DISTINCT69CD90, never vector4C8FE0.
//69CD90 floors/stores70, floors/stores74, RELOADS70/74 for20/24 extents,
//then actual SAME receiver4C and94. Genuine unchanged comparison returns
//before all stores/virtual calls. Required nearest-callee STOPs propagate.
static int native_control_base_offset_scalar_dispatch_89e100_owned(lua_State*L,bool x_axis){
    const std::size_t ordinal=x_axis?1u:2u;
    native_control_base_offset_record_current_owned(L,ordinal);
    const int count=lua_gettop(L);native_control_base_offset_record_current_owned(L,ordinal);
    if(count!=2){
        (void)lua_gettop(L);native_control_base_offset_record_current_owned(L,ordinal); //Actual89E154.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
            x_axis?"89E160 ControlBase::SetOffsetX exact receiver-plus-one-argument count":
                   "89E160 ControlBase::SetOffsetY exact receiver-plus-one-argument count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_offset_record_current_owned(L,ordinal);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
        x_axis?"89E146 ControlBase::SetOffsetX NULL receiver":"89E146 ControlBase::SetOffsetY NULL receiver");
    //Actual89E129 lua_tonumber(L,2),89E12E FSTPfloat. There is NO FLua
    //reference/vector decoder or added bool/type/finite check in this ABI.
    const float value=static_cast<float>(lua_tonumber(L,2));
    native_control_base_offset_record_current_owned(L,ordinal);
    //Reserve before the parent's physical Controls scan/strong C++ borrowers.
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase offset scalar requires actual Lua stack reserve32");
    native_control_base_offset_record_current_owned(L,ordinal);
    if(lua_gettop(L)!=2)return luaL_error(L,"ControlBase89E100 receiver/conversion stack changed");
    native_control_base_offset_axis_member_owned(L,cdata,x_axis,value);
    native_control_base_offset_record_current_owned(L,ordinal);
    return 0; //Actual89E13C after genuine member return, no readiness grant.
}
static int native_control_base_setoffset_x_primary_6c44b0_owned(lua_State*L){
    return native_control_base_offset_scalar_dispatch_89e100_owned(L,true);
}
static int native_control_base_setoffset_y_primary_410610_owned(lua_State*L){
    return native_control_base_offset_scalar_dispatch_89e100_owned(L,false);
}
static int native_control_base_setoffset_val_primary_688850_owned(lua_State*L){
    native_control_base_offset_record_current_owned(L,3u);
    const int count=lua_gettop(L);native_control_base_offset_record_current_owned(L,3u);
    if(count!=3){
        (void)lua_gettop(L);native_control_base_offset_record_current_owned(L,3u); //Actual89E1DA.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
            "89E1E6 ControlBase::SetOffsetVal exact receiver-plus-two-arguments count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_offset_record_current_owned(L,3u);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
        "89E1CB ControlBase::SetOffsetVal NULL receiver");
    //Actual89E19A/19F converts arg2 to FP32 BEFORE89E1A6/1AB converts arg3.
    //Direct Lua conversion preserves numeric strings and nonnumeric->0.
    const float x=static_cast<float>(lua_tonumber(L,2));native_control_base_offset_record_current_owned(L,3u);
    const float y=static_cast<float>(lua_tonumber(L,3));native_control_base_offset_record_current_owned(L,3u);
    const std::array<float,2>value{x,y}; //POD only; no argument registry refs.
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase offset pair requires actual Lua stack reserve32");
    native_control_base_offset_record_current_owned(L,3u);
    if(lua_gettop(L)!=3)return luaL_error(L,"ControlBase89E170 receiver/conversion stack changed");
    native_control_base_offset_pair_69cd90_owned(L,cdata,value);
    native_control_base_offset_record_current_owned(L,3u);
    return 0; //Actual89E1C0 after the real distinct69CD90 return.
}

static int native_control_base_setoffset_x_signature_49d160_owned(lua_State*L){
    native_control_base_offset_record_current_owned(L,1u);
    char buffer[0x100]{};
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","SetOffsetX","float","");
    native_control_base_offset_record_current_owned(L,1u);
    lua_pushstring(L,buffer);native_control_base_offset_record_current_owned(L,1u);return 1;
}
static int native_control_base_setoffset_y_signature_5775b0_owned(lua_State*L){
    native_control_base_offset_record_current_owned(L,2u);
    char buffer[0x100]{};
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","SetOffsetY","float","");
    native_control_base_offset_record_current_owned(L,2u);
    lua_pushstring(L,buffer);native_control_base_offset_record_current_owned(L,2u);return 1;
}
static int native_control_base_setoffset_val_signature_53f760_owned(lua_State*L){
    native_control_base_offset_record_current_owned(L,3u);
    char buffer[0x100]{};
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s )",
        "void","","SetOffsetVal","float","","float","");
    native_control_base_offset_record_current_owned(L,3u);
    lua_pushstring(L,buffer);native_control_base_offset_record_current_owned(L,3u);return 1;
}
