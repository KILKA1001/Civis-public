//Inside the actual Host::Impl, after the owned decoder. Parent owns the
//genuine Host/UI member seam below and its guarded canonical receiver actor:
//static void native_control_base_setoffset_member_4c8fe0_owned(
//    lua_State*,const void*resolved_cdata,const std::array<float,2>&);
//It must return only after actual SAME4C8FE0 equality return OR complete
//FP32/floor/current stores and actual virtual4C then94. No receipt/bool grant.
static int native_control_base_setoffset_primary_4e4eb0_owned(lua_State*L){
    native_control_base_setoffset_record_current_owned(L);
    const int count=lua_gettop(L);native_control_base_setoffset_record_current_owned(L);
    if(count!=2){
        (void)lua_gettop(L);native_control_base_setoffset_record_current_owned(L); //Actual89E996 repeat.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E9A2 exact receiver-plus-one-argument count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    if(!cdata)
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E985 Call cannot be made on NULL");
    auto*r=native_control_base_setoffset_begin_decode_owned(L,cdata);
    const std::array<float,2>value=native_control_base_setoffset_vector_89e530_owned(L,*r,2);
    native_control_base_setoffset_decode_full_owned(L,*r);
    if(r->stage!=NativeControlBaseSetOffsetDecodeCall::Stage::released||!r->value||lua_gettop(L)!=2)
        return luaL_error(L,"SetOffset original vector extraction/reference cleanup did not return");
    //Parent's nonalloc physical Controls/metatype scan requires highwater32.
    //Reserve before acquiring any C++ receiver/source/operation borrower.
    if(!lua_checkstack(L,32))return luaL_error(L,"SetOffset canonical receiver scan requires actual Lua stack reserve32");
    native_control_base_setoffset_decode_full_owned(L,*r);
    //ALL argument extraction/conversion Lua APIs and original reference
    //destruction are finished. Only POD/raw identity operands live locally.
    //Parent's seam freshly binds CData to its SAME retained current typed
    //NativeRootOrdinaryControlsResult; CData is never cast into a receiver.
    native_control_base_setoffset_member_4c8fe0_owned(L,cdata,value);
    native_control_base_setoffset_decode_full_owned(L,*r);
    r->stage=NativeControlBaseSetOffsetDecodeCall::Stage::member_returned;
    return 0; //Actual89E977/79 successful member-return branch.
}
