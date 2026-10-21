// Original639700/766CC0/475D80 scoped GameCore lifecycle, inside Host::Impl.
// Independent SDK children use this exact actual Source/world/VM owner.
struct NativeContextGameCoreFrame final {
    Impl& host;const NativeContextGameEventsFrame*const predecessor;
    const BnwWorldViewSourceOperationLoan*const source_operation;
    const BnwStockLuaAttemptLease source_lease;
    const std::shared_ptr<const LiveSessionState> world;
    const std::shared_ptr<const content::BnwGameInfoActionsOwner> actions;
    const content::RulesDatabase*notification_rules=nullptr;
    const std::shared_ptr<const content::BnwNotificationEntriesSourceOwner> notifications;
    const std::vector<content::BnwGameInfoAction>*actions_rows=nullptr;
    const std::vector<content::BnwNotificationEntry>*notification_rows=nullptr;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    Job*actual_job=nullptr;int table_ref=LUA_NOREF;const void*table=nullptr;
    std::size_t copied=0;int protected_status=-1;bool callback_returned=false,returned=false;
    explicit NativeContextGameCoreFrame(Impl&p)
      :host(p),predecessor(p.game_events_frame),
       source_operation(p.worldview_native_host?p.worldview_native_host->loan:nullptr),
       source_lease(source_operation?source_operation->sp_client_source_owned():throw ScriptError("GameCore Source operation absent")),
       world(source_lease.world_lease()),
       actions(source_operation->context_gameinfo_actions_owned()),
       notifications(source_operation->context_notification_entries_owned(&notification_rules)),
       epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(p.gamecore_frame||!predecessor||!world||!actions||!notifications||!notification_rules)
            throw ScriptError("GameCore requires actual GameEvents/Source/world/actions owners");
        predecessor->first();
        if(!predecessor->returned||!predecessor->callback_returned||predecessor->protected_status||
           predecessor->table_ref<0||!predecessor->table||!predecessor->actual_job)
            throw ScriptError("GameCore real GameEvents normal return absent");
        actions->require_same_rules(*notification_rules);notifications->require_current(*notification_rules);
        actions_rows=&actions->actions();notification_rows=&notifications->rows_owned(*notification_rules);
        source_operation->require_current();predecessor->full();
        p.gamecore_frame=this;
    }
    ~NativeContextGameCoreFrame(){host.native_gamecore_detach_frame_owned(this);}
    void first()const{
        if(host.gamecore_frame!=this||host.game_events_frame!=predecessor||creator!=std::this_thread::get_id()||
           creator!=host.ui_thread||epoch!=host.failure_epoch||writes!=host.failed.writes_||
           uncertainty!=host.execution_uncertainty||host.failed.exhausted_)
            throw ScriptError("GameCore real frame/Source/failure changed");
        predecessor->first();source_lease.require_active();
    }
    void full()const{first();predecessor->full();source_operation->require_current();first();}
};
class NativeGameCoreBootstrapOwner final {
    friend struct Impl;
    Impl*const host;lua_State*const master;global_State*const global;
    const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    const std::shared_ptr<const LiveSessionState> world;
    const std::shared_ptr<const content::BnwGameInfoActionsOwner> actions;
    const content::RulesDatabase*const notification_rules;
    const std::shared_ptr<const content::BnwNotificationEntriesSourceOwner> notifications;
    const std::vector<content::BnwGameInfoAction>*const actions_rows;
    const std::vector<content::BnwNotificationEntry>*const notification_rows;
    std::shared_ptr<const BnwContextLuaProfileConfigLoan> config;
    NativeContextGameCoreFrame*frame=nullptr;Job*actual_job=nullptr;
    enum class Phase{captured,thread_created,environment_retained,events_returned,profile_entered,profile_returned,
        table_created,global_assigned,metadata_applied,fenv_assigned,exports_entered,exports_returned,thread_destroyed,returned};
    Phase phase=Phase::captured;
    bool retired=false,profile_gc_stopped=false;
    lua_State*temporary_thread=nullptr;
    int thread_ref=LUA_NOREF,environment_ref=LUA_NOREF,table_ref=LUA_NOREF,metadata_ref=LUA_NOREF;
    const void*environment=nullptr;const void*table=nullptr;const void*metadata=nullptr;
    int creator_status=-1,events_status=-1,profile_status=-1,export_status=-1,destroy_status=-1;
    std::size_t export_stage=0;
    // SDK order: Enums,Fractal,GameInfo,Map,Game,Player,Team. Each pair contains
    // ACTUAL returned namespace/method counts or methods/instances for slots.
    std::array<std::pair<std::size_t,std::size_t>,7> completed_counts{};
    explicit NativeGameCoreBootstrapOwner(Impl&p,NativeContextGameCoreFrame&f)
      :host(&p),master(p.state),global(G(p.state)),creator(f.creator),epoch(f.epoch),writes(f.writes),
       uncertainty(f.uncertainty),world(f.world),actions(f.actions),notification_rules(f.notification_rules),
       notifications(f.notifications),actions_rows(f.actions_rows),notification_rows(f.notification_rows),
       frame(&f),actual_job(f.actual_job){}
public:~NativeGameCoreBootstrapOwner()=default;
};
void native_gamecore_detach_frame_owned(const NativeContextGameCoreFrame*f)noexcept{
    auto*s=native_gamecore_bootstrap.get();
    if(s&&s->frame==f){
        if(!f->returned||s->phase!=NativeGameCoreBootstrapOwner::Phase::returned)s->retired=true;
        s->frame=nullptr;s->actual_job=nullptr;
    }
    if(gamecore_frame==f)gamecore_frame=nullptr;
}
// OUTER C++ only. Bind the real Job before constructing the persistent owner;
// every fallible owning acquisition completes before any Lua C callback.
void native_gamecore_prepare_registration_owned(NativeContextGameCoreFrame&f,Job&job,
    const std::shared_ptr<const BnwContextLuaProfileConfigLoan>&actual_config){
    f.full();
    if(gamecore_frame!=&f||f.actual_job||f.returned||calling||native_master_protected_boundary||
       job.owner!=this||job.operation!=Operation::native_context_gamecore_registration||!actual_config)
        throw ScriptError("GameCore outer preparation requires idle actual Frame/Job/profile config");
    actual_config->require_current_owned(*f.source_operation);f.full();
    f.actual_job=&job;
    if(!native_gamecore_bootstrap)
        native_gamecore_bootstrap=std::unique_ptr<NativeGameCoreBootstrapOwner>(new NativeGameCoreBootstrapOwner(*this,f));
    else{
        auto&s=*native_gamecore_bootstrap;
        if(s.retired||s.host!=this||s.master!=state||s.global!=G(state)||
           s.phase!=NativeGameCoreBootstrapOwner::Phase::returned||s.frame||s.actual_job||
           s.world.get()!=f.world.get()||s.world.owner_before(f.world)||f.world.owner_before(s.world)||
           s.actions.get()!=f.actions.get()||s.actions.owner_before(f.actions)||f.actions.owner_before(s.actions)||
           s.notifications.get()!=f.notifications.get()||s.notifications.owner_before(f.notifications)||
           f.notifications.owner_before(s.notifications)||s.notification_rules!=f.notification_rules||
           s.actions_rows!=f.actions_rows||s.notification_rows!=f.notification_rows)
            throw ScriptError("GameCore returned cache cannot be replaced or adopt another actual world/data owner");
        s.frame=&f;s.actual_job=&job;
    }
    native_gamecore_bootstrap->config=actual_config;
    // Actual64 slot capture is another outer C++ acquisition; its independent
    // SDK adapter neither creates this owner nor enters a Lua constructor.
    native_gamecore_prepare_player_team_slots_owned();f.full();
}

