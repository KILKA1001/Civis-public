// Inside the actual Host::Impl after Source15's current Base/CData kernels
// and Source2's real FLua argument/reference decoder. Finite rows17..20.
static const NativeContextDefinition::Record*native_control_base_setsize_record_current_owned(
    lua_State*L,std::size_t ordinal){
    native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
    struct Recipe {std::uint32_t writer,key,primary,secondary;const char*name;};
    static constexpr Recipe exact[]={
        {0x9be460u,0xa3f770u,0x676030u,0x5c07b0u,"SetSize"},
        {0x9be4c0u,0xa3f79cu,0x4bad50u,0x4c6990u,"SetSizeX"},
        {0x9be520u,0xa3f7ccu,0x57a330u,0x407840u,"SetSizeY"},
        {0x9be580u,0xa3f7fcu,0x6eee90u,0x5c70f0u,"SetSizeVal"},
    };
    if(ordinal<17||ordinal>20)
        return (luaL_error(L,"ControlBase SetSize requires actual internal ordinal17..20"),nullptr);
    const auto&proof=exact[ordinal-17];
    const auto&record=p.native_control_base_constructor->definition->records[ordinal];
    if(!record.recipe||!record.key||record.key!=record.recipe->key||
        record.recipe->writer!=proof.writer||record.recipe->key_address!=proof.key||
        std::strcmp(record.key,proof.name)||record.primary!=proof.primary||
        record.secondary!=proof.secondary||record.primary!=record.recipe->primary||
        record.secondary!=record.recipe->secondary||record.flag!=0||
        record.flag!=record.recipe->flag||record.reference!=-1)
        return (luaL_error(L,"ControlBase SetSize actual ordinal/writer/key/functions/flag recipe changed"),nullptr);
    return &record;
}

// Agreed parent physical seams, DEFINED before this INC; no accepting defs:
// static void native_control_base_setsize_member_7a5510_owned(lua_State*,const void*,const std::array<float,2>&);
// static void native_control_base_setsize_x_member_4e99c0_owned(lua_State*,const void*,float);
// static void native_control_base_setsize_y_member_68a240_owned(lua_State*,const void*,float);
// static void native_control_base_setsize_val_member_4ab4f0_owned(lua_State*,const void*,const std::array<float,2>&);
// Portable array references carry two already-converted FP32 operands. The
// original x86 vector/pair member ABI passes BOTH FP32 values by value in an
// 8byte stack block. No native pointer/reference ABI or resize implementation
// is inferred from these portable contracts. Parent owns physical partner
// reads, virtual98, cache/layout/parent effects and canonical typed receiver.

