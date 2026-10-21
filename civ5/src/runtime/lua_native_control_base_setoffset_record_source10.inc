//Inside the SAME Host::Impl, after Source9's actual ControlBase constructor.
//Selected PE primary4E4EB0 ->89E920/89E050/89E530 ->member4C8FE0.
//This successor supplies the exact record/signature. Source10 owns the
//genuine callback-time geometry supplier consumed by the child callable.
static const NativeContextDefinition::Record*
native_control_base_setoffset_record_current_owned(lua_State*L){
    auto&p=owner(L);native_ordinary_metatype_full(L);
    const auto*c=p.native_control_base_constructor.get();
    if(!c)return (luaL_error(L,"SetOffset requires the SAME actual ControlBase constructor"),nullptr);
    if(!c->definition)return (luaL_error(L,"SetOffset real constructor definition missing"),nullptr);
    native_control_base_current_definition_read_owned(L,*c->definition);
    if(c->host!=&p||c->queue!=p.native_definition_deferred_queue||
        c->definition!=p.native_context_definitions[1]||
        !c->canonical||c->canonical!=p.native_ordinary_nested_descriptors[0]||
        !c->canonical->alive.load()||c->canonical->host!=&p||
        c->canonical->creator!=std::this_thread::get_id()||
        c->canonical->kind!=NativeOrdinaryNestedDescriptorReturn::Kind::descriptor_cb7c68||
        c->canonical->actual_descriptor!=0xcb7c68u||c->canonical->actual_ctor!=0x5210f0u||
        c->canonical->actual_name0!="ControlBase")
        return (luaL_error(L,"SetOffset actual constructor/definition/queue/canonical lineage changed"),nullptr);
    const auto*d=c->definition.get();
    const auto&r=d->records[0];
    if(d->count544!=49u||!r.recipe||!r.key||r.recipe->writer!=0x9bddf0u||
        r.recipe->key_address!=0xa3f418u||r.key!=r.recipe->key||
        std::strcmp(r.key,"SetOffset")||r.primary!=0x4e4eb0u||
        r.secondary!=0x43c790u||r.flag!=0||r.reference!=-1||
        r.primary!=r.recipe->primary||r.secondary!=r.recipe->secondary||r.flag!=r.recipe->flag)
        return (luaL_error(L,"SetOffset exact first9BDDF0 record changed"),nullptr);
    return &r;
}

static int native_control_base_setoffset_signature_43c790_owned(lua_State*L){
    //Complete straight-line43C790..43C7E2; actual MSVCR90!sprintf_s via
    //IAT9C9680, scratch256, ignored formatter return, pushstring, return1.
    //The original ignores its arguments, including CodeBuddy's primary.
    //Use the existing generic SAME VM/Source fence, which also supports the
    //current root. This callback does not issue Bootstrap completion.
    native_control_base_setoffset_record_current_owned(L);
    char buffer[0x100]{};
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )",
        "void","","SetOffset","FGXVector2","");
    native_control_base_setoffset_record_current_owned(L);
    lua_pushstring(L,buffer);
    native_control_base_setoffset_record_current_owned(L);
    return 1;
}