// A genuine GameCore scoped thread is distinct from selected518. Upstream
// registrar FULL remains restricted to its actual master/selected thread.
// This zero-result master callback protects its Lua errors independently;
// no master longjmp can bypass the outstanding temp exporter callback/lock.
static int native_gamecore_predecessor_master_full_owned(lua_State*L){
    auto&p=owner(L);auto*const f=p.gamecore_frame;
    auto*const job=static_cast<Job*>(lua_touserdata(L,1));
    if(!f||!job||L!=p.state||f->actual_job!=job||job->owner!=&p||
       job->operation!=Operation::native_context_gamecore_registration||!f->predecessor||
       !f->predecessor->actual_job)
        return native_master_thread_refuse(L,"GameCore scoped predecessor actual master/Frame/Job changed");
    native_context_game_events_registration_full(L,*f->predecessor->actual_job);
    if(p.gamecore_frame!=f||f->actual_job!=job)
        return native_master_thread_refuse(L,"GameCore scoped predecessor Frame/Job changed during master FULL");
    return 0;
}
static void native_context_gamecore_registration_full(lua_State*L,Job&job){
    auto&p=owner(L);auto*const f=p.gamecore_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("GameCore entered frame absent");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="GameCore Source FULL refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    if(p.gamecore_frame!=f||f->actual_job!=&job||job.owner!=&p||
       job.operation!=Operation::native_context_gamecore_registration||!f->predecessor->actual_job)
        return (void)native_master_thread_refuse(L,"GameCore actual master Job/GameEvents changed");
    if(L==p.state||L==f->predecessor->selected_thread){
        native_context_game_events_registration_full(L,*f->predecessor->actual_job);
    }else{
        auto*const s=p.native_gamecore_bootstrap.get();
        if(!s||s->retired||s->host!=&p||s->master!=p.state||s->global!=G(L)||
           s->frame!=f||s->actual_job!=&job||s->temporary_thread!=L||s->thread_ref<0||
           s->table_ref<0||!s->table||s->profile_gc_stopped||
           s->phase<NativeGameCoreBootstrapOwner::Phase::fenv_assigned||
           s->phase>NativeGameCoreBootstrapOwner::Phase::exports_returned||
           !ttistable(gt(L))||hvalue(gt(L))!=s->table||
           !p.native_master_protected_boundary||!p.native_master_protected_boundary->held_locks)
            return (void)native_master_thread_refuse(L,"GameCore FULL requires its actual earned scoped exporter thread");
        const int status=lua_cpcall(p.state,native_gamecore_predecessor_master_full_owned,&job);
        if(status){
            // Consume only the actual protected master error. Propagate on
            // THIS thread so its active child cpcall and parent lock unwind.
            lua_xmove(p.state,L,1);return (void)lua_error(L);
        }
    }
    if(p.gamecore_frame!=f)return (void)native_master_thread_refuse(L,"GameCore frame changed during predecessor FULL");
    p.source_exception={};p.source_error=nullptr;
    try{f->first();f->actions->require_same_rules(*f->notification_rules);
        f->notifications->require_current(*f->notification_rules);
        if(&f->actions->actions()!=f->actions_rows||
           &f->notifications->rows_owned(*f->notification_rules)!=f->notification_rows)
            throw ScriptError("GameCore actual immutable retained rows changed");}
    catch(...){p.source_exception=std::current_exception();p.source_error="GameCore actual retained data owners POST refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    const auto observed=native_master_thread_callback_first(L);native_master_thread_callback_post(L,observed);
}
static NativeGameCoreBootstrapOwner* native_gamecore_scope_current_owned(lua_State*L){
    auto&p=owner(L);auto*s=p.native_gamecore_bootstrap.get();
    if(!s||!s->frame||!s->actual_job)return (native_master_thread_refuse(L,"GameCore scoped callback owner absent"),nullptr);
    native_context_gamecore_registration_full(L,*s->actual_job);
    if(p.native_gamecore_bootstrap.get()!=s||s->host!=&p||s->master!=p.state||s->global!=G(L)||
       s->retired||s->creator!=std::this_thread::get_id()||s->creator!=p.ui_thread||
       s->epoch!=p.failure_epoch||s->writes!=p.failed.writes_||s->uncertainty!=p.execution_uncertainty||
       s->frame!=p.gamecore_frame||s->frame->actual_job!=s->actual_job)
        return (native_master_thread_refuse(L,"GameCore real scoped owner/VM/Source changed"),nullptr);
    const int top=lua_gettop(L);
    if(s->thread_ref>=0){lua_rawgeti(L,LUA_REGISTRYINDEX,s->thread_ref);
        if(lua_tothread(L,-1)!=s->temporary_thread)return (native_master_thread_refuse(L,"GameCore scoped thread keeper changed"),nullptr);lua_pop(L,1);}
    if(s->environment_ref>=0){lua_rawgeti(L,LUA_REGISTRYINDEX,s->environment_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=s->environment)return (native_master_thread_refuse(L,"GameCore prior environment keeper changed"),nullptr);lua_pop(L,1);}
    if(s->table_ref>=0){lua_rawgeti(L,LUA_REGISTRYINDEX,s->table_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=s->table)return (native_master_thread_refuse(L,"GameCore real cache keeper changed"),nullptr);lua_pop(L,1);}
    if(s->phase>=NativeGameCoreBootstrapOwner::Phase::global_assigned){
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_master_thread_bootstrap->globals_ref);lua_getfield(L,-1,"GameCore");
        if(lua_topointer(L,-1)!=s->table)return (native_master_thread_refuse(L,"GameCore master cache publication changed"),nullptr);lua_pop(L,2);}
    if(s->phase>=NativeGameCoreBootstrapOwner::Phase::metadata_applied){
        lua_rawgeti(L,LUA_REGISTRYINDEX,s->table_ref);
        if(!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=s->metadata)
            return (native_master_thread_refuse(L,"GameCore actual cache metatable changed"),nullptr);
        lua_rawgeti(L,LUA_REGISTRYINDEX,s->metadata_ref);
        if(!lua_rawequal(L,-1,-2))return (native_master_thread_refuse(L,"GameCore actual metatable keeper changed"),nullptr);
        lua_pop(L,1);lua_pushliteral(L,"__index");lua_rawget(L,-2);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=s->environment)
            return (native_master_thread_refuse(L,"GameCore actual prior-environment __index changed"),nullptr);
        lua_pop(L,3);
    }
    if(s->temporary_thread&&s->phase>=NativeGameCoreBootstrapOwner::Phase::fenv_assigned&&
       (!ttistable(gt(s->temporary_thread))||hvalue(gt(s->temporary_thread))!=s->table))
        return (native_master_thread_refuse(L,"GameCore real scoped fenv changed"),nullptr);
    lua_settop(L,top);return s;
}
static void native_gamecore_scoped_profile_current_owned(lua_State*L){
    auto*s=native_gamecore_scope_current_owned(L);
    if(L!=s->master||s->phase!=NativeGameCoreBootstrapOwner::Phase::profile_entered||!s->profile_gc_stopped||
       !s->temporary_thread||s->environment_ref<0||!s->environment||!s->config)
        return (void)native_master_thread_refuse(L,"GameCore repeated61 actual scoped/profile/GC phase changed");
}
static void native_gamecore_export_current_owned(lua_State*L){
    auto&p=owner(L);auto*s=native_gamecore_scope_current_owned(L);
    if(L!=s->temporary_thread||s->phase!=NativeGameCoreBootstrapOwner::Phase::exports_entered||
       s->profile_gc_stopped||s->export_stage>6||s->table_ref<0||!s->table)
        return (void)native_master_thread_refuse(L,"GameCore78A790 real exporter scope/current fenv absent");
    if(!ttistable(gt(L))||hvalue(gt(L))!=s->table||G(L)!=G(p.state))
        return (void)native_master_thread_refuse(L,"GameCore78A790 actual cached table is not scoped thread globals");
}
// Late bootstrap/method use follows the SAME persistent cache after normal
// scoped-thread destruction. A new current Source is checked in C++ before
// Lua APIs; no expired lexical Frame/Job is borrowed here.
void native_gamecore_runtime_current_cpp_owned(){
    auto*s=native_gamecore_bootstrap.get();
    if(!s||s->retired||s->host!=this||s->master!=state||!state||s->global!=G(state)||
       s->creator!=std::this_thread::get_id()||s->creator!=ui_thread||
       s->epoch!=failure_epoch||s->writes!=failed.writes_||s->uncertainty!=execution_uncertainty||failed.exhausted_||
       s->phase!=NativeGameCoreBootstrapOwner::Phase::returned||s->export_stage!=7||
       s->temporary_thread||s->thread_ref>=0||s->environment_ref>=0||s->profile_gc_stopped||
       s->table_ref<0||s->metadata_ref<0||!s->table||!s->metadata||!s->environment)
        throw ScriptError("GameCore runtime requires its actual normal scoped/exporter return");
    worldview_require_callback_origin();
    if(!worldview_operation||worldview_native_host->loan!=worldview_operation)
        throw ScriptError("GameCore runtime actual Source operation absent");
    const auto actual_source=worldview_operation->sp_client_source_owned();
    const auto actual_world=actual_source.world_lease();
    if(actual_world.get()!=s->world.get()||actual_world.owner_before(s->world)||s->world.owner_before(actual_world))
        throw ScriptError("GameCore runtime cached owner belongs to another actual world allocation");
    s->actions->require_same_rules(*s->notification_rules);s->notifications->require_current(*s->notification_rules);
    if(&s->actions->actions()!=s->actions_rows||&s->notifications->rows_owned(*s->notification_rules)!=s->notification_rows)
        throw ScriptError("GameCore runtime actual immutable data rows changed");
    actual_source.require_active();worldview_require_callback_origin();
}
static NativeGameCoreBootstrapOwner* native_gamecore_invocation_current_owned(lua_State*L){
    auto&p=owner(L);native_db_query_current(L);p.source_exception={};p.source_error=nullptr;
    try{p.native_gamecore_runtime_current_cpp_owned();}
    catch(...){p.source_exception=std::current_exception();p.source_error="GameCore persistent runtime Source/data owner refused";}
    if(p.source_error)return (native_master_thread_refuse(L,p.source_error),nullptr);
    auto*s=p.native_gamecore_bootstrap.get();
    if(G(L)!=s->global)return (native_master_thread_refuse(L,"GameCore runtime has another actual VM"),nullptr);
    const int top=lua_gettop(L);lua_rawgeti(L,LUA_REGISTRYINDEX,s->table_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=s->table||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=s->metadata)
        return (native_master_thread_refuse(L,"GameCore runtime actual cached table/metatable changed"),nullptr);
    lua_rawgeti(L,LUA_REGISTRYINDEX,s->metadata_ref);
    if(!lua_rawequal(L,-1,-2))return (native_master_thread_refuse(L,"GameCore runtime actual metadata keeper changed"),nullptr);
    lua_pop(L,1);lua_pushliteral(L,"__index");lua_rawget(L,-2);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=s->environment)
        return (native_master_thread_refuse(L,"GameCore runtime prior scoped environment changed"),nullptr);
    lua_settop(L,top);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_master_thread_bootstrap->globals_ref);
    lua_pushliteral(L,"GameCore");lua_rawget(L,-2);
    if(lua_topointer(L,-1)!=s->table)return (native_master_thread_refuse(L,"GameCore runtime master cache publication changed"),nullptr);
    lua_settop(L,top);native_db_query_current(L);return s;
}
static bool native_gamecore_preserve_namespace_owned(lua_State*L,int environment,const char*name){
    auto&p=owner(L);if(!p.worldview_native_host||!p.native_gamecore_bootstrap)return false;
    auto*s=native_gamecore_invocation_current_owned(L);const int top=lua_gettop(L);
    if(environment<0)environment=top+environment+1;
    if(!name||!lua_istable(L,environment)||p.worldview_native_host->child>=p.child_contexts.size())
        return (native_master_thread_refuse(L,"GameCore namespace preservation lacks actual selected environment"),false);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[p.worldview_native_host->child].environment);
    if(!lua_rawequal(L,-1,environment))
        return (native_master_thread_refuse(L,"GameCore namespace preservation offered another actual environment"),false);
    lua_pop(L,1);lua_rawgeti(L,LUA_REGISTRYINDEX,s->table_ref);lua_pushstring(L,name);lua_rawget(L,-2);
    if(lua_isnil(L,-1))return (native_master_thread_refuse(L,"GameCore requested namespace has no real own cached export"),false);
    lua_pushstring(L,name);lua_rawget(L,environment);
    if(!lua_rawequal(L,-1,-2))return (native_master_thread_refuse(L,"GameCore genuine selected namespace was overwritten"),false);
    lua_settop(L,top);(void)native_gamecore_invocation_current_owned(L);return true;
}
#include "lua_native_context_gamecore_cached_include_owned.inc"
static void native_gamecore_profile_copy_names_owned(lua_State*L,int source,int destination,const char*const*names){
    for(std::size_t i=0;names[i];++i){lua_getfield(L,source,names[i]);native_gamecore_scoped_profile_current_owned(L);
        lua_setfield(L,destination,names[i]);native_gamecore_scoped_profile_current_owned(L);}
}
static int native_gamecore_create_scoped_thread_5a0af0(lua_State*L){
    auto&s=*native_gamecore_scope_current_owned(L);
    if(L!=s.master||lua_touserdata(L,1)!=&s||s.phase!=NativeGameCoreBootstrapOwner::Phase::captured)
        return native_master_thread_refuse(L,"GameCore4D9FC0 actual scoped constructor changed");
    lua_getfield(L,LUA_GLOBALSINDEX,"_CreateThread");(void)native_gamecore_scope_current_owned(L);
    const auto*called=native_master_protected_call_8f7ae0_owned(L,0,1);native_master_protected_call_return_current_owned(L,called);
    if(called->actual_status)return luaL_error(L,"GameCore actual _CreateThread failed");
    s.temporary_thread=lua_tothread(L,-1);if(!s.temporary_thread)return luaL_error(L,"GameCore actual scoped thread result absent");
    lua_pushvalue(L,-1);s.thread_ref=luaL_ref(L,LUA_REGISTRYINDEX);if(s.thread_ref<0)return luaL_error(L,"GameCore actual scoped thread keeper failed");
    s.phase=NativeGameCoreBootstrapOwner::Phase::thread_created;(void)native_gamecore_scope_current_owned(L);return 0;
}
static int native_gamecore_events_4637c0(lua_State*L){
    auto&p=owner(L);auto*s=native_gamecore_scope_current_owned(L);
    if(L!=p.state||lua_touserdata(L,1)!=s||s->phase!=NativeGameCoreBootstrapOwner::Phase::environment_retained||
       !p.events_prefix_frame||p.events_prefix_frame->master_ref<0)
        return native_master_thread_refuse(L,"GameCore actual5ACF20 source/order changed");
    // Actual573AC0 cached mask0xFFFFFF returns its real factory, not a clone.
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.events_prefix_frame->master_ref);lua_rawgeti(L,-1,0xFFFFFF);
    if(lua_topointer(L,-1)!=p.events_prefix_frame->factory)return native_master_thread_refuse(L,"GameCore actual cached EventFactories[FFFFFF] changed");
    const int factory=lua_gettop(L);native_master_find_thread_environment_534b50_owned(L,s->temporary_thread);
    const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=s->environment)return native_master_thread_refuse(L,"GameCore actual scoped Events environment changed");
    lua_createtable(L,0,0);(void)native_gamecore_scope_current_owned(L);
    lua_pushvalue(L,factory);lua_setmetatable(L,-2);lua_setfield(L,environment,"Events");
    s->phase=NativeGameCoreBootstrapOwner::Phase::events_returned;(void)native_gamecore_scope_current_owned(L);return 0;
}
static int native_gamecore_profile_4ec020(lua_State*L){
    auto&p=owner(L);auto*s=native_gamecore_scope_current_owned(L);
    if(L!=p.state||lua_touserdata(L,1)!=s||s->phase!=NativeGameCoreBootstrapOwner::Phase::events_returned)
        return native_master_thread_refuse(L,"GameCore61 actual ordered scoped/profile call changed");
    // Old selected profile's partial-GC flag remains a completed prefix. THIS
    // repeated original profile owns its own real STOP/RESTART lifecycle.
    lua_gc(L,LUA_GCSTOP,0);s->profile_gc_stopped=true;s->phase=NativeGameCoreBootstrapOwner::Phase::profile_entered;
    native_gamecore_scoped_profile_current_owned(L);
    native_master_find_thread_environment_534b50_owned(L,s->temporary_thread);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=s->environment)return native_master_thread_refuse(L,"GameCore61 actual offered environment changed");
    p.source_exception={};p.source_error=nullptr;std::int32_t debug=0;
    try{s->config->require_current_owned(*s->frame->source_operation);
        debug=s->config->debug_library_value_owned(*s->frame->source_operation);
        s->config->require_current_without_callbacks(*s->frame->source_operation);}
    catch(...){p.source_exception=std::current_exception();p.source_error="GameCore61 real INI debug operand refused";}
    if(p.source_error)return native_master_thread_refuse(L,p.source_error);
    static const char*const globals[]={"assert","collectgarbage","error","getmetatable","ipairs","loadstring","next","pairs","pcall","print","select","setmetatable","tonumber","tostring","type","unpack","_VERSION","where","xpcall",nullptr};
    native_gamecore_profile_copy_names_owned(L,LUA_GLOBALSINDEX,environment,globals);
    if(debug==1){lua_getfield(L,LUA_GLOBALSINDEX,"debug");lua_setfield(L,environment,"debug");}
    else if(debug!=0)return native_master_thread_refuse(L,"GameCore61 actual selected debug value unsupported");
    static const char*const coroutine[]={"create","resume","running","status","wrap","yield",nullptr};
    static const char*const string[]={"byte","char","find","format","gmatch","gsub","len","lower","match","rep","reverse","sub","upper",nullptr};
    static const char*const table[]={"concat","count","fill","insert","maxn","remove","sort",nullptr};
    static const char*const math[]={"abs","acos","asin","atan","atan2","ceil","cos","cosh","clamp","deg","exp","floor","fmod","frexp","huge","ldexp","log","log10","max","min","modf","pi","pow","rad","random","randomseed","sin","sinh","sqrt","tan","tanh",nullptr};
    static const char*const os[]={"clock","date","difftime","time","tmpname",nullptr};
    static const char*const libraries[]={"coroutine","string","table","math","os"};
    static const char*const*const members[]={coroutine,string,table,math,os};
    for(std::size_t i=0;i<5;++i){
        lua_getfield(L,LUA_GLOBALSINDEX,libraries[i]);native_gamecore_scoped_profile_current_owned(L);const int source=lua_gettop(L);
        lua_createtable(L,0,0);native_gamecore_scoped_profile_current_owned(L);const int destination=lua_gettop(L);
        native_gamecore_profile_copy_names_owned(L,source,destination,members[i]);
        lua_remove(L,source);lua_createtable(L,0,0);lua_createtable(L,0,0);
        lua_pushvalue(L,-3);lua_setfield(L,-2,"__index");lua_pushboolean(L,0);lua_setfield(L,-2,"__metatable");
        lua_pushcfunction(L,native_profile_readonly_newindex);lua_setfield(L,-2,"__newindex");lua_setmetatable(L,-2);
        lua_remove(L,-2);lua_setfield(L,environment,libraries[i]);native_gamecore_scoped_profile_current_owned(L);
    }
    native_gamecore_profile_cached_include_owned(L,environment,s->actual_job->context);
    lua_getfield(L,LUA_GLOBALSINDEX,"debug");lua_getfield(L,-1,"traceback");
    if(!lua_isfunction(L,-1))return luaL_error(L,"Cannot find debug.traceback method.");
    lua_remove(L,-2);lua_setfield(L,environment,"traceback");native_gamecore_scoped_profile_current_owned(L);
    if(lua_gc(L,LUA_GCRESTART,0)!=0)return native_master_thread_refuse(L,"GameCore61 real GC restart result changed");
    s->profile_gc_stopped=false;s->phase=NativeGameCoreBootstrapOwner::Phase::profile_returned;
    (void)native_gamecore_scope_current_owned(L);return 0;
}
static std::size_t native_gamecore_register_notification_entries_owned(lua_State*L){
    native_gamecore_export_current_owned(L);auto*s=owner(L).native_gamecore_bootstrap.get();std::size_t count=0;
    // FULL wraps every throwing Source/data currentness operation. The actual
    // vectors were acquired in the outer C++ frame before any Lua callback.
    const auto& rows=*s->notification_rows;
    for(std::size_t i=0;i<rows.size();++i){native_gamecore_export_current_owned(L);
        lua_pushinteger(L,rows[i].lua_value);lua_setfield(L,-2,rows[i].type_name.c_str());++count;
        native_gamecore_export_current_owned(L);}
    return count;
}
static int native_gamecore_gameinfo_actions_owned(lua_State*L){
    native_gamecore_export_current_owned(L);auto*s=owner(L).native_gamecore_bootstrap.get();
    lua_createtable(L,0,0);native_gamecore_export_current_owned(L);const int table=lua_gettop(L);
    lua_pushvalue(L,table);lua_setglobal(L,"GameInfoActions");native_gamecore_export_current_owned(L);
    const auto& rows=*s->actions_rows;
    for(std::size_t i=0;i<rows.size();++i){
        if(i>static_cast<std::size_t>(std::numeric_limits<int>::max()))return luaL_error(L,"GameInfoActions actual row index exceeds nativeint");
        lua_createtable(L,0,0);native_gamecore_export_current_owned(L);const int entry=lua_gettop(L);
        for(std::size_t j=0;j<28;++j){const auto& field=rows[i][j];
            if(const auto*v=std::get_if<std::int64_t>(&field.second))lua_pushnumber(L,static_cast<lua_Number>(*v));
            else if(const auto*v=std::get_if<double>(&field.second))lua_pushnumber(L,*v);
            else if(const auto*v=std::get_if<bool>(&field.second))lua_pushboolean(L,*v);
            else if(const auto*v=std::get_if<std::string>(&field.second))lua_pushlstring(L,v->data(),v->size());
            else lua_pushnil(L);
            native_gamecore_export_current_owned(L);lua_setfield(L,entry,field.first.data());native_gamecore_export_current_owned(L);
        }
        lua_rawseti(L,table,static_cast<int>(i));native_gamecore_export_current_owned(L);
    }
    s->completed_counts[2]={1,rows.size()*28};return 0;
}
// Independent SDK helpers are included after this genuine export guard.
#include "lua_native_gamecore_missions_registration_owned.inc"
#include "lua_native_gamecore_enums_registration_owned.inc"
#include "lua_native_gamecore_fractal_registration_owned.inc"
// Map/Game and Players/Teams exact helpers supplied by their bounded packets.
static int native_gamecore_enums_callback(lua_State*L){auto*s=owner(L).native_gamecore_bootstrap.get();s->completed_counts[0]=native_gamecore_register_enums_owned(L);return 0;}
static int native_gamecore_fractal_callback(lua_State*L){auto*s=owner(L).native_gamecore_bootstrap.get();s->completed_counts[1]=native_gamecore_register_fractal_owned(L);return 0;}
static int native_gamecore_map_callback(lua_State*L){auto*s=owner(L).native_gamecore_bootstrap.get();s->completed_counts[3]=native_gamecore_register_map_owned(L);return 0;}
static int native_gamecore_game_callback(lua_State*L){auto*s=owner(L).native_gamecore_bootstrap.get();s->completed_counts[4]=native_gamecore_register_game_owned(L);return 0;}
static int native_gamecore_players_callback(lua_State*L){auto*s=owner(L).native_gamecore_bootstrap.get();s->completed_counts[5]=native_gamecore_register_players_owned(L);return 0;}
static int native_gamecore_teams_callback(lua_State*L){auto*s=owner(L).native_gamecore_bootstrap.get();s->completed_counts[6]=native_gamecore_register_teams_owned(L);return 0;}
static void native_gamecore_protected_sdk_child(lua_State*L,lua_CFunction body){
    native_gamecore_export_current_owned(L);
    const int status=lua_cpcall(L,body,nullptr);
    if(status)return (void)lua_error(L);native_gamecore_export_current_owned(L);
}
static int native_gamecore_export_78a790(lua_State*L){
    auto&s=*native_gamecore_scope_current_owned(L);
    if(L!=s.temporary_thread||s.phase!=NativeGameCoreBootstrapOwner::Phase::fenv_assigned)
        return native_master_thread_refuse(L,"GameCore78A790 actual thread/fenv/source order changed");
    s.phase=NativeGameCoreBootstrapOwner::Phase::exports_entered;s.export_stage=0;native_gamecore_export_current_owned(L);
    lua_createtable(L,0,2);lua_setglobal(L,"MapModData");native_gamecore_export_current_owned(L);
    native_gamecore_protected_sdk_child(L,native_gamecore_enums_callback);++s.export_stage;
    native_gamecore_protected_sdk_child(L,native_gamecore_fractal_callback);++s.export_stage;
    native_gamecore_protected_sdk_child(L,native_gamecore_gameinfo_actions_owned);
    native_gamecore_protected_sdk_child(L,native_gameinfo_types_info_owned);++s.export_stage;
    native_gamecore_protected_sdk_child(L,native_gamecore_map_callback);++s.export_stage;
    native_gamecore_protected_sdk_child(L,native_gamecore_game_callback);++s.export_stage;
    native_gamecore_protected_sdk_child(L,native_gamecore_players_callback);++s.export_stage;
    native_gamecore_protected_sdk_child(L,native_gamecore_teams_callback);++s.export_stage;
    if(s.export_stage!=7)return native_master_thread_refuse(L,"GameCore actual7 SDK registrations incomplete");
    s.phase=NativeGameCoreBootstrapOwner::Phase::exports_returned;(void)native_gamecore_scope_current_owned(L);return 0;
}