static void native_control_base_setsize_decode_current_owned(
    lua_State*L,const NativeControlBaseSetOffsetDecodeCall&r){
    native_control_base_setsize_record_current_owned(L,17u);
    // Existing PURE SAME Host/L/master/Source/CData/thread/epoch guard. It
    // grants no ordinal0 arguments, native SetOffset call or old return.
    native_control_base_setoffset_decode_full_owned(L,r);
    native_control_base_setsize_record_current_owned(L,17u);
}
static int native_control_base_setsize_primary_676030_owned(lua_State*L){
    native_control_base_setsize_record_current_owned(L,17u);
    const int count=lua_gettop(L);native_control_base_setsize_record_current_owned(L,17u);
    if(count!=2){
        (void)lua_gettop(L);native_control_base_setsize_record_current_owned(L,17u); // Actual89E996.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E9A2 ControlBase::SetSize receiver-plus-one-argument count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_setsize_record_current_owned(L,17u);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E985 ControlBase::SetSize NULL receiver");
    auto*r=native_control_base_setoffset_begin_decode_owned(L,cdata); // NEW current custody only.
    native_control_base_setsize_decode_current_owned(L,*r);
    if(r->argument.state||r->key_x.state||r->key_y.state||r->temporary.state||r->value||
        r->stage!=NativeControlBaseSetOffsetDecodeCall::Stage::entered)
        return luaL_error(L,"SetSize requires new unbound FLua argument/reference custody");
    // Exact same89E530: retain argument2; retain y then x; lazy RAW x lookup
    // and tonumber-to-FP32, then y. Original non-table input yields +0/+0.
    // No added number/bool/finite predicate or fabricated physical defaults.
    const std::array<float,2>value=native_control_base_setoffset_vector_89e530_owned(L,*r,2);
    native_control_base_setsize_decode_current_owned(L,*r);
    if(r->stage!=NativeControlBaseSetOffsetDecodeCall::Stage::released||!r->value||
        r->argument.state||r->key_x.state||r->key_y.state||r->temporary.state||lua_gettop(L)!=2)
        return luaL_error(L,"SetSize actual89E530 extraction/reference cleanup did not return");
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase SetSize requires actual Lua stack reserve32");
    native_control_base_setsize_decode_current_owned(L,*r);
    // Actual89E95D/62/6E copy x,y into the member's 8byte by-value block.
    // Every Lua extraction/conversion/ref destruction precedes the fresh
    // parent receiver/Source borrower. Locally only POD/raw identity remains.
    native_control_base_setsize_member_7a5510_owned(L,cdata,value);
    native_control_base_setsize_decode_current_owned(L,*r);return 0;
}
static int native_control_base_setsize_scalar_dispatch_89e100_owned(lua_State*L,bool x_axis){
    const std::size_t ordinal=x_axis?18u:19u;
    native_control_base_setsize_record_current_owned(L,ordinal);
    const int count=lua_gettop(L);native_control_base_setsize_record_current_owned(L,ordinal);
    if(count!=2){
        (void)lua_gettop(L);native_control_base_setsize_record_current_owned(L,ordinal); // Actual89E154.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
            x_axis?"89E160 ControlBase::SetSizeX receiver-plus-one-argument count":
                   "89E160 ControlBase::SetSizeY receiver-plus-one-argument count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_setsize_record_current_owned(L,ordinal);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
        x_axis?"89E146 ControlBase::SetSizeX NULL receiver":"89E146 ControlBase::SetSizeY NULL receiver");
    const float value=static_cast<float>(lua_tonumber(L,2)); // Exact direct89E129 ->89E12E FSTPfloat.
    native_control_base_setsize_record_current_owned(L,ordinal);
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase SetSize scalar requires actual Lua stack reserve32");
    native_control_base_setsize_record_current_owned(L,ordinal);
    if(lua_gettop(L)!=2)return luaL_error(L,"ControlBase89E100 SetSize receiver/argument stack changed");
    if(x_axis)native_control_base_setsize_x_member_4e99c0_owned(L,cdata,value);
    else native_control_base_setsize_y_member_68a240_owned(L,cdata,value);
    native_control_base_setsize_record_current_owned(L,ordinal);return 0;
}
static int native_control_base_setsize_x_primary_4bad50_owned(lua_State*L){
    return native_control_base_setsize_scalar_dispatch_89e100_owned(L,true);
}
static int native_control_base_setsize_y_primary_57a330_owned(lua_State*L){
    return native_control_base_setsize_scalar_dispatch_89e100_owned(L,false);
}
static int native_control_base_setsize_val_primary_6eee90_owned(lua_State*L){
    native_control_base_setsize_record_current_owned(L,20u);
    const int count=lua_gettop(L);native_control_base_setsize_record_current_owned(L,20u);
    if(count!=3){
        (void)lua_gettop(L);native_control_base_setsize_record_current_owned(L,20u); // Actual89E1DA.
        native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E1E6 ControlBase::SetSizeVal receiver-plus-two-argument count");
    }
    const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
    native_control_base_setsize_record_current_owned(L,20u);
    if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E1CB ControlBase::SetSizeVal NULL receiver");
    const float x=static_cast<float>(lua_tonumber(L,2));native_control_base_setsize_record_current_owned(L,20u);
    const float y=static_cast<float>(lua_tonumber(L,3));native_control_base_setsize_record_current_owned(L,20u);
    const std::array<float,2>value{x,y}; // Actual89E170: arg2 first, then arg3, no partner read.
    if(!lua_checkstack(L,32))return luaL_error(L,"ControlBase SetSizeVal requires actual Lua stack reserve32");
    native_control_base_setsize_record_current_owned(L,20u);
    if(lua_gettop(L)!=3)return luaL_error(L,"ControlBase89E170 SetSize receiver/argument stack changed");
    native_control_base_setsize_val_member_4ab4f0_owned(L,cdata,value);
    native_control_base_setsize_record_current_owned(L,20u);return 0;
}
static int native_control_base_setsize_signature_owned(lua_State*L,std::size_t ordinal){
    const auto*const record=native_control_base_setsize_record_current_owned(L,ordinal);
    char buffer[0x100]{};
    if(ordinal==20u)
        (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s )","void","",record->key,"float","","float","");
    else
        (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","",record->key,
            ordinal==17u?"FGXVector2":"float","");
    native_control_base_setsize_record_current_owned(L,ordinal);
    lua_pushstring(L,buffer);native_control_base_setsize_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_setsize_signature_5c07b0_owned(lua_State*L){
    return native_control_base_setsize_signature_owned(L,17u);
}
static int native_control_base_setsize_x_signature_4c6990_owned(lua_State*L){
    return native_control_base_setsize_signature_owned(L,18u);
}
static int native_control_base_setsize_y_signature_407840_owned(lua_State*L){
    return native_control_base_setsize_signature_owned(L,19u);
}
static int native_control_base_setsize_val_signature_5c70f0_owned(lua_State*L){
    return native_control_base_setsize_signature_owned(L,20u);
}
