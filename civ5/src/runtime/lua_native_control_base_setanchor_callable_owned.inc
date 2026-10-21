// Inside the actual Host::Impl after Source12's genuine current Base/CData
// and required normally returning diagnostic helpers. Ordinal 7 ONLY.
// Parent owns the current typed receiver borrower, 51E6A0 member, and parser.
static const NativeContextDefinition::Record*native_control_base_setanchor_record_current_owned(lua_State*L){
    native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
    const auto&record=p.native_control_base_constructor->definition->records[7];
    if(!record.recipe||!record.key||record.key!=record.recipe->key||
        record.recipe->writer!=0x9be0a0u||record.recipe->key_address!=0xa3f580u||
        std::strcmp(record.key,"SetAnchor")||record.primary!=0x5ab320u||
        record.secondary!=0x461820u||record.primary!=record.recipe->primary||
        record.secondary!=record.recipe->secondary||record.flag!=0||
        record.flag!=record.recipe->flag||record.reference!=-1)
        return (luaL_error(L,"ControlBase SetAnchor actual ordinal7/writer/key/function recipe changed"),nullptr);
    return &record;
}

// Agreed parent seam, defined before this INC:
// static void native_control_base_setanchor_member_51e6a0_owned(
//     lua_State*,const void* resolved_cdata,const char* borrowed_lua_string);
// Its current physical receiver/Source borrower is genuine and synchronous.
// The string is rooted by Lua argument 2; NULL is an actual conversion result.
// No strong C++ owner may survive this seam into the next Lua API or error.
static int native_control_base_setanchor_dispatch_89e260_owned(lua_State*L){
    native_control_base_setanchor_record_current_owned(L);
    const int count=lua_gettop(L);native_control_base_setanchor_record_current_owned(L);
    if(count!=2){
        (void)lua_gettop(L);native_control_base_setanchor_record_current_owned(L); // Actual89E2B3.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
            "89E2BF ControlBase::SetAnchor exact receiver-plus-one-argument count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_setanchor_record_current_owned(L);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
        "89E2A5 ControlBase::SetAnchor NULL receiver");
    // Actual89E28B: lua_tolstring(L,2,NULL). No luaL_checkstring/type predicate,
    // __tostring call, length, registry reference, or empty-string substitution.
    // A numeric argument is converted in its rooted slot; other non-strings
    // yield NULL, which reaches the same actual member seam unchanged.
    const char*const text=lua_tolstring(L,2,nullptr);
    native_control_base_setanchor_record_current_owned(L);
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase SetAnchor requires actual Lua stack reserve32");
    native_control_base_setanchor_record_current_owned(L);
    if(lua_gettop(L)!=2)return luaL_error(L,"ControlBase89E260 receiver/argument stack changed");
    // Only opaque receiver identity and the rooted string pointer cross this
    // synchronous call; actual member/parser effects are owned by the parent.
    native_control_base_setanchor_member_51e6a0_owned(L,cdata,text);
    native_control_base_setanchor_record_current_owned(L);return 0;
}
static int native_control_base_setanchor_primary_5ab320_owned(lua_State*L){
    return native_control_base_setanchor_dispatch_89e260_owned(L);
}
static int native_control_base_setanchor_signature_461820_owned(lua_State*L){
    native_control_base_setanchor_record_current_owned(L);
    char buffer[0x100]{};
    // Actual46184E sprintf_s256; exact five string operands and format.
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","SetAnchor","const char","*");
    native_control_base_setanchor_record_current_owned(L);
    lua_pushstring(L,buffer);native_control_base_setanchor_record_current_owned(L);return 1;
}