static int native_gamecore_destroy_scoped_thread_4bde30(lua_State*L){
    auto&p=owner(L);auto*s=native_gamecore_scope_current_owned(L);
    if(L!=p.state||lua_touserdata(L,1)!=s->temporary_thread||s->phase!=NativeGameCoreBootstrapOwner::Phase::exports_returned)
        return native_master_thread_refuse(L,"GameCore real4BDE30 thread/normal return changed");
    lua_getfield(L,LUA_GLOBALSINDEX,"_DestroyThread");(void)native_gamecore_scope_current_owned(L);
    native_master_find_thread_environment_534b50_owned(L,s->temporary_thread);
    if(lua_topointer(L,-1)!=s->environment)return native_master_thread_refuse(L,"GameCore4BDE30 prior Threads map value changed");
    lua_pushcclosure(L,native_master_clear_thread_environment_6acc70,0);lua_pushvalue(L,-2);lua_call(L,1,0);
    (void)native_gamecore_scope_current_owned(L);lua_pop(L,1);lua_call(L,1,0);(void)native_gamecore_scope_current_owned(L);return 0;
}
static void native_gamecore_normal_scoped_destruction_owned(lua_State*L){
    auto&p=owner(L);auto*s=native_gamecore_scope_current_owned(L);
    if(L!=p.state||s->phase!=NativeGameCoreBootstrapOwner::Phase::exports_returned||s->profile_gc_stopped)
        return (void)native_master_thread_refuse(L,"GameCore normal destruction requires actual7 SDK return andGC restart");
    auto*bank=native_master_reference_bank_current_owned(L);std::uint32_t ordinal=0;
    while(ordinal<bank->reference_count){auto&row=bank->references[ordinal];
        if(row.actual_vm==s->temporary_thread){
            if(row.actual_vm&&row.actual_reference!=LUA_NOREF){
                if(G(row.actual_vm)!=G(L))return (void)native_master_thread_refuse(L,"GameCore real LuaRef belongs to different VM");
                luaL_unref(row.actual_vm,LUA_REGISTRYINDEX,row.actual_reference);
            }row.actual_vm=nullptr;row.actual_reference=LUA_NOREF;
        }bank=native_master_reference_bank_current_owned(L);++ordinal;
    }
    auto*ledger=native_master_thread_ledger_current_owned(L);p.source_exception={};p.source_error=nullptr;
    try{ledger->clear_matching_state_callback_free(s->temporary_thread);}
    catch(...){p.source_exception=std::current_exception();p.source_error="GameCore7458E0 matching ledger clear refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    auto*const boundary=p.native_master_protected_boundary;native_master_protected_boundary_current(L,boundary);
    if(boundary->held_locks==UINT64_MAX)return (void)native_master_thread_refuse(L,"GameCore normal destructor lock exhausted");
    ::EnterCriticalSection(&boundary->critical->actual);++boundary->held_locks;
    s->destroy_status=lua_cpcall(L,native_gamecore_destroy_scoped_thread_4bde30,s->temporary_thread);
    if(s->destroy_status){::LeaveCriticalSection(&boundary->critical->actual);--boundary->held_locks;return (void)lua_error(L);}
    s->phase=NativeGameCoreBootstrapOwner::Phase::thread_destroyed;
    const int thread=s->thread_ref;s->thread_ref=LUA_NOREF;s->temporary_thread=nullptr;luaL_unref(L,LUA_REGISTRYINDEX,thread);
    const int environment=s->environment_ref;s->environment_ref=LUA_NOREF;luaL_unref(L,LUA_REGISTRYINDEX,environment);
    (void)lua_gc(L,LUA_GCCOLLECT,0);::LeaveCriticalSection(&boundary->critical->actual);--boundary->held_locks;
    (void)native_gamecore_scope_current_owned(L);bank=native_master_reference_bank_current_owned(L);
    if(bank->actual_socket_owner->native_socket_state==3)
        return (void)native_master_thread_refuse(L,"GameCore actual4E3750 connected debug transport supplier required");
    bank->actual_socket_owner->require_current_without_callbacks();s->phase=NativeGameCoreBootstrapOwner::Phase::returned;
    (void)native_gamecore_scope_current_owned(L);
}
static void native_gamecore_locked_call_owned(lua_State*master,lua_State*offered,lua_CFunction body,void*userdata,int*status){
    auto&p=owner(master);auto*const boundary=p.native_master_protected_boundary;
    native_master_protected_boundary_current(master,boundary);
    if(boundary->held_locks==UINT64_MAX)return (void)native_master_thread_refuse(master,"GameCore real recursive lock exhausted");
    ::EnterCriticalSection(&boundary->critical->actual);++boundary->held_locks;
    *status=lua_cpcall(offered,body,userdata);
    ::LeaveCriticalSection(&boundary->critical->actual);--boundary->held_locks;
    if(*status){if(offered!=master)lua_xmove(offered,master,1);return (void)lua_error(master);}
}
static int native_gamecore_cache_constructor_475d80(lua_State*L){
    auto&p=owner(L);auto*s=native_gamecore_scope_current_owned(L);
    if(L!=p.state||s->phase!=NativeGameCoreBootstrapOwner::Phase::captured)
        return native_master_thread_refuse(L,"GameCore475D80 fresh scoped constructor replay changed");
    native_gamecore_locked_call_owned(L,L,native_gamecore_create_scoped_thread_5a0af0,s,&s->creator_status);
    native_master_find_thread_environment_534b50_owned(L,s->temporary_thread);
    const int environment=lua_gettop(L),thread=environment-1;
    if(lua_tothread(L,thread)!=s->temporary_thread||!lua_istable(L,environment))
        return native_master_thread_refuse(L,"GameCore real scoped Threads operands absent");
    s->environment=lua_topointer(L,environment);lua_pushvalue(L,environment);s->environment_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(s->environment_ref<0)return luaL_error(L,"GameCore scoped environment keeper failed");
    s->phase=NativeGameCoreBootstrapOwner::Phase::environment_retained;(void)native_gamecore_scope_current_owned(L);
    native_gamecore_locked_call_owned(L,L,native_gamecore_events_4637c0,s,&s->events_status);
    native_gamecore_locked_call_owned(L,L,native_gamecore_profile_4ec020,s,&s->profile_status);
    (void)native_gamecore_scope_current_owned(L);
    // Original publishes GameCore BEFORE its metatable and before DLL exporter.
    lua_createtable(L,0,0);s->table=lua_topointer(L,-1);lua_pushvalue(L,-1);s->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(s->table_ref<0)return luaL_error(L,"GameCore actual cache keeper failed");
    s->phase=NativeGameCoreBootstrapOwner::Phase::table_created;(void)native_gamecore_scope_current_owned(L);
    lua_pushvalue(L,-1);lua_setfield(L,LUA_GLOBALSINDEX,"GameCore");
    s->phase=NativeGameCoreBootstrapOwner::Phase::global_assigned;(void)native_gamecore_scope_current_owned(L);
    lua_createtable(L,0,0);s->metadata=lua_topointer(L,-1);lua_pushvalue(L,-1);s->metadata_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(s->metadata_ref<0)return luaL_error(L,"GameCore actual metadata keeper failed");
    lua_pushvalue(L,environment);lua_setfield(L,-2,"__index");lua_setmetatable(L,-2);
    s->phase=NativeGameCoreBootstrapOwner::Phase::metadata_applied;(void)native_gamecore_scope_current_owned(L);
    //475E0D->8F9814 IS lua_setfenv, consuming the real GameCore table. Thread
    // key is environment-1; the restricted prior env remains in Threads map.
    if(!lua_setfenv(L,thread))return luaL_error(L,"GameCore actual scoped cached fenv switch failed");
    s->phase=NativeGameCoreBootstrapOwner::Phase::fenv_assigned;(void)native_gamecore_scope_current_owned(L);
    native_gamecore_locked_call_owned(L,s->temporary_thread,native_gamecore_export_78a790,nullptr,&s->export_status);
    native_gamecore_normal_scoped_destruction_owned(L);return 0;
}
static int native_gamecore_selected_callback_766cc0(lua_State*L){
    auto&p=owner(L);auto*const f=p.gamecore_frame;
    if(!f||L!=p.state||!f->predecessor||lua_touserdata(L,1)!=f->predecessor->selected_thread)
        return native_master_thread_refuse(L,"GameCore766CC0 actual selecteduserdata/master changed");
    native_context_gamecore_registration_full(L,*f->actual_job);
    lua_getfield(L,LUA_GLOBALSINDEX,"GameCore");
    if(lua_isnil(L,-1)){
        lua_pop(L,1);auto*s=native_gamecore_scope_current_owned(L);
        if(s->phase!=NativeGameCoreBootstrapOwner::Phase::captured)return luaL_error(L,"GameCore uncompleted cache cannot replay");
        native_gamecore_locked_call_owned(L,L,native_gamecore_cache_constructor_475d80,nullptr,&f->protected_status);
        lua_getfield(L,LUA_GLOBALSINDEX,"GameCore");
    }
    luaL_checktype(L,-1,LUA_TTABLE);const int source=lua_gettop(L);auto*s=native_gamecore_scope_current_owned(L);
    if(s->phase!=NativeGameCoreBootstrapOwner::Phase::returned||lua_topointer(L,source)!=s->table)
        return luaL_error(L,"GameCore actual fully returned cache changed before selectedcopy");
    native_master_find_thread_environment_534b50_owned(L,f->predecessor->selected_thread);
    const int destination=lua_gettop(L);
    if(lua_topointer(L,destination)!=f->predecessor->actual_environment)return luaL_error(L,"GameCore actual selected518 environment changed");
    lua_pushnil(L);
    while(lua_next(L,source)){
        (void)native_gamecore_scope_current_owned(L);
        lua_pushvalue(L,-2);lua_pushvalue(L,-2);lua_settable(L,destination);lua_pop(L,1);++f->copied;
        (void)native_gamecore_scope_current_owned(L);
        if(lua_topointer(L,source)!=s->table||lua_topointer(L,destination)!=f->predecessor->actual_environment)
            return luaL_error(L,"GameCore actual5D7BD0 source/destination changed during copy");
    }
    f->table_ref=s->table_ref;f->table=s->table;f->callback_returned=true;return 0;
}
static void perform_native_context_gamecore_registration(lua_State*L,Job&job){
    auto&p=owner(L);auto*const f=p.gamecore_frame;
    if(!f||f->actual_job!=&job||f->returned||L!=p.state||job.owner!=&p||job.operation!=Operation::native_context_gamecore_registration)
        return (void)native_master_thread_refuse(L,"GameCore639700 real entered master/job/replay changed");
    native_context_gamecore_registration_full(L,job);
    auto*s=p.native_gamecore_bootstrap.get();
    if(!s||s->world.get()!=f->world.get()||s->actions.get()!=f->actions.get()||
       s->notification_rules!=f->notification_rules||s->notifications.get()!=f->notifications.get()||
       s->actions_rows!=f->actions_rows||s->notification_rows!=f->notification_rows)
        return (void)native_master_thread_refuse(L,"GameCore actual retained outer SDK owners changed");
    if(s->phase==NativeGameCoreBootstrapOwner::Phase::returned){s->frame=f;s->actual_job=&job;}
    else if(s->frame!=f||s->actual_job!=&job||s->phase!=NativeGameCoreBootstrapOwner::Phase::captured)
        return (void)native_master_thread_refuse(L,"GameCore retained partial cold owner cannot replay");
    native_gamecore_locked_call_owned(L,L,native_gamecore_selected_callback_766cc0,f->predecessor->selected_thread,&f->protected_status);
    native_context_gamecore_registration_full(L,job);
    if(!f->callback_returned||f->table_ref<0||!f->table||s->phase!=NativeGameCoreBootstrapOwner::Phase::returned||s->export_stage!=7)
        return (void)native_master_thread_refuse(L,"GameCore real7 registrations/scope/selectedcopy never returned");
    f->returned=true;job.context_gamecore_reference=f->table_ref;job.context_gamecore_table=f->table;job.context_gamecore_copied=f->copied;
}
