//Inside Host::Impl AFTER actual child Context creator and common615D70 body.
//Current root instance is distinct from child-Controls instances. Reuse the
//genuine static type owner and common executing615D70, never child return/frame.
struct NativePostreadRootCDataControl final {
    enum class Stage {captured,keys,table,field,master,metatype,assigned,returned,cleanup,retired,failed};
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsReference> globals;
    const std::shared_ptr<NativePostreadGlobalsControl> parent;
    const std::weak_ptr<ui::detail::NotificationLayoutContextOrdinaryState> root;
    const std::shared_ptr<const NativeContextCDataCreator> type;
    const void*const cdata;
    Stage stage=Stage::captured;
    int keys_ref=LUA_NOREF,table_ref=LUA_NOREF,metatype_ref=LUA_NOREF;
    const void*keys=nullptr,*table=nullptr,*metatype=nullptr;
    NativeOrdinaryMasterTypesResult* master=nullptr;
    bool unref_started=false,unref_returned=false,closed=false;
    int entry_top=0,returned_top=0;
    std::weak_ptr<const ui::detail::NotificationContextPostreadRootCDataReturn> canonical;
    NativePostreadRootCDataControl(std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsReference>g,
        std::shared_ptr<NativePostreadGlobalsControl>p,std::weak_ptr<ui::detail::NotificationLayoutContextOrdinaryState>r,
        std::shared_ptr<const NativeContextCDataCreator>t,const void*ptr)
        :globals(std::move(g)),parent(std::move(p)),root(std::move(r)),type(std::move(t)),cdata(ptr){}
};
struct NativePostreadRootCDataFrame final {
    Impl&host;const LuaScriptHost&facade;const BnwWorldViewSourceOperationLoan&operation;
    NativePostreadRootCDataControl&control;
    //Physical current root custody survives all allocating APIs in cpcall.
    const std::shared_ptr<ui::detail::NotificationLayoutContextOrdinaryState> held_root;
    const std::shared_ptr<const ui::detail::NotificationContextDirectLuaTailInput> direct_input;
    NativePostreadRootCDataFrame(Impl&p,const LuaScriptHost&f,const BnwWorldViewSourceOperationLoan&op,
        NativePostreadRootCDataControl&r,std::shared_ptr<ui::detail::NotificationLayoutContextOrdinaryState>root,
        std::shared_ptr<const ui::detail::NotificationContextDirectLuaTailInput>input={})
        :host(p),facade(f),operation(op),control(r),held_root(std::move(root)),direct_input(std::move(input)){
        if(host.postread_root_cdata_frame)throw ScriptError("postRead root CData entered twice");
        host.postread_root_cdata_frame=this;
    }
    NativePostreadRootCDataFrame(const NativePostreadRootCDataFrame&)=delete;
    ~NativePostreadRootCDataFrame(){
        if(control.creator!=std::this_thread::get_id())std::terminate();
        if(host.postread_root_cdata_frame==this)host.postread_root_cdata_frame=nullptr;else host.record_failure();
    }
    void first()const {
        if(direct_input){host.native_direct_root_cdata_first_owned(*this);return;}
        host.native_postread_globals_first(*control.parent); //Host/Source/facade FIRST
        facade.require_context_postread_globals_input_without_callbacks(operation,control.parent->input);
        if(host.postread_root_cdata_frame!=this||control.closed||control.creator!=std::this_thread::get_id()||
            !held_root||control.root.lock()!=held_root||held_root.get()!=control.cdata||
            control.parent->phase!=NativePostreadGlobalsControl::Phase::prefix_returned||
            !control.type||control.type!=host.native_context_cdata_creator||control.type->host!=&host||
            !control.type->alive.load()||control.type->creator!=control.creator||
            std::strcmp(control.type->type_name,"LuaContext")||
            control.type->push_metatype!=&Impl::native_child_context_push_existing_metatable)
            throw ScriptError("postRead root Context actual instance/creator/parent current binding changed");
    }
    void full()const {
        if(direct_input){host.native_direct_root_cdata_full_owned(*this);return;}
        first();facade.require_context_postread_globals_input_owned(operation,control.parent->input);first();
    }
};
static void native_postread_root_cdata_full(lua_State*L){
    auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{
        if(!p.postread_root_cdata_frame)throw ScriptError("postRead root Context reflection frame missing");
        p.postread_root_cdata_frame->full();
    }catch(...){p.source_exception=std::current_exception();p.source_error="postRead root Context reflection Source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static bool native_postread_root_cdata_physical(lua_State*L,const NativePostreadRootCDataControl&r){
    auto&p=owner(L);if(!L||r.closed||r.keys_ref<0||r.table_ref<0||r.metatype_ref<0||!r.master||
        !r.parent||L!=r.parent->thread||L->ci->top-L->top<8)return false;
    const int top=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.keys_ref);const int keys=lua_gettop(L);
    if(lua_type(L,keys)!=LUA_TTABLE||lua_topointer(L,keys)!=r.keys){lua_settop(L,top);return false;}
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.table_ref);const int table=lua_gettop(L);
    bool same=lua_type(L,table)==LUA_TTABLE&&lua_topointer(L,table)==r.table;
    if(!same){lua_settop(L,top);return false;} //rawget requires a proved actual table
    lua_rawgeti(L,keys,1);lua_rawget(L,table);
    same=same&&lua_type(L,-1)==LUA_TLIGHTUSERDATA&&lua_touserdata(L,-1)==r.cdata;lua_pop(L,1);
    if(!lua_getmetatable(L,table)){lua_settop(L,top);return false;}
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.metatype_ref);
    same=same&&lua_type(L,-1)==LUA_TTABLE&&lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==r.metatype;lua_pop(L,2);
    lua_rawgeti(L,keys,2);lua_rawget(L,LUA_GLOBALSINDEX);
    same=same&&lua_type(L,-1)==LUA_TTABLE&&lua_rawequal(L,-1,table);lua_pop(L,1);
    lua_rawgeti(L,keys,3);lua_rawget(L,LUA_GLOBALSINDEX);const int cache=lua_gettop(L);
    const bool retained_master=std::any_of(p.native_ordinary_master_types_results.begin(),p.native_ordinary_master_types_results.end(),
        [&](const auto&keeper){return keeper&&keeper.get()==r.master&&keeper->host==&p&&keeper->alive.load()&&
            keeper->stage==NativeOrdinaryMasterTypesResult::Stage::returned;});
    if(!retained_master){lua_settop(L,top);return false;}
    same=same&&native_ordinary_master_physical_without_callbacks(p,L,*r.master,cache);
    if(!same){lua_settop(L,top);return false;}
    lua_rawgeti(L,keys,4);lua_rawget(L,cache);lua_rawgeti(L,LUA_REGISTRYINDEX,r.metatype_ref);
    same=same&&lua_type(L,-1)==LUA_TTABLE&&lua_rawequal(L,-1,-2);
    lua_settop(L,top);return same;
}
static void native_postread_root_metatype_43f610_owned(lua_State*L){
    auto&p=owner(L);native_postread_root_cdata_full(L);
    auto*r=p.postread_root_cdata_frame->control.parent?&p.postread_root_cdata_frame->control:nullptr;
    if(!r)return (void)luaL_error(L,"postRead current Context root receiver absent");
    //Own root wrapper shares the authentic type/name issuer, not the child
    //callback's receiver frame. Original lazy static initializer already ran
    //in this SAME Host ctor. No original global address is invoked/cast.
    native_ordinary_push_master_types_615d70_owned(L);native_postread_root_cdata_full(L);
    r->master=nullptr;
    for(const auto&keeper:p.native_ordinary_master_types_results)
        if(keeper&&keeper->cache==lua_topointer(L,-1)&&native_ordinary_master_physical_without_callbacks(p,L,*keeper,-1)){
            r->master=keeper.get();break;
        }
    if(!r->master)return (void)luaL_error(L,"postRead43F610 requires actual closed615D70 master owner");
    r->stage=NativePostreadRootCDataControl::Stage::master;
    lua_getfield(L,-1,r->type->type_name);native_postread_root_cdata_full(L);
    if(lua_type(L,-1)!=LUA_TTABLE){
        //Exact43F659 settop(-3) removes both lookup and master. The next
        //43F667 calls517C20 with ECX=actualCAAEB0 Context descriptor.
        lua_settop(L,-3);native_postread_root_cdata_full(L);
        r->master=native_context_construct_root_metatype_and_borrow_master_owned(L);
        native_postread_root_cdata_full(L);
        if(!r->master)return (void)luaL_error(L,"postRead actual Context517C whole return missing");
    }else {
        lua_remove(L,-2);native_postread_root_cdata_full(L); //exact43F671 HIT only
    }
    r->metatype=lua_topointer(L,-1);lua_pushvalue(L,-1);
    r->metatype_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_postread_root_cdata_full(L);
    if(r->metatype_ref<0)return (void)luaL_error(L,"postRead actual Context cache-HIT metatype retention failed");
    //The HIT already removed its cache; MISS517C returned only its metatype.
    r->stage=NativePostreadRootCDataControl::Stage::metatype;
}
static int native_postread_root_cdata_coroutine_body(lua_State*L){
    auto*job=static_cast<Job*>(lua_touserdata(L,1));auto&p=owner(L);
    auto*r=job?job->context_postread_root_cdata:nullptr;
    if(!job||job->owner!=&p||!r||!r->parent||L!=r->parent->thread||lua_gettop(L)!=1)
        return luaL_error(L,"postRead root CData physical Job/coroutine differs");
    if(!lua_checkstack(L,32))return luaL_error(L,"postRead root CData actual stack reserve failed");
    native_postread_root_cdata_full(L);
    if(!native_postread_globals_physical(L,*r->parent,true))return luaL_error(L,"postRead root CData genuine Globals/key refs changed");
    if(job->operation==Operation::native_context_postread_root_cdata_query){
        if(r->stage!=NativePostreadRootCDataControl::Stage::returned||!native_postread_root_cdata_physical(L,*r))
            return luaL_error(L,"postRead current root CData publication/metatype physical POST changed");
        native_postread_root_cdata_full(L);return 0;
    }
    if(job->operation!=Operation::native_context_postread_root_cdata_create||r->stage!=NativePostreadRootCDataControl::Stage::captured)
        return luaL_error(L,"postRead root Context creator replay");
    //Target-only pinned TString keeper for subsequent callback-free physical
    //queries, allocated under this actual cpcall before native semantic body.
    lua_createtable(L,4,0);
    const char*names[]={"CData","ContextPtr","ExposedTypes",r->type->type_name};
    for(int i=0;i<4;++i){lua_pushstring(L,names[i]);lua_rawseti(L,-2,i+1);}
    r->keys=lua_topointer(L,-1);r->keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    r->stage=NativePostreadRootCDataControl::Stage::keys;native_postread_root_cdata_full(L);
    //42F068 actual SAME state was proven, so no mismatch diagnostic branch.
    if(r->parent->globals_state!=L||r->parent->key_state!=L)
        return luaL_error(L,"postRead42F060 real local states differ");
    if(!lua_checkstack(L,1))return luaL_error(L,"postRead Globals PushTable stack reserve failed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->parent->globals_reference);native_postread_root_cdata_full(L);
    if(lua_type(L,-1)!=LUA_TTABLE)return luaL_error(L,"postRead8F88F0 PushTable returned non-table");
    if(!lua_checkstack(L,1))return luaL_error(L,"postRead key Push stack reserve failed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->parent->key_reference);native_postread_root_cdata_full(L);
    if(lua_type(L,-1)==LUA_TNIL)return luaL_error(L,"postRead42F099 key type reaches mandatory diagnostic branch");
    if(!lua_checkstack(L,1))return luaL_error(L,"postRead actual897FF0 output stack reserve failed");
    lua_createtable(L,0,0);r->table=lua_topointer(L,-1);r->stage=NativePostreadRootCDataControl::Stage::table;
    native_postread_root_cdata_full(L);
    lua_pushlightuserdata(L,const_cast<void*>(r->cdata)); //actual mapped SAME current root owner; no32-bit cast
    lua_setfield(L,-2,"CData");r->stage=NativePostreadRootCDataControl::Stage::field;native_postread_root_cdata_full(L);
    lua_pushvalue(L,-1);r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_postread_root_cdata_full(L);
    if(r->table_ref<0)return luaL_error(L,"postRead actual root reflection table keeper failed");
    //Actual nonNULL creator belongs to genuine executed Host initialization.
    native_postread_root_metatype_43f610_owned(L);native_postread_root_cdata_full(L);
    lua_setmetatable(L,-2);native_postread_root_cdata_full(L); //actual898024, ignoredBOOL captured by real API effect
    lua_settable(L,-3);r->stage=NativePostreadRootCDataControl::Stage::assigned;native_postread_root_cdata_full(L);
    lua_settop(L,-2); //42F0EA removes actual Globals after key/value assignment
    if(lua_gettop(L)!=1||!native_postread_root_cdata_physical(L,*r))
        return luaL_error(L,"postRead actual42F060 Globals[ContextPtr]/CData/metatype return absent");
    native_postread_root_cdata_full(L);r->stage=NativePostreadRootCDataControl::Stage::returned;return 0;
}
static int native_postread_root_cdata_coroutine(lua_State*L){
    auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
    //Existing615D70 owns std allocations as well as Lua allocations. Convert
    //any C++ failure before returning through the child's Lua C frames, so
    //its actual luaD_pcall can restore CI/top. No owner survives luaL_error.
    try{return native_postread_root_cdata_coroutine_body(L);}
    catch(...){p.source_exception=std::current_exception();p.source_error="postRead root CData required C++ supplier failed";}
    p.record_failure();return luaL_error(L,"%s",p.source_error);
}
static void perform_native_postread_root_cdata(lua_State*L,Job&job){
    auto&p=owner(L);auto*r=job.context_postread_root_cdata;
    if(!r||!r->parent||L!=p.state)return (void)luaL_error(L,"postRead root CData requires actual Host master");
    native_postread_root_cdata_full(L);
    if(!native_postread_globals_retained_thread_without_callbacks(p,*r->parent))
        return (void)luaL_error(L,"postRead root CData actual thread keeper retired before pointer borrow");
    lua_State*thread=r->parent->thread;
    if(lua_status(thread)||lua_gettop(thread)!=0)return (void)luaL_error(L,"postRead root CData actual coroutine not idle");
    r->entry_top=lua_gettop(thread);
    const int status=lua_cpcall(thread,native_postread_root_cdata_coroutine,&job);
    if(status){
        if(!lua_checkstack(L,1))return (void)luaL_error(L,"postRead root CData error transfer stack reserve failed");
        lua_xmove(thread,L,1);lua_settop(thread,r->entry_top);return (void)lua_error(L);
    }
    r->returned_top=lua_gettop(thread);
    if(r->returned_top!=r->entry_top||r->stage!=NativePostreadRootCDataControl::Stage::returned)
        return (void)luaL_error(L,"postRead root CData coroutine/caller did not return");
    native_postread_root_cdata_full(L);
}
static int native_postread_root_cdata_cleanup(lua_State*L){
    auto*r=static_cast<NativePostreadRootCDataControl*>(lua_touserdata(L,1));
    if(!r||!r->parent||L!=r->parent->thread||r->creator!=std::this_thread::get_id())
        return luaL_error(L,"postRead root reflection cleanup owner differs");
    if(!lua_checkstack(L,4))return luaL_error(L,"postRead root reflection cleanup stack reserve failed");
    r->stage=NativePostreadRootCDataControl::Stage::cleanup;r->unref_started=true;
    if(r->metatype_ref!=LUA_NOREF){luaL_unref(L,LUA_REGISTRYINDEX,r->metatype_ref);r->metatype_ref=LUA_NOREF;}
    if(r->table_ref!=LUA_NOREF){luaL_unref(L,LUA_REGISTRYINDEX,r->table_ref);r->table_ref=LUA_NOREF;}
    if(r->keys_ref!=LUA_NOREF){luaL_unref(L,LUA_REGISTRYINDEX,r->keys_ref);r->keys_ref=LUA_NOREF;}
    r->unref_returned=true;return 0;
}
void native_postread_root_cdata_release_after_abort(NativePostreadRootCDataControl&r)noexcept{
    if(r.creator!=std::this_thread::get_id()||r.creator!=ui_thread)std::terminate();
    if(r.closed||!r.parent||!native_postread_globals_retained_thread_without_callbacks(*this,*r.parent)||
        (r.unref_started&&!r.unref_returned)){r.closed=true;r.stage=NativePostreadRootCDataControl::Stage::failed;return;}
    lua_State*L=r.parent->thread;const int top=lua_gettop(L);const bool previous=calling;calling=true;
    const int status=lua_cpcall(L,native_postread_root_cdata_cleanup,&r);lua_settop(L,top);calling=previous;
    r.closed=true;r.stage=status?NativePostreadRootCDataControl::Stage::failed:NativePostreadRootCDataControl::Stage::retired;
}
void native_postread_root_cdata_close()noexcept{
    for(auto&r:postread_root_cdata_controls)if(r&&!r->closed)native_postread_root_cdata_release_after_abort(*r);
}
