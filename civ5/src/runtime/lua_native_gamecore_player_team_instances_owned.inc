// Inside LuaScriptHost::Impl. Root owns GameCore's scoped constructor/exporter.
// The outer Root frame already borrowed Source->world BEFORE protected Lua work.
// This adapter stores addresses of actual saved slots, never copied ID objects.
// SDK method names are registered separately below; unissued bodies refuse.
struct NativeGameCorePlayerTeamNamespace final {
    enum class Phase {empty,keys_retained,type_created,cache_created,methods_returned,collection_created,instances_entered,returned};
    Phase phase=Phase::empty;
    int keys_ref=LUA_NOREF,type_ref=LUA_NOREF,cache_ref=LUA_NOREF,weak_meta_ref=LUA_NOREF,collection_ref=LUA_NOREF;
    const void*keys=nullptr;const void*type=nullptr;const void*cache=nullptr;const void*weak_meta=nullptr;const void*collection=nullptr;
    std::array<const void*,64> instances{},instance_metas{};
    std::size_t methods=0,instances_written=0;
};
struct NativeGameCorePlayerTeamSlots final {
    Impl*const host;lua_State*const master;global_State*const global;
    NativeGameCoreBootstrapOwner*const bootstrap;
    const ScriptSourceProvider*const sources;
    const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    const BnwStockLuaAttemptLease source;
    const std::shared_ptr<const LiveSessionState> world;
    std::array<const saves::SavedPlayerObservation*,64> players{};
    std::array<const ResearchPlayerState*,64> setup{};
    std::array<const saves::TeamObservation*,64> teams{};
    std::array<const TeamTechnologies*,64> technologies{};
    std::array<std::size_t,64> saved_team_rows{},technology_rows{};
    std::array<NativeGameCorePlayerTeamNamespace,2> namespaces;
    NativeGameCorePlayerTeamSlots(Impl&p,const NativeContextGameCoreFrame&f,NativeGameCoreBootstrapOwner&b)
      :host(&p),master(p.state),global(G(p.state)),bootstrap(&b),sources(&p.sources),creator(p.ui_thread),
       epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty),source(f.source_lease),world(f.world){}
};
std::unique_ptr<NativeGameCorePlayerTeamSlots> native_gamecore_player_team_slots;
static std::int64_t native_gamecore_team_wire_id_owned(const saves::TeamObservation&team){
    const saves::NamedSaveInteger* found=nullptr;
    for(const auto&field:team.fields)if(field.name&&std::string_view(field.name)=="id"){
        if(found)throw ScriptError("GameCore actual Team slot has duplicate native ID");found=&field;
    }
    if(!found)throw ScriptError("GameCore actual Team slot native ID is absent");return found->value;
}
static void native_gamecore_player_wire_identity_owned(const saves::SavedPlayerObservation&player,std::size_t id){
    const saves::PlayerCoreField*found=nullptr;
    for(const auto&field:player.core.fields)if(field.name&&std::string_view(field.name)=="m_eID"){
        if(found)throw ScriptError("GameCore actual Player slot has duplicate m_eID");found=&field;
    }
    if(player.index!=id||player.saved_player_id!=static_cast<std::int64_t>(id)||!found||found->values.size()!=1||
       found->values[0]!=static_cast<std::int64_t>(id)||player.core.layout!=saves::GameStateLayout::expansion2_version1)
        throw ScriptError("GameCore actual Player slot/index/m_eID differs");
}
// C++ only: every shared/weak temporary below dies before the caller uses Lua.
void native_gamecore_player_team_slots_current_cpp_owned(){
    const auto*const s=native_gamecore_player_team_slots.get();
    if(!s||s->host!=this||s->master!=state||!state||s->global!=G(state)||s->sources!=&sources||
       s->creator!=std::this_thread::get_id()||s->creator!=ui_thread||s->epoch!=failure_epoch||s->writes!=failed.writes_||
       s->uncertainty!=execution_uncertainty||failed.exhausted_||native_gamecore_bootstrap.get()!=s->bootstrap||
       !s->bootstrap||s->bootstrap->retired||s->bootstrap->world.get()!=s->world.get()||
       s->bootstrap->world.owner_before(s->world)||s->world.owner_before(s->bootstrap->world))
        throw ScriptError("GameCore actual Player/Team storage/VM/Source/failure retired");
    worldview_require_callback_origin();
    if(!worldview_native_host||!worldview_operation||worldview_native_host->loan!=worldview_operation)
        throw ScriptError("GameCore Player/Team fresh native Source loan absent");
    s->source.require_active();
    const auto actual_source=worldview_operation->sp_client_source_owned();
    const auto actual_world=actual_source.world_lease();
    if(actual_world.get()!=s->world.get()||actual_world.owner_before(s->world)||s->world.owner_before(actual_world))
        throw ScriptError("GameCore Player/Team current Source has another actual world allocation");
    const auto&w=*s->world;
    if(w.setup.profile!=content::TechnologyProfile::bnw||!w.world.all_players||!w.world.teams||
       w.world.all_players->layout!=saves::GameStateLayout::expansion2_version1||
       w.world.teams->layout!=saves::GameStateLayout::expansion2_version1||
       w.world.all_players->context.player_count!=64||w.world.all_players->players.size()!=64||
       w.world.teams->teams.size()!=64||w.setup.players.size()!=64||w.technologies.size()!=64)
        throw ScriptError("GameCore actual Player/Team native64 storage changed");
    for(std::size_t id=0;id<64;++id){
        const auto player=w.setup.players.find(static_cast<std::int64_t>(id));
        if(player==w.setup.players.end()||&player->second!=s->setup[id]||
           &w.world.all_players->players[id]!=s->players[id]||s->saved_team_rows[id]>=64||s->technology_rows[id]>=64||
           &w.world.teams->teams[s->saved_team_rows[id]]!=s->teams[id]||
           &w.technologies[s->technology_rows[id]]!=s->technologies[id])
            throw ScriptError("GameCore actual Player/Team source cell was replaced");
        native_gamecore_player_wire_identity_owned(*s->players[id],id);
        if(native_gamecore_team_wire_id_owned(*s->teams[id])!=static_cast<std::int64_t>(id)||
           s->technologies[id]->team()!=static_cast<std::int64_t>(id))
            throw ScriptError("GameCore actual Team native ID pairing changed");
    }
    actual_source.require_active();s->source.require_active();worldview_require_callback_origin();
}
// Root calls this OUTER C++ helper after constructing its BootstrapOwner and
// before ANY cpcall/allocating Lua constructor. It reuses its real world/source.
void native_gamecore_prepare_player_team_slots_owned(){
    auto*const f=gamecore_frame;auto*const b=native_gamecore_bootstrap.get();
    if(!f||!b||b->frame!=f||!f->world||b->world.get()!=f->world.get()||
       b->world.owner_before(f->world)||f->world.owner_before(b->world))
        throw ScriptError("GameCore Player/Team birth requires Root's real outer frame/world");
    f->full();
    if(native_gamecore_player_team_slots){native_gamecore_player_team_slots_current_cpp_owned();f->full();return;}
    auto actual=std::make_unique<NativeGameCorePlayerTeamSlots>(*this,*f,*b);
    const auto&w=*actual->world;
    if(w.setup.profile!=content::TechnologyProfile::bnw||!w.world.all_players||!w.world.teams||
       w.world.all_players->layout!=saves::GameStateLayout::expansion2_version1||w.world.teams->layout!=saves::GameStateLayout::expansion2_version1||
       w.world.all_players->context.player_count!=64||w.world.all_players->players.size()!=64||
       w.world.teams->teams.size()!=64||w.setup.players.size()!=64||w.technologies.size()!=64)
        throw ScriptError("GameCore Player/Team birth has no complete real64-slot state");
    std::set<const void*>addresses;
    for(std::size_t id=0;id<64;++id){
        const auto&p=w.world.all_players->players[id];native_gamecore_player_wire_identity_owned(p,id);
        const auto found=w.setup.players.find(static_cast<std::int64_t>(id));
        if(found==w.setup.players.end()||!addresses.insert(&p).second)
            throw ScriptError("GameCore actual64 Player identity/slot is absent or duplicated");
        actual->players[id]=&p;actual->setup[id]=&found->second;
    }
    for(std::size_t row=0;row<64;++row){
        const auto&team=w.world.teams->teams[row];const auto id=native_gamecore_team_wire_id_owned(team);
        if(id<0||id>=64||actual->teams[static_cast<std::size_t>(id)]||!addresses.insert(&team).second)
            throw ScriptError("GameCore actual Team ID/domain/source pointer is duplicated or absent");
        actual->teams[static_cast<std::size_t>(id)]=&team;actual->saved_team_rows[static_cast<std::size_t>(id)]=row;
        const auto&tech=w.technologies[row];const auto tech_id=tech.team();
        if(tech_id<0||tech_id>=64||actual->technologies[static_cast<std::size_t>(tech_id)])
            throw ScriptError("GameCore actual Team technology owner/domain is duplicated");
        actual->technologies[static_cast<std::size_t>(tech_id)]=&tech;actual->technology_rows[static_cast<std::size_t>(tech_id)]=row;
    }
    for(std::size_t id=0;id<64;++id)if(!actual->teams[id]||!actual->technologies[id])
        throw ScriptError("GameCore actual64 Team slot/technology pair is missing");
    f->full();if(gamecore_frame!=f||native_gamecore_bootstrap.get()!=b)
        throw ScriptError("GameCore Player/Team outer source changed during slot capture");
    native_gamecore_player_team_slots=std::move(actual);native_gamecore_player_team_slots_current_cpp_owned();f->full();
}
static NativeGameCorePlayerTeamSlots* native_gamecore_slots_lua_current_owned(lua_State*L,bool constructing){
    auto&p=owner(L);
    if(constructing)native_gamecore_export_current_owned(L);else native_db_query_current(L);
    p.source_exception={};p.source_error=nullptr;
    try{p.native_gamecore_player_team_slots_current_cpp_owned();}
    catch(const std::exception&e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="GameCore actual Player/Team source currentness refused";}
    if(p.source_error){p.record_failure();return (luaL_error(L,"%s",p.source_error),nullptr);}
    auto*const s=p.native_gamecore_player_team_slots.get();
    if(!s||G(L)!=s->global)return (p.record_failure(),luaL_error(L,"GameCore Player/Team has another real VM"),nullptr);
    if(constructing){
        if(s->bootstrap->temporary_thread!=L)return (p.record_failure(),luaL_error(L,"GameCore Player/Team constructor is not actual scoped thread"),nullptr);
    }else if(s->bootstrap->phase!=NativeGameCoreBootstrapOwner::Phase::returned){
        return (p.record_failure(),luaL_error(L,"GameCore Player/Team constructor/exporter has no normal terminal return"),nullptr);
    }
    return s;
}
struct NativeGameCoreSlotMethod final {const char*name;lua_CFunction body;};
static void native_gamecore_slot_key_owned(lua_State*L,bool team,int key){
    const auto&s=*owner(L).native_gamecore_player_team_slots;const auto&n=s.namespaces[team?1:0];
    lua_rawgeti(L,LUA_REGISTRYINDEX,n.keys_ref);lua_rawgeti(L,-1,key);lua_remove(L,-2);
}
static const void* native_gamecore_actual_slot_pointer_owned(const NativeGameCorePlayerTeamSlots&s,bool team,std::size_t id){
    return team?static_cast<const void*>(s.teams[id]):static_cast<const void*>(s.players[id]);
}
static void native_gamecore_slot_namespace_identity_owned(lua_State*L,bool team,bool constructing){
    auto*const s=native_gamecore_slots_lua_current_owned(L,constructing);const auto&n=s->namespaces[team?1:0];const int top=lua_gettop(L);
    if(n.keys_ref<0||n.type_ref<0||n.cache_ref<0||n.weak_meta_ref<0||n.collection_ref<0||
       !n.keys||!n.type||!n.cache||!n.weak_meta||!n.collection)
        return (void)luaL_error(L,"GameCore Player/Team authentic namespace/cache roots are absent");
    if(!constructing&&(n.phase!=NativeGameCorePlayerTeamNamespace::Phase::returned||n.instances_written!=64||
       n.methods!=(team?std::size(native_gamecore_team_methods):std::size(native_gamecore_player_methods))))
        return (void)luaL_error(L,"GameCore Player/Team namespace has no complete genuine slot registration return");
    lua_rawgeti(L,LUA_REGISTRYINDEX,n.keys_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=n.keys||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"GameCore Player/Team retained original keys changed");
    for(int i=1;i<=7;++i){
        const char*const expected=i==1?"__instance":i==2?"__index":i==3?"__instances":i==4?"__mode":i==5?"v":i==6?(team?"Team":"Player"):(team?"Teams":"Players");
        lua_rawgeti(L,-1,i);std::size_t length=0;const char*const actual=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&length):nullptr;
        if(!actual||length!=std::strlen(expected)||std::memcmp(actual,expected,length))
            return (void)luaL_error(L,"GameCore Player/Team retained SDK key TString changed");lua_pop(L,1);
    }lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,s->bootstrap->table_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=s->bootstrap->table)
        return (void)luaL_error(L,"GameCore Player/Team real cachedGameCore table changed");
    native_gamecore_slot_key_owned(L,team,6);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,n.type_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=n.type||!lua_rawequal(L,-1,-2)||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"GameCore Player/Team actual shared SDK type table changed");lua_pop(L,2);
    native_gamecore_slot_key_owned(L,team,7);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,n.collection_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=n.collection||!lua_rawequal(L,-1,-2)||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"GameCore Player/Team actual SDK collection changed");lua_pop(L,3);
    lua_rawgeti(L,LUA_REGISTRYINDEX,n.type_ref);native_gamecore_slot_key_owned(L,team,3);lua_rawget(L,-2);
    lua_rawgeti(L,LUA_REGISTRYINDEX,n.cache_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=n.cache||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=n.weak_meta)
        return (void)luaL_error(L,"GameCore Player/Team actual pointer-key weak cache changed");
    native_gamecore_slot_key_owned(L,team,4);lua_rawget(L,-2);native_gamecore_slot_key_owned(L,team,5);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"GameCore Player/Team weak-values __mode changed");
    lua_settop(L,top);
    if(!constructing){
        const auto child=owner(L).worldview_native_host->child;
        if(child>=owner(L).child_contexts.size())return (void)luaL_error(L,"GameCore Player/Team selected child retired");
        lua_rawgeti(L,LUA_REGISTRYINDEX,owner(L).child_contexts[child].environment);
        lua_pushvalue(L,LUA_GLOBALSINDEX);
        if(!lua_istable(L,-1)||!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"GameCore Player/Team caller uses another selected fenv");lua_pop(L,1);
        native_gamecore_slot_key_owned(L,team,6);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,n.type_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"GameCore Player/Team selected exported type table changed");lua_pop(L,2);
        native_gamecore_slot_key_owned(L,team,7);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,n.collection_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"GameCore Player/Team selected exported collection changed");lua_pop(L,3);
    }
    native_gamecore_slots_lua_current_owned(L,constructing);lua_settop(L,top);
}
static std::size_t native_gamecore_slot_receiver_owned(lua_State*L,bool team,bool constructing,int receiver){
    native_gamecore_slot_namespace_identity_owned(L,team,constructing);
    auto*const s=owner(L).native_gamecore_player_team_slots.get();const auto&n=s->namespaces[team?1:0];
    if(receiver<0)receiver=lua_gettop(L)+receiver+1;
    if(lua_type(L,receiver)!=LUA_TTABLE)return (luaL_error(L,"GameCore actual scoped instance table is required"),64);
    const int top=lua_gettop(L);native_gamecore_slot_key_owned(L,team,1);lua_rawget(L,receiver);
    if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA||!lua_touserdata(L,-1))return (luaL_error(L,"GameCore actual nonNULL lightuserdata instance is required"),64);
    const void*const actual=lua_touserdata(L,-1);lua_pop(L,1);std::size_t id=64;
    for(std::size_t i=0;i<64;++i)if(actual==native_gamecore_actual_slot_pointer_owned(*s,team,i)){id=i;break;}
    if(id==64||lua_topointer(L,receiver)!=n.instances[id]||!lua_getmetatable(L,receiver)||lua_topointer(L,-1)!=n.instance_metas[id])
        return (luaL_error(L,"GameCore instance is not a genuine current actual source slot"),64);
    native_gamecore_slot_key_owned(L,team,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,n.type_ref);
    if(!lua_rawequal(L,-1,-2))return (luaL_error(L,"GameCore instance actual shared type __index changed"),64);lua_settop(L,top);
    lua_rawgeti(L,LUA_REGISTRYINDEX,n.cache_ref);lua_pushlightuserdata(L,const_cast<void*>(actual));lua_rawget(L,-2);
    if(!lua_rawequal(L,-1,receiver))return (luaL_error(L,"GameCore actual pointer cache has another instance identity"),64);lua_settop(L,top);
    lua_rawgeti(L,LUA_REGISTRYINDEX,n.collection_ref);lua_rawgeti(L,-1,static_cast<int>(id));
    if(!lua_rawequal(L,-1,receiver))return (luaL_error(L,"GameCore actual64 collection has another instance identity"),64);lua_settop(L,top);
    native_gamecore_slots_lua_current_owned(L,constructing);return id;
}
struct NativeGameCoreSlotReadScope final {
    Impl&p;lua_State*const entry;NativeGameCorePlayerTeamSlots*const slots;
    const WorldViewNativeHostScope*const native;const BnwWorldViewSourceOperationLoan*const loan;
    const std::uint64_t epoch,writes,uncertainty,serial;const std::size_t child;
    const std::weak_ptr<const ScriptUiChildSource> source;const ScriptUiChildSource*const source_get;
    const int environment_ref,thread_ref;const std::size_t graph_scope,execution_source;
    const bool prior_failed,prior_ready;
    enum class Value {none,integer,boolean};Value value=Value::none;
    std::int64_t integer=0;bool boolean=false,provisional=false,returned=false;
    NativeGameCoreSlotReadScope(Impl&h,lua_State*L):p(h),entry(L),slots(p.native_gamecore_player_team_slots.get()),
      native(p.worldview_native_host),loan(p.worldview_operation),epoch(p.failure_epoch),writes(p.failed.writes_),
      uncertainty(p.execution_uncertainty),serial(native?native->serial:0),child(native?native->child:graph_no_owner),
      source(child<p.child_contexts.size()?p.child_contexts[child].source:std::weak_ptr<const ScriptUiChildSource>{}),
      source_get(child<p.child_contexts.size()?p.child_contexts[child].source.get():nullptr),
      environment_ref(child<p.child_contexts.size()?p.child_contexts[child].environment:LUA_NOREF),
      thread_ref(child<p.child_contexts.size()?p.child_contexts[child].execution_thread:LUA_NOREF),graph_scope(p.graph_execution_scope),
      execution_source(child<p.child_contexts.size()?p.child_contexts[child].execution_source:graph_no_owner),
      prior_failed(p.failed),prior_ready(p.ready){
        if(p.native_gamecore_slot_read_scope||!slots||!native||!loan||child>=p.child_contexts.size())
            throw ScriptError("GameCore instance getter requires one actual entered native child/source scope");
        p.native_gamecore_player_team_slots_current_cpp_owned();p.native_gamecore_slot_read_scope=this;
    }
    void current()const{
        if(p.native_gamecore_slot_read_scope!=this||p.native_gamecore_player_team_slots.get()!=slots||
           p.worldview_native_host!=native||p.worldview_operation!=loan||!native||native->child!=child||native->serial!=serial||
           child>=p.child_contexts.size()||p.child_contexts[child].retired||source.expired()||
           source_get!=p.child_contexts[child].source.get()||source.owner_before(p.child_contexts[child].source)||
           p.child_contexts[child].source.owner_before(source)||environment_ref!=p.child_contexts[child].environment||
           thread_ref!=p.child_contexts[child].execution_thread||graph_scope!=p.graph_execution_scope||
           execution_source!=p.child_contexts[child].execution_source||!p.worldview_native_child_controls_current(child)||
           p.failure_epoch!=epoch||p.failed.writes_!=writes||p.failed.exhausted_||p.execution_uncertainty!=uncertainty||
           static_cast<bool>(p.failed)!=(provisional?true:prior_failed)||p.ready!=(provisional?false:prior_ready))
            throw ScriptError("GameCore instance getter actual lender/source/failure changed");
        p.native_gamecore_player_team_slots_current_cpp_owned();
    }
    void enter(){current();p.failed.set_loader_sentinel();p.ready=false;provisional=true;current();}
    void accept(lua_State*L){
        current();
        if(L!=entry||lua_gettop(L)!=1||value==Value::none||
           (value==Value::integer&&(lua_type(L,1)!=LUA_TNUMBER||lua_tonumber(L,1)!=static_cast<lua_Number>(integer)))||
           (value==Value::boolean&&(lua_type(L,1)!=LUA_TBOOLEAN||(lua_toboolean(L,1)!=0)!=boolean)))
            throw ScriptError("GameCore actual instance getter protected publication differs from earned result");
        p.ready=prior_ready;if(prior_failed)p.failed.set_loader_sentinel();else p.failed=false;provisional=false;current();returned=true;
    }
    ~NativeGameCoreSlotReadScope(){if(p.native_gamecore_slot_read_scope!=this||!returned)p.record_failure();p.native_gamecore_slot_read_scope=nullptr;}
    NativeGameCoreSlotReadScope(const NativeGameCoreSlotReadScope&)=delete;
    NativeGameCoreSlotReadScope&operator=(const NativeGameCoreSlotReadScope&)=delete;
};
NativeGameCoreSlotReadScope*native_gamecore_slot_read_scope=nullptr;
template<bool Team,std::size_t Ordinal>static int native_gamecore_slot_read_body_owned(lua_State*L){
    auto&p=owner(L);auto*const f=p.native_gamecore_slot_read_scope;
    if(!f||f->entry!=L){p.record_failure();return luaL_error(L,"GameCore instance getter lacks actual protected lender");}
    const auto id=native_gamecore_slot_receiver_owned(L,Team,false,1);
    const auto&method=Team?native_gamecore_team_methods[Ordinal]:native_gamecore_player_methods[Ordinal];
    const auto&n=f->slots->namespaces[Team?1:0];const int top=lua_gettop(L);
    native_gamecore_slot_key_owned(L,Team,static_cast<int>(Ordinal)+8);std::size_t key_length=0;
    const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&key_length):nullptr;
    if(!key||key_length!=std::strlen(method.name)||std::memcmp(key,method.name,key_length)){
        p.record_failure();return luaL_error(L,"GameCore original method key TString changed");}lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,n.type_ref);native_gamecore_slot_key_owned(L,Team,static_cast<int>(Ordinal)+8);lua_rawget(L,-2);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=method.body||lua_getupvalue(L,-1,1)){
        p.record_failure();return luaL_error(L,"GameCore original zero-upvalue instance method identity changed");}lua_settop(L,top);
    std::int64_t technology=0;
    if constexpr(Team){if(std::strcmp(method.name,"IsHasTech")==0){technology=source_integer(L,2);
        if(technology<INT32_MIN||technology>INT32_MAX)return luaL_error(L,"GameCore Team technology native index is outside int32");}}
    p.source_exception={};p.source_error=nullptr;
    try{
        f->current();
        if(std::strcmp(method.name,"GetID")==0){
            f->integer=Team?native_gamecore_team_wire_id_owned(*f->slots->teams[id]):f->slots->players[id]->saved_player_id;
            f->value=NativeGameCoreSlotReadScope::Value::integer;
        }else if constexpr(Team){
            if(std::strcmp(method.name,"IsHasTech")!=0)throw ScriptError("Original Team method has no issued genuine engine consumer");
            if(!p.gameplay.has_tech)throw ScriptError("Team IsHasTech actual owned technology provider is unissued");
            f->boolean=p.gameplay.has_tech(static_cast<std::int64_t>(id),technology);f->value=NativeGameCoreSlotReadScope::Value::boolean;
        }else{
            if(std::strcmp(method.name,"GetTeam")==0){
                if(!p.gameplay.player_team)throw ScriptError("Player GetTeam genuine state provider is unissued");
                f->integer=p.gameplay.player_team(static_cast<std::int64_t>(id));
                if(f->integer< -1||f->integer>=64)throw ScriptError("Player actual team is outside native64 domain");
                f->value=NativeGameCoreSlotReadScope::Value::integer;
            }else if(std::strcmp(method.name,"IsAlive")==0){
                if(!p.gameplay.player_alive)throw ScriptError("Player IsAlive genuine state provider is unissued");
                f->boolean=p.gameplay.player_alive(static_cast<std::int64_t>(id));f->value=NativeGameCoreSlotReadScope::Value::boolean;
            }else if(std::strcmp(method.name,"IsEverAlive")==0){
                if(!p.gameplay.player_ever_alive)throw ScriptError("Player IsEverAlive genuine state provider is unissued");
                f->boolean=p.gameplay.player_ever_alive(static_cast<std::int64_t>(id));f->value=NativeGameCoreSlotReadScope::Value::boolean;
            }else if(std::strcmp(method.name,"IsHuman")==0){
                if(!p.gameplay.player_human)throw ScriptError("Player IsHuman genuine world/rules provider is unissued");
                f->boolean=p.gameplay.player_human(static_cast<std::int64_t>(id),0);f->value=NativeGameCoreSlotReadScope::Value::boolean;
            }else if(std::strcmp(method.name,"IsTurnActive")==0){
                if(!p.map.calculated_yield)throw ScriptError("Player IsTurnActive actual imported Game/VM owner is unissued");
                p.map.calculated_yield->require_current();f->current();
                f->boolean=p.map.calculated_yield->player_is_turn_active(static_cast<std::int64_t>(id));
                p.map.calculated_yield->require_current();f->value=NativeGameCoreSlotReadScope::Value::boolean;
            }else if(std::strcmp(method.name,"GetLeaderType")==0){
                if(!p.gameplay.player_leader)throw ScriptError("Player GetLeaderType genuine retained roster supplier is unissued");
                f->integer=p.gameplay.player_leader(static_cast<std::int64_t>(id));f->value=NativeGameCoreSlotReadScope::Value::integer;
            }else if(std::strcmp(method.name,"GetCivilizationType")==0){
                if(!p.gameplay.player_civilization)throw ScriptError("Player GetCivilizationType genuine retained roster supplier is unissued");
                f->integer=p.gameplay.player_civilization(static_cast<std::int64_t>(id));f->value=NativeGameCoreSlotReadScope::Value::integer;
            }else if(std::strcmp(method.name,"IsMinorCiv")==0){
                if(!p.gameplay.player_minor)throw ScriptError("Player IsMinorCiv genuine retained roster supplier is unissued");
                f->boolean=p.gameplay.player_minor(static_cast<std::int64_t>(id));f->value=NativeGameCoreSlotReadScope::Value::boolean;
            }else if(std::strcmp(method.name,"GetMinorCivType")==0){
                if(!p.gameplay.player_minor_type)throw ScriptError("Player GetMinorCivType genuine retained roster supplier is unissued");
                f->integer=p.gameplay.player_minor_type(static_cast<std::int64_t>(id));f->value=NativeGameCoreSlotReadScope::Value::integer;
            }else throw ScriptError("Original Player method has no issued genuine engine consumer");
        }
        if(f->value==NativeGameCoreSlotReadScope::Value::integer)validate_value(f->integer);
        f->current();
    }catch(const std::exception&e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="GameCore actual instance consumer failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s: %s",method.name,p.source_error);}
    native_gamecore_slot_receiver_owned(L,Team,false,1);
    if(f->value==NativeGameCoreSlotReadScope::Value::boolean)lua_pushboolean(L,f->boolean);
    else if(f->value==NativeGameCoreSlotReadScope::Value::integer)lua_pushinteger(L,f->integer);
    else {p.record_failure();return luaL_error(L,"GameCore instance consumer did not earn a value");}
    return 1;
}
template<bool Team,std::size_t Ordinal>static int native_gamecore_slot_method_owned(lua_State*L){
    auto&p=owner(L);native_db_query_current(L);
    if(!lua_checkstack(L,12)){p.record_failure();return luaL_error(L,"GameCore actual instance getter protected stack is unavailable");}
    const int args=lua_gettop(L);lua_pushcfunction(L,(native_gamecore_slot_read_body_owned<Team,Ordinal>));lua_insert(L,1);
    int status=0;p.source_exception={};p.source_error=nullptr;
    try{{NativeGameCoreSlotReadScope lender(p,L);lender.enter();status=lua_pcall(L,args,1,0);if(!status)lender.accept(L);}}
    catch(const std::exception&e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="GameCore actual protected instance getter refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}if(status)return lua_error(L);return 1;
}

// SDK Expansion2 PushMethods; exact order, zero upvalues.
static constexpr NativeGameCoreSlotMethod native_gamecore_player_methods[]={
    {"InitCity",native_gamecore_slot_method_owned<false,0>}, // SDK line47
    {"AcquireCity",native_gamecore_slot_method_owned<false,1>}, // SDK line48
    {"KillCities",native_gamecore_slot_method_owned<false,2>}, // SDK line49
    {"GetNewCityName",native_gamecore_slot_method_owned<false,3>}, // SDK line51
    {"InitUnit",native_gamecore_slot_method_owned<false,4>}, // SDK line53
    {"InitUnitWithNameOffset",native_gamecore_slot_method_owned<false,5>}, // SDK line54
    {"DisbandUnit",native_gamecore_slot_method_owned<false,6>}, // SDK line55
    {"AddFreeUnit",native_gamecore_slot_method_owned<false,7>}, // SDK line56
    {"ChooseTech",native_gamecore_slot_method_owned<false,8>}, // SDK line58
    {"KillUnits",native_gamecore_slot_method_owned<false,9>}, // SDK line60
    {"IsHuman",native_gamecore_slot_method_owned<false,10>}, // SDK line61
    {"IsBarbarian",native_gamecore_slot_method_owned<false,11>}, // SDK line62
    {"GetName",native_gamecore_slot_method_owned<false,12>}, // SDK line63
    {"GetNameKey",native_gamecore_slot_method_owned<false,13>}, // SDK line64
    {"GetNickName",native_gamecore_slot_method_owned<false,14>}, // SDK line65
    {"GetCivilizationDescription",native_gamecore_slot_method_owned<false,15>}, // SDK line66
    {"GetCivilizationDescriptionKey",native_gamecore_slot_method_owned<false,16>}, // SDK line67
    {"GetCivilizationShortDescription",native_gamecore_slot_method_owned<false,17>}, // SDK line68
    {"GetCivilizationShortDescriptionKey",native_gamecore_slot_method_owned<false,18>}, // SDK line69
    {"GetCivilizationAdjective",native_gamecore_slot_method_owned<false,19>}, // SDK line70
    {"GetCivilizationAdjectiveKey",native_gamecore_slot_method_owned<false,20>}, // SDK line71
    {"IsWhiteFlag",native_gamecore_slot_method_owned<false,21>}, // SDK line72
    {"GetStateReligionName",native_gamecore_slot_method_owned<false,22>}, // SDK line73
    {"GetStateReligionKey",native_gamecore_slot_method_owned<false,23>}, // SDK line74
    {"GetWorstEnemyName",native_gamecore_slot_method_owned<false,24>}, // SDK line75
    {"GetArtStyleType",native_gamecore_slot_method_owned<false,25>}, // SDK line76
    {"CountCityFeatures",native_gamecore_slot_method_owned<false,26>}, // SDK line78
    {"CountNumBuildings",native_gamecore_slot_method_owned<false,27>}, // SDK line79
    {"GetNumWorldWonders",native_gamecore_slot_method_owned<false,28>}, // SDK line81
    {"ChangeNumWorldWonders",native_gamecore_slot_method_owned<false,29>}, // SDK line82
    {"GetNumWondersBeatenTo",native_gamecore_slot_method_owned<false,30>}, // SDK line83
    {"SetNumWondersBeatenTo",native_gamecore_slot_method_owned<false,31>}, // SDK line84
    {"IsCapitalConnectedToCity",native_gamecore_slot_method_owned<false,32>}, // SDK line86
    {"IsTurnActive",native_gamecore_slot_method_owned<false,33>}, // SDK line88
    {"IsSimultaneousTurns",native_gamecore_slot_method_owned<false,34>}, // SDK line89
    {"FindNewCapital",native_gamecore_slot_method_owned<false,35>}, // SDK line91
    {"CanRaze",native_gamecore_slot_method_owned<false,36>}, // SDK line92
    {"Raze",native_gamecore_slot_method_owned<false,37>}, // SDK line93
    {"Disband",native_gamecore_slot_method_owned<false,38>}, // SDK line94
    {"CanReceiveGoody",native_gamecore_slot_method_owned<false,39>}, // SDK line96
    {"ReceiveGoody",native_gamecore_slot_method_owned<false,40>}, // SDK line97
    {"DoGoody",native_gamecore_slot_method_owned<false,41>}, // SDK line98
    {"CanGetGoody",native_gamecore_slot_method_owned<false,42>}, // SDK line99
    {"CanFound",native_gamecore_slot_method_owned<false,43>}, // SDK line101
    {"Found",native_gamecore_slot_method_owned<false,44>}, // SDK line102
    {"CanTrain",native_gamecore_slot_method_owned<false,45>}, // SDK line104
    {"CanConstruct",native_gamecore_slot_method_owned<false,46>}, // SDK line105
    {"CanCreate",native_gamecore_slot_method_owned<false,47>}, // SDK line106
    {"CanPrepare",native_gamecore_slot_method_owned<false,48>}, // SDK line107
    {"CanMaintain",native_gamecore_slot_method_owned<false,49>}, // SDK line108
    {"IsCanPurchaseAnyCity",native_gamecore_slot_method_owned<false,50>}, // SDK line110
    {"GetFaithPurchaseType",native_gamecore_slot_method_owned<false,51>}, // SDK line111
    {"SetFaithPurchaseType",native_gamecore_slot_method_owned<false,52>}, // SDK line112
    {"GetFaithPurchaseIndex",native_gamecore_slot_method_owned<false,53>}, // SDK line113
    {"SetFaithPurchaseIndex",native_gamecore_slot_method_owned<false,54>}, // SDK line114
    {"IsProductionMaxedUnitClass",native_gamecore_slot_method_owned<false,55>}, // SDK line116
    {"IsProductionMaxedBuildingClass",native_gamecore_slot_method_owned<false,56>}, // SDK line117
    {"IsProductionMaxedProject",native_gamecore_slot_method_owned<false,57>}, // SDK line118
    {"GetUnitProductionNeeded",native_gamecore_slot_method_owned<false,58>}, // SDK line119
    {"GetBuildingProductionNeeded",native_gamecore_slot_method_owned<false,59>}, // SDK line120
    {"GetProjectProductionNeeded",native_gamecore_slot_method_owned<false,60>}, // SDK line121
    {"HasReadyUnit",native_gamecore_slot_method_owned<false,61>}, // SDK line123
    {"GetFirstReadyUnit",native_gamecore_slot_method_owned<false,62>}, // SDK line124
    {"GetFirstReadyUnitPlot",native_gamecore_slot_method_owned<false,63>}, // SDK line125
    {"HasBusyUnit",native_gamecore_slot_method_owned<false,64>}, // SDK line127
    {"HasBusyMovingUnit",native_gamecore_slot_method_owned<false,65>}, // SDK line128
    {"GetBuildingClassPrereqBuilding",native_gamecore_slot_method_owned<false,66>}, // SDK line130
    {"RemoveBuildingClass",native_gamecore_slot_method_owned<false,67>}, // SDK line132
    {"CanBuild",native_gamecore_slot_method_owned<false,68>}, // SDK line134
    {"IsBuildBlockedByFeature",native_gamecore_slot_method_owned<false,69>}, // SDK line135
    {"GetBestRoute",native_gamecore_slot_method_owned<false,70>}, // SDK line136
    {"GetImprovementUpgradeRate",native_gamecore_slot_method_owned<false,71>}, // SDK line137
    {"CalculateTotalYield",native_gamecore_slot_method_owned<false,72>}, // SDK line139
    {"CalculateUnitCost",native_gamecore_slot_method_owned<false,73>}, // SDK line141
    {"CalculateUnitSupply",native_gamecore_slot_method_owned<false,74>}, // SDK line142
    {"GetNumMaintenanceFreeUnits",native_gamecore_slot_method_owned<false,75>}, // SDK line144
    {"GetBuildingGoldMaintenance",native_gamecore_slot_method_owned<false,76>}, // SDK line146
    {"SetBaseBuildingGoldMaintenance",native_gamecore_slot_method_owned<false,77>}, // SDK line147
    {"ChangeBaseBuildingGoldMaintenance",native_gamecore_slot_method_owned<false,78>}, // SDK line148
    {"GetImprovementGoldMaintenance",native_gamecore_slot_method_owned<false,79>}, // SDK line150
    {"CalculateGoldRate",native_gamecore_slot_method_owned<false,80>}, // SDK line151
    {"CalculateGoldRateTimes100",native_gamecore_slot_method_owned<false,81>}, // SDK line152
    {"CalculateGrossGoldTimes100",native_gamecore_slot_method_owned<false,82>}, // SDK line153
    {"CalculateInflatedCosts",native_gamecore_slot_method_owned<false,83>}, // SDK line154
    {"CalculateResearchModifier",native_gamecore_slot_method_owned<false,84>}, // SDK line155
    {"IsResearch",native_gamecore_slot_method_owned<false,85>}, // SDK line156
    {"CanEverResearch",native_gamecore_slot_method_owned<false,86>}, // SDK line157
    {"CanResearch",native_gamecore_slot_method_owned<false,87>}, // SDK line158
    {"CanResearchForFree",native_gamecore_slot_method_owned<false,88>}, // SDK line159
    {"GetCurrentResearch",native_gamecore_slot_method_owned<false,89>}, // SDK line160
    {"IsCurrentResearchRepeat",native_gamecore_slot_method_owned<false,90>}, // SDK line161
    {"IsNoResearchAvailable",native_gamecore_slot_method_owned<false,91>}, // SDK line162
    {"GetResearchTurnsLeft",native_gamecore_slot_method_owned<false,92>}, // SDK line163
    {"GetResearchCost",native_gamecore_slot_method_owned<false,93>}, // SDK line164
    {"GetResearchProgress",native_gamecore_slot_method_owned<false,94>}, // SDK line165
    {"UnitsRequiredForGoldenAge",native_gamecore_slot_method_owned<false,95>}, // SDK line167
    {"UnitsGoldenAgeCapable",native_gamecore_slot_method_owned<false,96>}, // SDK line168
    {"UnitsGoldenAgeReady",native_gamecore_slot_method_owned<false,97>}, // SDK line169
    {"GreatGeneralThreshold",native_gamecore_slot_method_owned<false,98>}, // SDK line170
    {"GreatAdmiralThreshold",native_gamecore_slot_method_owned<false,99>}, // SDK line171
    {"SpecialistYield",native_gamecore_slot_method_owned<false,100>}, // SDK line172
    {"SetGreatGeneralCombatBonus",native_gamecore_slot_method_owned<false,101>}, // SDK line173
    {"GetGreatGeneralCombatBonus",native_gamecore_slot_method_owned<false,102>}, // SDK line174
    {"GetStartingPlot",native_gamecore_slot_method_owned<false,103>}, // SDK line176
    {"SetStartingPlot",native_gamecore_slot_method_owned<false,104>}, // SDK line177
    {"GetTotalPopulation",native_gamecore_slot_method_owned<false,105>}, // SDK line178
    {"GetAveragePopulation",native_gamecore_slot_method_owned<false,106>}, // SDK line179
    {"GetRealPopulation",native_gamecore_slot_method_owned<false,107>}, // SDK line180
    {"GetNewCityExtraPopulation",native_gamecore_slot_method_owned<false,108>}, // SDK line182
    {"ChangeNewCityExtraPopulation",native_gamecore_slot_method_owned<false,109>}, // SDK line183
    {"GetTotalLand",native_gamecore_slot_method_owned<false,110>}, // SDK line185
    {"GetTotalLandScored",native_gamecore_slot_method_owned<false,111>}, // SDK line186
    {"GetGold",native_gamecore_slot_method_owned<false,112>}, // SDK line188
    {"SetGold",native_gamecore_slot_method_owned<false,113>}, // SDK line189
    {"ChangeGold",native_gamecore_slot_method_owned<false,114>}, // SDK line190
    {"CalculateGrossGold",native_gamecore_slot_method_owned<false,115>}, // SDK line191
    {"GetLifetimeGrossGold",native_gamecore_slot_method_owned<false,116>}, // SDK line192
    {"GetGoldFromCitiesTimes100",native_gamecore_slot_method_owned<false,117>}, // SDK line193
    {"GetGoldFromCitiesMinusTradeRoutesTimes100",native_gamecore_slot_method_owned<false,118>}, // SDK line194
    {"GetGoldPerTurnFromDiplomacy",native_gamecore_slot_method_owned<false,119>}, // SDK line195
    {"GetCityConnectionRouteGoldTimes100",native_gamecore_slot_method_owned<false,120>}, // SDK line196
    {"GetCityConnectionGold",native_gamecore_slot_method_owned<false,121>}, // SDK line197
    {"GetCityConnectionGoldTimes100",native_gamecore_slot_method_owned<false,122>}, // SDK line198
    {"GetGoldPerTurnFromReligion",native_gamecore_slot_method_owned<false,123>}, // SDK line199
    {"GetGoldPerTurnFromTradeRoutes",native_gamecore_slot_method_owned<false,124>}, // SDK line200
    {"GetGoldPerTurnFromTradeRoutesTimes100",native_gamecore_slot_method_owned<false,125>}, // SDK line201
    {"GetGoldPerTurnFromTraits",native_gamecore_slot_method_owned<false,126>}, // SDK line202
    {"GetTotalJONSCulturePerTurn",native_gamecore_slot_method_owned<false,127>}, // SDK line206
    {"GetJONSCulturePerTurnFromCities",native_gamecore_slot_method_owned<false,128>}, // SDK line208
    {"GetJONSCulturePerTurnFromExcessHappiness",native_gamecore_slot_method_owned<false,129>}, // SDK line210
    {"GetJONSCulturePerTurnFromTraits",native_gamecore_slot_method_owned<false,130>}, // SDK line211
    {"GetCultureWonderMultiplier",native_gamecore_slot_method_owned<false,131>}, // SDK line213
    {"GetJONSCulturePerTurnForFree",native_gamecore_slot_method_owned<false,132>}, // SDK line215
    {"ChangeJONSCulturePerTurnForFree",native_gamecore_slot_method_owned<false,133>}, // SDK line216
    {"GetJONSCulturePerTurnFromMinorCivs",native_gamecore_slot_method_owned<false,134>}, // SDK line218
    {"ChangeJONSCulturePerTurnFromMinorCivs",native_gamecore_slot_method_owned<false,135>}, // SDK line219
    {"GetCulturePerTurnFromMinorCivs",native_gamecore_slot_method_owned<false,136>}, // SDK line220
    {"GetCulturePerTurnFromMinor",native_gamecore_slot_method_owned<false,137>}, // SDK line221
    {"GetCulturePerTurnFromReligion",native_gamecore_slot_method_owned<false,138>}, // SDK line223
    {"GetCulturePerTurnFromBonusTurns",native_gamecore_slot_method_owned<false,139>}, // SDK line224
    {"GetCultureCityModifier",native_gamecore_slot_method_owned<false,140>}, // SDK line225
    {"GetJONSCulture",native_gamecore_slot_method_owned<false,141>}, // SDK line227
    {"SetJONSCulture",native_gamecore_slot_method_owned<false,142>}, // SDK line228
    {"ChangeJONSCulture",native_gamecore_slot_method_owned<false,143>}, // SDK line229
    {"GetJONSCultureEverGenerated",native_gamecore_slot_method_owned<false,144>}, // SDK line231
    {"GetLastTurnLifetimeCulture",native_gamecore_slot_method_owned<false,145>}, // SDK line233
    {"GetInfluenceOn",native_gamecore_slot_method_owned<false,146>}, // SDK line234
    {"GetLastTurnInfluenceOn",native_gamecore_slot_method_owned<false,147>}, // SDK line235
    {"GetInfluencePerTurn",native_gamecore_slot_method_owned<false,148>}, // SDK line236
    {"GetInfluenceLevel",native_gamecore_slot_method_owned<false,149>}, // SDK line237
    {"GetInfluenceTrend",native_gamecore_slot_method_owned<false,150>}, // SDK line238
    {"GetTurnsToInfluential",native_gamecore_slot_method_owned<false,151>}, // SDK line239
    {"GetNumCivsInfluentialOn",native_gamecore_slot_method_owned<false,152>}, // SDK line240
    {"GetNumCivsToBeInfluentialOn",native_gamecore_slot_method_owned<false,153>}, // SDK line241
    {"GetInfluenceTradeRouteScienceBonus",native_gamecore_slot_method_owned<false,154>}, // SDK line242
    {"GetInfluenceCityStateSpyRankBonus",native_gamecore_slot_method_owned<false,155>}, // SDK line243
    {"GetInfluenceMajorCivSpyRankBonus",native_gamecore_slot_method_owned<false,156>}, // SDK line244
    {"GetInfluenceSpyRankTooltip",native_gamecore_slot_method_owned<false,157>}, // SDK line245
    {"GetTourism",native_gamecore_slot_method_owned<false,158>}, // SDK line246
    {"GetTourismModifierWith",native_gamecore_slot_method_owned<false,159>}, // SDK line247
    {"GetTourismModifierWithTooltip",native_gamecore_slot_method_owned<false,160>}, // SDK line248
    {"GetPublicOpinionType",native_gamecore_slot_method_owned<false,161>}, // SDK line249
    {"GetPublicOpinionPreferredIdeology",native_gamecore_slot_method_owned<false,162>}, // SDK line250
    {"GetPublicOpinionTooltip",native_gamecore_slot_method_owned<false,163>}, // SDK line251
    {"GetPublicOpinionUnhappiness",native_gamecore_slot_method_owned<false,164>}, // SDK line252
    {"GetPublicOpinionUnhappinessTooltip",native_gamecore_slot_method_owned<false,165>}, // SDK line253
    {"HasAvailableGreatWorkSlot",native_gamecore_slot_method_owned<false,166>}, // SDK line255
    {"GetCityOfClosestGreatWorkSlot",native_gamecore_slot_method_owned<false,167>}, // SDK line256
    {"GetBuildingOfClosestGreatWorkSlot",native_gamecore_slot_method_owned<false,168>}, // SDK line257
    {"GetNextDigCompletePlot",native_gamecore_slot_method_owned<false,169>}, // SDK line258
    {"GetWrittenArtifactCulture",native_gamecore_slot_method_owned<false,170>}, // SDK line259
    {"GetNumGreatWorks",native_gamecore_slot_method_owned<false,171>}, // SDK line260
    {"GetNumGreatWorkSlots",native_gamecore_slot_method_owned<false,172>}, // SDK line261
    {"GetFaith",native_gamecore_slot_method_owned<false,173>}, // SDK line265
    {"SetFaith",native_gamecore_slot_method_owned<false,174>}, // SDK line266
    {"ChangeFaith",native_gamecore_slot_method_owned<false,175>}, // SDK line267
    {"GetTotalFaithPerTurn",native_gamecore_slot_method_owned<false,176>}, // SDK line268
    {"GetFaithPerTurnFromCities",native_gamecore_slot_method_owned<false,177>}, // SDK line269
    {"GetFaithPerTurnFromMinorCivs",native_gamecore_slot_method_owned<false,178>}, // SDK line270
    {"GetFaithPerTurnFromReligion",native_gamecore_slot_method_owned<false,179>}, // SDK line271
    {"HasCreatedPantheon",native_gamecore_slot_method_owned<false,180>}, // SDK line272
    {"GetBeliefInPantheon",native_gamecore_slot_method_owned<false,181>}, // SDK line273
    {"HasCreatedReligion",native_gamecore_slot_method_owned<false,182>}, // SDK line274
    {"CanCreatePantheon",native_gamecore_slot_method_owned<false,183>}, // SDK line275
    {"GetReligionCreatedByPlayer",native_gamecore_slot_method_owned<false,184>}, // SDK line276
    {"GetFoundedReligionEnemyCityCombatMod",native_gamecore_slot_method_owned<false,185>}, // SDK line277
    {"GetFoundedReligionFriendlyCityCombatMod",native_gamecore_slot_method_owned<false,186>}, // SDK line278
    {"GetMinimumFaithNextGreatProphet",native_gamecore_slot_method_owned<false,187>}, // SDK line279
    {"HasReligionInMostCities",native_gamecore_slot_method_owned<false,188>}, // SDK line280
    {"DoesUnitPassFaithPurchaseCheck",native_gamecore_slot_method_owned<false,189>}, // SDK line281
    {"GetHappiness",native_gamecore_slot_method_owned<false,190>}, // SDK line285
    {"SetHappiness",native_gamecore_slot_method_owned<false,191>}, // SDK line286
    {"GetExcessHappiness",native_gamecore_slot_method_owned<false,192>}, // SDK line288
    {"IsEmpireUnhappy",native_gamecore_slot_method_owned<false,193>}, // SDK line289
    {"IsEmpireVeryUnhappy",native_gamecore_slot_method_owned<false,194>}, // SDK line290
    {"IsEmpireSuperUnhappy",native_gamecore_slot_method_owned<false,195>}, // SDK line291
    {"GetHappinessFromPolicies",native_gamecore_slot_method_owned<false,196>}, // SDK line293
    {"GetHappinessFromCities",native_gamecore_slot_method_owned<false,197>}, // SDK line294
    {"GetHappinessFromBuildings",native_gamecore_slot_method_owned<false,198>}, // SDK line295
    {"GetExtraHappinessPerCity",native_gamecore_slot_method_owned<false,199>}, // SDK line297
    {"ChangeExtraHappinessPerCity",native_gamecore_slot_method_owned<false,200>}, // SDK line298
    {"GetHappinessFromResources",native_gamecore_slot_method_owned<false,201>}, // SDK line300
    {"GetHappinessFromResourceVariety",native_gamecore_slot_method_owned<false,202>}, // SDK line301
    {"GetExtraHappinessPerLuxury",native_gamecore_slot_method_owned<false,203>}, // SDK line302
    {"GetHappinessFromReligion",native_gamecore_slot_method_owned<false,204>}, // SDK line303
    {"GetHappinessFromNaturalWonders",native_gamecore_slot_method_owned<false,205>}, // SDK line304
    {"GetHappinessFromLeagues",native_gamecore_slot_method_owned<false,206>}, // SDK line305
    {"GetUnhappiness",native_gamecore_slot_method_owned<false,207>}, // SDK line307
    {"GetUnhappinessForecast",native_gamecore_slot_method_owned<false,208>}, // SDK line308
    {"GetUnhappinessFromCityForUI",native_gamecore_slot_method_owned<false,209>}, // SDK line310
    {"GetUnhappinessFromCityCount",native_gamecore_slot_method_owned<false,210>}, // SDK line312
    {"GetUnhappinessFromCapturedCityCount",native_gamecore_slot_method_owned<false,211>}, // SDK line313
    {"GetUnhappinessFromCityPopulation",native_gamecore_slot_method_owned<false,212>}, // SDK line314
    {"GetUnhappinessFromCitySpecialists",native_gamecore_slot_method_owned<false,213>}, // SDK line315
    {"GetUnhappinessFromOccupiedCities",native_gamecore_slot_method_owned<false,214>}, // SDK line316
    {"GetUnhappinessFromPuppetCityPopulation",native_gamecore_slot_method_owned<false,215>}, // SDK line317
    {"GetUnhappinessFromPublicOpinion",native_gamecore_slot_method_owned<false,216>}, // SDK line318
    {"GetUnhappinessFromUnits",native_gamecore_slot_method_owned<false,217>}, // SDK line319
    {"ChangeUnhappinessFromUnits",native_gamecore_slot_method_owned<false,218>}, // SDK line320
    {"GetUnhappinessMod",native_gamecore_slot_method_owned<false,219>}, // SDK line322
    {"GetCityCountUnhappinessMod",native_gamecore_slot_method_owned<false,220>}, // SDK line323
    {"GetOccupiedPopulationUnhappinessMod",native_gamecore_slot_method_owned<false,221>}, // SDK line324
    {"GetCapitalUnhappinessMod",native_gamecore_slot_method_owned<false,222>}, // SDK line325
    {"GetTraitCityUnhappinessMod",native_gamecore_slot_method_owned<false,223>}, // SDK line326
    {"GetTraitPopUnhappinessMod",native_gamecore_slot_method_owned<false,224>}, // SDK line327
    {"IsHalfSpecialistUnhappiness",native_gamecore_slot_method_owned<false,225>}, // SDK line328
    {"GetHappinessPerGarrisonedUnit",native_gamecore_slot_method_owned<false,226>}, // SDK line330
    {"SetHappinessPerGarrisonedUnit",native_gamecore_slot_method_owned<false,227>}, // SDK line331
    {"ChangeHappinessPerGarrisonedUnit",native_gamecore_slot_method_owned<false,228>}, // SDK line332
    {"GetHappinessFromTradeRoutes",native_gamecore_slot_method_owned<false,229>}, // SDK line334
    {"GetHappinessPerTradeRoute",native_gamecore_slot_method_owned<false,230>}, // SDK line335
    {"SetHappinessPerTradeRoute",native_gamecore_slot_method_owned<false,231>}, // SDK line336
    {"ChangeHappinessPerTradeRoute",native_gamecore_slot_method_owned<false,232>}, // SDK line337
    {"GetCityConnectionTradeRouteGoldModifier",native_gamecore_slot_method_owned<false,233>}, // SDK line339
    {"GetHappinessFromMinorCivs",native_gamecore_slot_method_owned<false,234>}, // SDK line341
    {"GetHappinessFromMinor",native_gamecore_slot_method_owned<false,235>}, // SDK line342
    {"GetBarbarianCombatBonus",native_gamecore_slot_method_owned<false,236>}, // SDK line346
    {"SetBarbarianCombatBonus",native_gamecore_slot_method_owned<false,237>}, // SDK line347
    {"ChangeBarbarianCombatBonus",native_gamecore_slot_method_owned<false,238>}, // SDK line348
    {"GetCombatBonusVsHigherTech",native_gamecore_slot_method_owned<false,239>}, // SDK line349
    {"GetCombatBonusVsLargerCiv",native_gamecore_slot_method_owned<false,240>}, // SDK line350
    {"GetGarrisonedCityRangeStrikeModifier",native_gamecore_slot_method_owned<false,241>}, // SDK line352
    {"ChangeGarrisonedCityRangeStrikeModifier",native_gamecore_slot_method_owned<false,242>}, // SDK line353
    {"IsAlwaysSeeBarbCamps",native_gamecore_slot_method_owned<false,243>}, // SDK line355
    {"SetAlwaysSeeBarbCampsCount",native_gamecore_slot_method_owned<false,244>}, // SDK line356
    {"ChangeAlwaysSeeBarbCampsCount",native_gamecore_slot_method_owned<false,245>}, // SDK line357
    {"IsPolicyBlocked",native_gamecore_slot_method_owned<false,246>}, // SDK line359
    {"IsPolicyBranchBlocked",native_gamecore_slot_method_owned<false,247>}, // SDK line360
    {"IsPolicyBranchUnlocked",native_gamecore_slot_method_owned<false,248>}, // SDK line361
    {"SetPolicyBranchUnlocked",native_gamecore_slot_method_owned<false,249>}, // SDK line362
    {"GetNumPolicyBranchesUnlocked",native_gamecore_slot_method_owned<false,250>}, // SDK line363
    {"GetPolicyBranchChosen",native_gamecore_slot_method_owned<false,251>}, // SDK line364
    {"GetNumPolicyBranchesAllowed",native_gamecore_slot_method_owned<false,252>}, // SDK line365
    {"GetNumPolicies",native_gamecore_slot_method_owned<false,253>}, // SDK line366
    {"GetNumPoliciesInBranch",native_gamecore_slot_method_owned<false,254>}, // SDK line367
    {"HasPolicy",native_gamecore_slot_method_owned<false,255>}, // SDK line368
    {"SetHasPolicy",native_gamecore_slot_method_owned<false,256>}, // SDK line369
    {"GetNextPolicyCost",native_gamecore_slot_method_owned<false,257>}, // SDK line370
    {"CanAdoptPolicy",native_gamecore_slot_method_owned<false,258>}, // SDK line371
    {"DoAdoptPolicy",native_gamecore_slot_method_owned<false,259>}, // SDK line372
    {"CanUnlockPolicyBranch",native_gamecore_slot_method_owned<false,260>}, // SDK line373
    {"GetDominantPolicyBranchForTitle",native_gamecore_slot_method_owned<false,261>}, // SDK line374
    {"GetLateGamePolicyTree",native_gamecore_slot_method_owned<false,262>}, // SDK line375
    {"GetBranchPicked1",native_gamecore_slot_method_owned<false,263>}, // SDK line376
    {"GetBranchPicked2",native_gamecore_slot_method_owned<false,264>}, // SDK line377
    {"GetBranchPicked3",native_gamecore_slot_method_owned<false,265>}, // SDK line378
    {"GetPolicyCatchSpiesModifier",native_gamecore_slot_method_owned<false,266>}, // SDK line380
    {"GetNumPolicyBranchesFinished",native_gamecore_slot_method_owned<false,267>}, // SDK line382
    {"IsPolicyBranchFinished",native_gamecore_slot_method_owned<false,268>}, // SDK line383
    {"GetAvailableTenets",native_gamecore_slot_method_owned<false,269>}, // SDK line385
    {"GetTenet",native_gamecore_slot_method_owned<false,270>}, // SDK line386
    {"IsAnarchy",native_gamecore_slot_method_owned<false,271>}, // SDK line388
    {"GetAnarchyNumTurns",native_gamecore_slot_method_owned<false,272>}, // SDK line389
    {"SetAnarchyNumTurns",native_gamecore_slot_method_owned<false,273>}, // SDK line390
    {"ChangeAnarchyNumTurns",native_gamecore_slot_method_owned<false,274>}, // SDK line391
    {"GetAdvancedStartPoints",native_gamecore_slot_method_owned<false,275>}, // SDK line393
    {"SetAdvancedStartPoints",native_gamecore_slot_method_owned<false,276>}, // SDK line394
    {"ChangeAdvancedStartPoints",native_gamecore_slot_method_owned<false,277>}, // SDK line395
    {"GetAdvancedStartUnitCost",native_gamecore_slot_method_owned<false,278>}, // SDK line396
    {"GetAdvancedStartCityCost",native_gamecore_slot_method_owned<false,279>}, // SDK line397
    {"GetAdvancedStartPopCost",native_gamecore_slot_method_owned<false,280>}, // SDK line398
    {"GetAdvancedStartBuildingCost",native_gamecore_slot_method_owned<false,281>}, // SDK line399
    {"GetAdvancedStartImprovementCost",native_gamecore_slot_method_owned<false,282>}, // SDK line400
    {"GetAdvancedStartRouteCost",native_gamecore_slot_method_owned<false,283>}, // SDK line401
    {"GetAdvancedStartTechCost",native_gamecore_slot_method_owned<false,284>}, // SDK line402
    {"GetAdvancedStartVisibilityCost",native_gamecore_slot_method_owned<false,285>}, // SDK line403
    {"GetAttackBonusTurns",native_gamecore_slot_method_owned<false,286>}, // SDK line405
    {"GetCultureBonusTurns",native_gamecore_slot_method_owned<false,287>}, // SDK line406
    {"GetTourismBonusTurns",native_gamecore_slot_method_owned<false,288>}, // SDK line407
    {"GetGoldenAgeProgressThreshold",native_gamecore_slot_method_owned<false,289>}, // SDK line409
    {"GetGoldenAgeProgressMeter",native_gamecore_slot_method_owned<false,290>}, // SDK line410
    {"SetGoldenAgeProgressMeter",native_gamecore_slot_method_owned<false,291>}, // SDK line411
    {"ChangeGoldenAgeProgressMeter",native_gamecore_slot_method_owned<false,292>}, // SDK line412
    {"GetNumGoldenAges",native_gamecore_slot_method_owned<false,293>}, // SDK line413
    {"SetNumGoldenAges",native_gamecore_slot_method_owned<false,294>}, // SDK line414
    {"ChangeNumGoldenAges",native_gamecore_slot_method_owned<false,295>}, // SDK line415
    {"GetGoldenAgeTurns",native_gamecore_slot_method_owned<false,296>}, // SDK line416
    {"GetGoldenAgeLength",native_gamecore_slot_method_owned<false,297>}, // SDK line417
    {"IsGoldenAge",native_gamecore_slot_method_owned<false,298>}, // SDK line418
    {"ChangeGoldenAgeTurns",native_gamecore_slot_method_owned<false,299>}, // SDK line419
    {"GetNumUnitGoldenAges",native_gamecore_slot_method_owned<false,300>}, // SDK line420
    {"ChangeNumUnitGoldenAges",native_gamecore_slot_method_owned<false,301>}, // SDK line421
    {"GetStrikeTurns",native_gamecore_slot_method_owned<false,302>}, // SDK line422
    {"GetGoldenAgeModifier",native_gamecore_slot_method_owned<false,303>}, // SDK line423
    {"GetGoldenAgeTourismModifier",native_gamecore_slot_method_owned<false,304>}, // SDK line424
    {"GetGoldenAgeGreatWriterRateModifier",native_gamecore_slot_method_owned<false,305>}, // SDK line425
    {"GetGoldenAgeGreatArtistRateModifier",native_gamecore_slot_method_owned<false,306>}, // SDK line426
    {"GetGoldenAgeGreatMusicianRateModifier",native_gamecore_slot_method_owned<false,307>}, // SDK line427
    {"GetHurryModifier",native_gamecore_slot_method_owned<false,308>}, // SDK line429
    {"CreateGreatGeneral",native_gamecore_slot_method_owned<false,309>}, // SDK line431
    {"GetGreatPeopleCreated",native_gamecore_slot_method_owned<false,310>}, // SDK line432
    {"GetGreatGeneralsCreated",native_gamecore_slot_method_owned<false,311>}, // SDK line433
    {"GetGreatPeopleThresholdModifier",native_gamecore_slot_method_owned<false,312>}, // SDK line434
    {"GetGreatGeneralsThresholdModifier",native_gamecore_slot_method_owned<false,313>}, // SDK line435
    {"GetGreatAdmiralsThresholdModifier",native_gamecore_slot_method_owned<false,314>}, // SDK line436
    {"GetGreatPeopleRateModifier",native_gamecore_slot_method_owned<false,315>}, // SDK line437
    {"GetGreatGeneralRateModifier",native_gamecore_slot_method_owned<false,316>}, // SDK line438
    {"GetDomesticGreatGeneralRateModifier",native_gamecore_slot_method_owned<false,317>}, // SDK line439
    {"GetGreatWriterRateModifier",native_gamecore_slot_method_owned<false,318>}, // SDK line440
    {"GetGreatArtistRateModifier",native_gamecore_slot_method_owned<false,319>}, // SDK line441
    {"GetGreatMusicianRateModifier",native_gamecore_slot_method_owned<false,320>}, // SDK line442
    {"GetGreatScientistRateModifier",native_gamecore_slot_method_owned<false,321>}, // SDK line443
    {"GetGreatMerchantRateModifier",native_gamecore_slot_method_owned<false,322>}, // SDK line444
    {"GetGreatEngineerRateModifier",native_gamecore_slot_method_owned<false,323>}, // SDK line445
    {"GetPolicyGreatPeopleRateModifier",native_gamecore_slot_method_owned<false,324>}, // SDK line447
    {"GetPolicyGreatWriterRateModifier",native_gamecore_slot_method_owned<false,325>}, // SDK line448
    {"GetPolicyGreatArtistRateModifier",native_gamecore_slot_method_owned<false,326>}, // SDK line449
    {"GetPolicyGreatMusicianRateModifier",native_gamecore_slot_method_owned<false,327>}, // SDK line450
    {"GetPolicyGreatScientistRateModifier",native_gamecore_slot_method_owned<false,328>}, // SDK line451
    {"GetPolicyGreatMerchantRateModifier",native_gamecore_slot_method_owned<false,329>}, // SDK line452
    {"GetPolicyGreatEngineerRateModifier",native_gamecore_slot_method_owned<false,330>}, // SDK line453
    {"GetProductionModifier",native_gamecore_slot_method_owned<false,331>}, // SDK line455
    {"GetUnitProductionModifier",native_gamecore_slot_method_owned<false,332>}, // SDK line456
    {"GetBuildingProductionModifier",native_gamecore_slot_method_owned<false,333>}, // SDK line457
    {"GetProjectProductionModifier",native_gamecore_slot_method_owned<false,334>}, // SDK line458
    {"GetSpecialistProductionModifier",native_gamecore_slot_method_owned<false,335>}, // SDK line459
    {"GetMaxGlobalBuildingProductionModifier",native_gamecore_slot_method_owned<false,336>}, // SDK line460
    {"GetMaxTeamBuildingProductionModifier",native_gamecore_slot_method_owned<false,337>}, // SDK line461
    {"GetMaxPlayerBuildingProductionModifier",native_gamecore_slot_method_owned<false,338>}, // SDK line462
    {"GetFreeExperience",native_gamecore_slot_method_owned<false,339>}, // SDK line463
    {"GetFeatureProductionModifier",native_gamecore_slot_method_owned<false,340>}, // SDK line464
    {"GetWorkerSpeedModifier",native_gamecore_slot_method_owned<false,341>}, // SDK line465
    {"GetImprovementUpgradeRateModifier",native_gamecore_slot_method_owned<false,342>}, // SDK line466
    {"GetMilitaryProductionModifier",native_gamecore_slot_method_owned<false,343>}, // SDK line467
    {"GetSpaceProductionModifier",native_gamecore_slot_method_owned<false,344>}, // SDK line468
    {"GetSettlerProductionModifier",native_gamecore_slot_method_owned<false,345>}, // SDK line469
    {"GetCapitalSettlerProductionModifier",native_gamecore_slot_method_owned<false,346>}, // SDK line470
    {"GetWonderProductionModifier",native_gamecore_slot_method_owned<false,347>}, // SDK line471
    {"GetUnitProductionMaintenanceMod",native_gamecore_slot_method_owned<false,348>}, // SDK line473
    {"GetNumUnitsSupplied",native_gamecore_slot_method_owned<false,349>}, // SDK line474
    {"GetNumUnitsSuppliedByHandicap",native_gamecore_slot_method_owned<false,350>}, // SDK line475
    {"GetNumUnitsSuppliedByCities",native_gamecore_slot_method_owned<false,351>}, // SDK line476
    {"GetNumUnitsSuppliedByPopulation",native_gamecore_slot_method_owned<false,352>}, // SDK line477
    {"GetNumUnitsOutOfSupply",native_gamecore_slot_method_owned<false,353>}, // SDK line478
    {"GetCityDefenseModifier",native_gamecore_slot_method_owned<false,354>}, // SDK line480
    {"GetNumNukeUnits",native_gamecore_slot_method_owned<false,355>}, // SDK line481
    {"GetNumOutsideUnits",native_gamecore_slot_method_owned<false,356>}, // SDK line482
    {"GetGoldPerUnit",native_gamecore_slot_method_owned<false,357>}, // SDK line484
    {"ChangeGoldPerUnitTimes100",native_gamecore_slot_method_owned<false,358>}, // SDK line485
    {"GetGoldPerMilitaryUnit",native_gamecore_slot_method_owned<false,359>}, // SDK line486
    {"GetExtraUnitCost",native_gamecore_slot_method_owned<false,360>}, // SDK line487
    {"GetNumMilitaryUnits",native_gamecore_slot_method_owned<false,361>}, // SDK line488
    {"GetHappyPerMilitaryUnit",native_gamecore_slot_method_owned<false,362>}, // SDK line489
    {"IsMilitaryFoodProduction",native_gamecore_slot_method_owned<false,363>}, // SDK line490
    {"GetHighestUnitLevel",native_gamecore_slot_method_owned<false,364>}, // SDK line491
    {"GetConscriptCount",native_gamecore_slot_method_owned<false,365>}, // SDK line493
    {"SetConscriptCount",native_gamecore_slot_method_owned<false,366>}, // SDK line494
    {"ChangeConscriptCount",native_gamecore_slot_method_owned<false,367>}, // SDK line495
    {"GetMaxConscript",native_gamecore_slot_method_owned<false,368>}, // SDK line497
    {"GetOverflowResearch",native_gamecore_slot_method_owned<false,369>}, // SDK line498
    {"GetExpInBorderModifier",native_gamecore_slot_method_owned<false,370>}, // SDK line499
    {"GetLevelExperienceModifier",native_gamecore_slot_method_owned<false,371>}, // SDK line501
    {"GetCultureBombTimer",native_gamecore_slot_method_owned<false,372>}, // SDK line503
    {"GetConversionTimer",native_gamecore_slot_method_owned<false,373>}, // SDK line504
    {"GetCapitalCity",native_gamecore_slot_method_owned<false,374>}, // SDK line506
    {"IsHasLostCapital",native_gamecore_slot_method_owned<false,375>}, // SDK line507
    {"GetCitiesLost",native_gamecore_slot_method_owned<false,376>}, // SDK line508
    {"GetPower",native_gamecore_slot_method_owned<false,377>}, // SDK line510
    {"GetMilitaryMight",native_gamecore_slot_method_owned<false,378>}, // SDK line511
    {"GetTotalTimePlayed",native_gamecore_slot_method_owned<false,379>}, // SDK line512
    {"GetScore",native_gamecore_slot_method_owned<false,380>}, // SDK line514
    {"GetScoreFromCities",native_gamecore_slot_method_owned<false,381>}, // SDK line515
    {"GetScoreFromPopulation",native_gamecore_slot_method_owned<false,382>}, // SDK line516
    {"GetScoreFromLand",native_gamecore_slot_method_owned<false,383>}, // SDK line517
    {"GetScoreFromWonders",native_gamecore_slot_method_owned<false,384>}, // SDK line518
    {"GetScoreFromTechs",native_gamecore_slot_method_owned<false,385>}, // SDK line519
    {"GetScoreFromFutureTech",native_gamecore_slot_method_owned<false,386>}, // SDK line520
    {"ChangeScoreFromFutureTech",native_gamecore_slot_method_owned<false,387>}, // SDK line521
    {"GetScoreFromPolicies",native_gamecore_slot_method_owned<false,388>}, // SDK line522
    {"GetScoreFromGreatWorks",native_gamecore_slot_method_owned<false,389>}, // SDK line523
    {"GetScoreFromReligion",native_gamecore_slot_method_owned<false,390>}, // SDK line524
    {"GetScoreFromScenario1",native_gamecore_slot_method_owned<false,391>}, // SDK line525
    {"ChangeScoreFromScenario1",native_gamecore_slot_method_owned<false,392>}, // SDK line526
    {"GetScoreFromScenario2",native_gamecore_slot_method_owned<false,393>}, // SDK line527
    {"ChangeScoreFromScenario2",native_gamecore_slot_method_owned<false,394>}, // SDK line528
    {"GetScoreFromScenario3",native_gamecore_slot_method_owned<false,395>}, // SDK line529
    {"ChangeScoreFromScenario3",native_gamecore_slot_method_owned<false,396>}, // SDK line530
    {"GetScoreFromScenario4",native_gamecore_slot_method_owned<false,397>}, // SDK line531
    {"ChangeScoreFromScenario4",native_gamecore_slot_method_owned<false,398>}, // SDK line532
    {"IsGoldenAgeCultureBonusDisabled",native_gamecore_slot_method_owned<false,399>}, // SDK line534
    {"IsMinorCiv",native_gamecore_slot_method_owned<false,400>}, // SDK line536
    {"GetMinorCivType",native_gamecore_slot_method_owned<false,401>}, // SDK line537
    {"GetMinorCivTrait",native_gamecore_slot_method_owned<false,402>}, // SDK line538
    {"GetPersonality",native_gamecore_slot_method_owned<false,403>}, // SDK line539
    {"IsMinorCivHasUniqueUnit",native_gamecore_slot_method_owned<false,404>}, // SDK line540
    {"GetMinorCivUniqueUnit",native_gamecore_slot_method_owned<false,405>}, // SDK line541
    {"SetMinorCivUniqueUnit",native_gamecore_slot_method_owned<false,406>}, // SDK line542
    {"GetAlly",native_gamecore_slot_method_owned<false,407>}, // SDK line543
    {"GetAlliedTurns",native_gamecore_slot_method_owned<false,408>}, // SDK line544
    {"IsFriends",native_gamecore_slot_method_owned<false,409>}, // SDK line545
    {"IsAllies",native_gamecore_slot_method_owned<false,410>}, // SDK line546
    {"IsPlayerHasOpenBorders",native_gamecore_slot_method_owned<false,411>}, // SDK line547
    {"IsPlayerHasOpenBordersAutomatically",native_gamecore_slot_method_owned<false,412>}, // SDK line548
    {"GetFriendshipChangePerTurnTimes100",native_gamecore_slot_method_owned<false,413>}, // SDK line549
    {"GetMinorCivFriendshipWithMajor",native_gamecore_slot_method_owned<false,414>}, // SDK line550
    {"ChangeMinorCivFriendshipWithMajor",native_gamecore_slot_method_owned<false,415>}, // SDK line551
    {"GetMinorCivFriendshipAnchorWithMajor",native_gamecore_slot_method_owned<false,416>}, // SDK line552
    {"GetMinorCivFriendshipLevelWithMajor",native_gamecore_slot_method_owned<false,417>}, // SDK line553
    {"GetActiveQuestForPlayer",native_gamecore_slot_method_owned<false,418>}, // SDK line554
    {"IsMinorCivActiveQuestForPlayer",native_gamecore_slot_method_owned<false,419>}, // SDK line555
    {"GetMinorCivNumActiveQuestsForPlayer",native_gamecore_slot_method_owned<false,420>}, // SDK line556
    {"IsMinorCivDisplayedQuestForPlayer",native_gamecore_slot_method_owned<false,421>}, // SDK line557
    {"GetMinorCivNumDisplayedQuestsForPlayer",native_gamecore_slot_method_owned<false,422>}, // SDK line558
    {"GetQuestData1",native_gamecore_slot_method_owned<false,423>}, // SDK line559
    {"GetQuestData2",native_gamecore_slot_method_owned<false,424>}, // SDK line560
    {"GetQuestTurnsRemaining",native_gamecore_slot_method_owned<false,425>}, // SDK line561
    {"IsMinorCivContestLeader",native_gamecore_slot_method_owned<false,426>}, // SDK line562
    {"GetMinorCivContestValueForLeader",native_gamecore_slot_method_owned<false,427>}, // SDK line563
    {"GetMinorCivContestValueForPlayer",native_gamecore_slot_method_owned<false,428>}, // SDK line564
    {"IsMinorCivUnitSpawningDisabled",native_gamecore_slot_method_owned<false,429>}, // SDK line565
    {"IsMinorCivRouteEstablishedWithMajor",native_gamecore_slot_method_owned<false,430>}, // SDK line566
    {"IsMinorWarQuestWithMajorActive",native_gamecore_slot_method_owned<false,431>}, // SDK line567
    {"GetMinorWarQuestWithMajorRemainingCount",native_gamecore_slot_method_owned<false,432>}, // SDK line568
    {"IsProxyWarActiveForMajor",native_gamecore_slot_method_owned<false,433>}, // SDK line569
    {"IsThreateningBarbariansEventActiveForPlayer",native_gamecore_slot_method_owned<false,434>}, // SDK line570
    {"GetTurnsSinceThreatenedByBarbarians",native_gamecore_slot_method_owned<false,435>}, // SDK line571
    {"GetTurnsSinceThreatenedAnnouncement",native_gamecore_slot_method_owned<false,436>}, // SDK line572
    {"GetFriendshipFromGoldGift",native_gamecore_slot_method_owned<false,437>}, // SDK line573
    {"GetFriendshipNeededForNextLevel",native_gamecore_slot_method_owned<false,438>}, // SDK line574
    {"GetMinorCivFavoriteMajor",native_gamecore_slot_method_owned<false,439>}, // SDK line575
    {"GetMinorCivScienceFriendshipBonus",native_gamecore_slot_method_owned<false,440>}, // SDK line576
    {"GetMinorCivCultureFriendshipBonus",native_gamecore_slot_method_owned<false,441>}, // SDK line577
    {"GetMinorCivCurrentCultureFlatBonus",native_gamecore_slot_method_owned<false,442>}, // SDK line578
    {"GetMinorCivCurrentCulturePerBuildingBonus",native_gamecore_slot_method_owned<false,443>}, // SDK line579
    {"GetCurrentCultureBonus",native_gamecore_slot_method_owned<false,444>}, // SDK line580
    {"GetMinorCivCurrentCultureBonus",native_gamecore_slot_method_owned<false,445>}, // SDK line581
    {"GetMinorCivHappinessFriendshipBonus",native_gamecore_slot_method_owned<false,446>}, // SDK line582
    {"GetMinorCivCurrentHappinessFlatBonus",native_gamecore_slot_method_owned<false,447>}, // SDK line583
    {"GetMinorCivCurrentHappinessPerLuxuryBonus",native_gamecore_slot_method_owned<false,448>}, // SDK line584
    {"GetMinorCivCurrentHappinessBonus",native_gamecore_slot_method_owned<false,449>}, // SDK line585
    {"GetMinorCivCurrentFaithBonus",native_gamecore_slot_method_owned<false,450>}, // SDK line586
    {"GetCurrentCapitalFoodBonus",native_gamecore_slot_method_owned<false,451>}, // SDK line587
    {"GetCurrentOtherCityFoodBonus",native_gamecore_slot_method_owned<false,452>}, // SDK line588
    {"GetCurrentSpawnEstimate",native_gamecore_slot_method_owned<false,453>}, // SDK line589
    {"GetCurrentScienceFriendshipBonusTimes100",native_gamecore_slot_method_owned<false,454>}, // SDK line590
    {"IsPeaceBlocked",native_gamecore_slot_method_owned<false,455>}, // SDK line591
    {"IsMinorPermanentWar",native_gamecore_slot_method_owned<false,456>}, // SDK line592
    {"GetNumMinorCivsMet",native_gamecore_slot_method_owned<false,457>}, // SDK line593
    {"DoMinorLiberationByMajor",native_gamecore_slot_method_owned<false,458>}, // SDK line594
    {"IsProtectedByMajor",native_gamecore_slot_method_owned<false,459>}, // SDK line595
    {"CanMajorProtect",native_gamecore_slot_method_owned<false,460>}, // SDK line596
    {"CanMajorStartProtection",native_gamecore_slot_method_owned<false,461>}, // SDK line597
    {"CanMajorWithdrawProtection",native_gamecore_slot_method_owned<false,462>}, // SDK line598
    {"GetTurnLastPledgedProtectionByMajor",native_gamecore_slot_method_owned<false,463>}, // SDK line599
    {"GetTurnLastPledgeBrokenByMajor",native_gamecore_slot_method_owned<false,464>}, // SDK line600
    {"GetMinorCivBullyGoldAmount",native_gamecore_slot_method_owned<false,465>}, // SDK line601
    {"CanMajorBullyGold",native_gamecore_slot_method_owned<false,466>}, // SDK line602
    {"GetMajorBullyGoldDetails",native_gamecore_slot_method_owned<false,467>}, // SDK line603
    {"CanMajorBullyUnit",native_gamecore_slot_method_owned<false,468>}, // SDK line604
    {"GetMajorBullyUnitDetails",native_gamecore_slot_method_owned<false,469>}, // SDK line605
    {"CanMajorBuyout",native_gamecore_slot_method_owned<false,470>}, // SDK line606
    {"GetBuyoutCost",native_gamecore_slot_method_owned<false,471>}, // SDK line607
    {"CanMajorGiftTileImprovement",native_gamecore_slot_method_owned<false,472>}, // SDK line608
    {"CanMajorGiftTileImprovementAtPlot",native_gamecore_slot_method_owned<false,473>}, // SDK line609
    {"GetGiftTileImprovementCost",native_gamecore_slot_method_owned<false,474>}, // SDK line610
    {"AddMinorCivQuestIfAble",native_gamecore_slot_method_owned<false,475>}, // SDK line611
    {"GetFriendshipFromUnitGift",native_gamecore_slot_method_owned<false,476>}, // SDK line612
    {"IsAlive",native_gamecore_slot_method_owned<false,477>}, // SDK line614
    {"IsEverAlive",native_gamecore_slot_method_owned<false,478>}, // SDK line615
    {"IsExtendedGame",native_gamecore_slot_method_owned<false,479>}, // SDK line616
    {"IsFoundedFirstCity",native_gamecore_slot_method_owned<false,480>}, // SDK line617
    {"GetEndTurnBlockingType",native_gamecore_slot_method_owned<false,481>}, // SDK line619
    {"GetEndTurnBlockingNotificationIndex",native_gamecore_slot_method_owned<false,482>}, // SDK line620
    {"HasReceivedNetTurnComplete",native_gamecore_slot_method_owned<false,483>}, // SDK line621
    {"IsStrike",native_gamecore_slot_method_owned<false,484>}, // SDK line622
    {"GetID",native_gamecore_slot_method_owned<false,485>}, // SDK line624
    {"GetHandicapType",native_gamecore_slot_method_owned<false,486>}, // SDK line625
    {"GetCivilizationType",native_gamecore_slot_method_owned<false,487>}, // SDK line626
    {"GetLeaderType",native_gamecore_slot_method_owned<false,488>}, // SDK line627
    {"GetPersonalityType",native_gamecore_slot_method_owned<false,489>}, // SDK line628
    {"SetPersonalityType",native_gamecore_slot_method_owned<false,490>}, // SDK line629
    {"GetCurrentEra",native_gamecore_slot_method_owned<false,491>}, // SDK line630
    {"GetTeam",native_gamecore_slot_method_owned<false,492>}, // SDK line632
    {"GetPlayerColor",native_gamecore_slot_method_owned<false,493>}, // SDK line634
    {"GetPlayerColors",native_gamecore_slot_method_owned<false,494>}, // SDK line635
    {"GetSeaPlotYield",native_gamecore_slot_method_owned<false,495>}, // SDK line637
    {"GetYieldRateModifier",native_gamecore_slot_method_owned<false,496>}, // SDK line638
    {"GetCapitalYieldRateModifier",native_gamecore_slot_method_owned<false,497>}, // SDK line639
    {"GetExtraYieldThreshold",native_gamecore_slot_method_owned<false,498>}, // SDK line640
    {"GetScience",native_gamecore_slot_method_owned<false,499>}, // SDK line644
    {"GetScienceTimes100",native_gamecore_slot_method_owned<false,500>}, // SDK line645
    {"GetScienceFromCitiesTimes100",native_gamecore_slot_method_owned<false,501>}, // SDK line647
    {"GetScienceFromOtherPlayersTimes100",native_gamecore_slot_method_owned<false,502>}, // SDK line648
    {"GetScienceFromHappinessTimes100",native_gamecore_slot_method_owned<false,503>}, // SDK line649
    {"GetScienceFromResearchAgreementsTimes100",native_gamecore_slot_method_owned<false,504>}, // SDK line650
    {"GetScienceFromBudgetDeficitTimes100",native_gamecore_slot_method_owned<false,505>}, // SDK line651
    {"GetProximityToPlayer",native_gamecore_slot_method_owned<false,506>}, // SDK line655
    {"DoUpdateProximityToPlayer",native_gamecore_slot_method_owned<false,507>}, // SDK line656
    {"GetIncomingUnitType",native_gamecore_slot_method_owned<false,508>}, // SDK line658
    {"GetIncomingUnitCountdown",native_gamecore_slot_method_owned<false,509>}, // SDK line659
    {"IsOption",native_gamecore_slot_method_owned<false,510>}, // SDK line661
    {"SetOption",native_gamecore_slot_method_owned<false,511>}, // SDK line662
    {"IsPlayable",native_gamecore_slot_method_owned<false,512>}, // SDK line663
    {"SetPlayable",native_gamecore_slot_method_owned<false,513>}, // SDK line664
    {"GetNumResourceUsed",native_gamecore_slot_method_owned<false,514>}, // SDK line666
    {"GetNumResourceTotal",native_gamecore_slot_method_owned<false,515>}, // SDK line667
    {"ChangeNumResourceTotal",native_gamecore_slot_method_owned<false,516>}, // SDK line668
    {"GetNumResourceAvailable",native_gamecore_slot_method_owned<false,517>}, // SDK line669
    {"GetResourceExport",native_gamecore_slot_method_owned<false,518>}, // SDK line671
    {"GetResourceImport",native_gamecore_slot_method_owned<false,519>}, // SDK line672
    {"GetResourceFromMinors",native_gamecore_slot_method_owned<false,520>}, // SDK line673
    {"GetImprovementCount",native_gamecore_slot_method_owned<false,521>}, // SDK line675
    {"IsBuildingFree",native_gamecore_slot_method_owned<false,522>}, // SDK line677
    {"GetUnitClassCount",native_gamecore_slot_method_owned<false,523>}, // SDK line678
    {"IsUnitClassMaxedOut",native_gamecore_slot_method_owned<false,524>}, // SDK line679
    {"GetUnitClassMaking",native_gamecore_slot_method_owned<false,525>}, // SDK line680
    {"GetUnitClassCountPlusMaking",native_gamecore_slot_method_owned<false,526>}, // SDK line681
    {"GetBuildingClassCount",native_gamecore_slot_method_owned<false,527>}, // SDK line683
    {"IsBuildingClassMaxedOut",native_gamecore_slot_method_owned<false,528>}, // SDK line684
    {"GetBuildingClassMaking",native_gamecore_slot_method_owned<false,529>}, // SDK line685
    {"GetBuildingClassCountPlusMaking",native_gamecore_slot_method_owned<false,530>}, // SDK line686
    {"GetHurryCount",native_gamecore_slot_method_owned<false,531>}, // SDK line687
    {"IsHasAccessToHurry",native_gamecore_slot_method_owned<false,532>}, // SDK line688
    {"IsCanHurry",native_gamecore_slot_method_owned<false,533>}, // SDK line689
    {"GetHurryGoldCost",native_gamecore_slot_method_owned<false,534>}, // SDK line690
    {"IsResearchingTech",native_gamecore_slot_method_owned<false,535>}, // SDK line693
    {"SetResearchingTech",native_gamecore_slot_method_owned<false,536>}, // SDK line694
    {"GetCombatExperience",native_gamecore_slot_method_owned<false,537>}, // SDK line696
    {"ChangeCombatExperience",native_gamecore_slot_method_owned<false,538>}, // SDK line697
    {"SetCombatExperience",native_gamecore_slot_method_owned<false,539>}, // SDK line698
    {"GetLifetimeCombatExperience",native_gamecore_slot_method_owned<false,540>}, // SDK line699
    {"GetNavalCombatExperience",native_gamecore_slot_method_owned<false,541>}, // SDK line700
    {"ChangeNavalCombatExperience",native_gamecore_slot_method_owned<false,542>}, // SDK line701
    {"SetNavalCombatExperience",native_gamecore_slot_method_owned<false,543>}, // SDK line702
    {"GetSpecialistExtraYield",native_gamecore_slot_method_owned<false,544>}, // SDK line704
    {"FindPathLength",native_gamecore_slot_method_owned<false,545>}, // SDK line706
    {"GetQueuePosition",native_gamecore_slot_method_owned<false,546>}, // SDK line708
    {"ClearResearchQueue",native_gamecore_slot_method_owned<false,547>}, // SDK line709
    {"PushResearch",native_gamecore_slot_method_owned<false,548>}, // SDK line710
    {"PopResearch",native_gamecore_slot_method_owned<false,549>}, // SDK line711
    {"GetLengthResearchQueue",native_gamecore_slot_method_owned<false,550>}, // SDK line712
    {"AddCityName",native_gamecore_slot_method_owned<false,551>}, // SDK line713
    {"GetNumCityNames",native_gamecore_slot_method_owned<false,552>}, // SDK line714
    {"GetCityName",native_gamecore_slot_method_owned<false,553>}, // SDK line715
    {"Cities",native_gamecore_slot_method_owned<false,554>}, // SDK line717
    {"GetNumCities",native_gamecore_slot_method_owned<false,555>}, // SDK line718
    {"GetCityByID",native_gamecore_slot_method_owned<false,556>}, // SDK line719
    {"Units",native_gamecore_slot_method_owned<false,557>}, // SDK line721
    {"GetNumUnits",native_gamecore_slot_method_owned<false,558>}, // SDK line722
    {"GetUnitByID",native_gamecore_slot_method_owned<false,559>}, // SDK line723
    {"AI_updateFoundValues",native_gamecore_slot_method_owned<false,560>}, // SDK line725
    {"AI_foundValue",native_gamecore_slot_method_owned<false,561>}, // SDK line726
    {"GetScoreHistory",native_gamecore_slot_method_owned<false,562>}, // SDK line728
    {"GetEconomyHistory",native_gamecore_slot_method_owned<false,563>}, // SDK line729
    {"GetIndustryHistory",native_gamecore_slot_method_owned<false,564>}, // SDK line730
    {"GetAgricultureHistory",native_gamecore_slot_method_owned<false,565>}, // SDK line731
    {"GetPowerHistory",native_gamecore_slot_method_owned<false,566>}, // SDK line732
    {"GetReplayData",native_gamecore_slot_method_owned<false,567>}, // SDK line734
    {"SetReplayDataValue",native_gamecore_slot_method_owned<false,568>}, // SDK line735
    {"GetScriptData",native_gamecore_slot_method_owned<false,569>}, // SDK line737
    {"SetScriptData",native_gamecore_slot_method_owned<false,570>}, // SDK line738
    {"GetNumPlots",native_gamecore_slot_method_owned<false,571>}, // SDK line740
    {"GetNumPlotsBought",native_gamecore_slot_method_owned<false,572>}, // SDK line742
    {"SetNumPlotsBought",native_gamecore_slot_method_owned<false,573>}, // SDK line743
    {"ChangeNumPlotsBought",native_gamecore_slot_method_owned<false,574>}, // SDK line744
    {"GetBuyPlotCost",native_gamecore_slot_method_owned<false,575>}, // SDK line746
    {"GetPlotDanger",native_gamecore_slot_method_owned<false,576>}, // SDK line747
    {"DoBeginDiploWithHuman",native_gamecore_slot_method_owned<false,577>}, // SDK line749
    {"DoTradeScreenOpened",native_gamecore_slot_method_owned<false,578>}, // SDK line750
    {"DoTradeScreenClosed",native_gamecore_slot_method_owned<false,579>}, // SDK line751
    {"GetMajorCivApproach",native_gamecore_slot_method_owned<false,580>}, // SDK line752
    {"GetApproachTowardsUsGuess",native_gamecore_slot_method_owned<false,581>}, // SDK line753
    {"IsWillAcceptPeaceWithPlayer",native_gamecore_slot_method_owned<false,582>}, // SDK line754
    {"IsProtectingMinor",native_gamecore_slot_method_owned<false,583>}, // SDK line755
    {"IsDontSettleMessageTooSoon",native_gamecore_slot_method_owned<false,584>}, // SDK line756
    {"IsStopSpyingMessageTooSoon",native_gamecore_slot_method_owned<false,585>}, // SDK line757
    {"IsAskedToStopConverting",native_gamecore_slot_method_owned<false,586>}, // SDK line758
    {"IsAskedToStopDigging",native_gamecore_slot_method_owned<false,587>}, // SDK line759
    {"IsDoFMessageTooSoon",native_gamecore_slot_method_owned<false,588>}, // SDK line760
    {"IsDoF",native_gamecore_slot_method_owned<false,589>}, // SDK line761
    {"GetDoFCounter",native_gamecore_slot_method_owned<false,590>}, // SDK line762
    {"IsPlayerDoFwithAnyFriend",native_gamecore_slot_method_owned<false,591>}, // SDK line763
    {"IsPlayerDoFwithAnyEnemy",native_gamecore_slot_method_owned<false,592>}, // SDK line764
    {"IsPlayerDenouncedFriend",native_gamecore_slot_method_owned<false,593>}, // SDK line765
    {"IsPlayerDenouncedEnemy",native_gamecore_slot_method_owned<false,594>}, // SDK line766
    {"IsUntrustworthyFriend",native_gamecore_slot_method_owned<false,595>}, // SDK line767
    {"GetNumFriendsDenouncedBy",native_gamecore_slot_method_owned<false,596>}, // SDK line768
    {"IsFriendDenouncedUs",native_gamecore_slot_method_owned<false,597>}, // SDK line769
    {"GetWeDenouncedFriendCount",native_gamecore_slot_method_owned<false,598>}, // SDK line770
    {"IsFriendDeclaredWarOnUs",native_gamecore_slot_method_owned<false,599>}, // SDK line771
    {"GetWeDeclaredWarOnFriendCount",native_gamecore_slot_method_owned<false,600>}, // SDK line772
    {"GetCoopWarAcceptedState",native_gamecore_slot_method_owned<false,601>}, // SDK line774
    {"GetNumWarsFought",native_gamecore_slot_method_owned<false,602>}, // SDK line775
    {"GetLandDisputeLevel",native_gamecore_slot_method_owned<false,603>}, // SDK line777
    {"GetVictoryDisputeLevel",native_gamecore_slot_method_owned<false,604>}, // SDK line778
    {"GetWonderDisputeLevel",native_gamecore_slot_method_owned<false,605>}, // SDK line779
    {"GetMinorCivDisputeLevel",native_gamecore_slot_method_owned<false,606>}, // SDK line780
    {"GetWarmongerThreat",native_gamecore_slot_method_owned<false,607>}, // SDK line781
    {"IsPlayerNoSettleRequestEverAsked",native_gamecore_slot_method_owned<false,608>}, // SDK line782
    {"IsPlayerStopSpyingRequestEverAsked",native_gamecore_slot_method_owned<false,609>}, // SDK line783
    {"IsDemandEverMade",native_gamecore_slot_method_owned<false,610>}, // SDK line784
    {"GetNumCiviliansReturnedToMe",native_gamecore_slot_method_owned<false,611>}, // SDK line785
    {"GetNumLandmarksBuiltForMe",native_gamecore_slot_method_owned<false,612>}, // SDK line786
    {"GetNumTimesCultureBombed",native_gamecore_slot_method_owned<false,613>}, // SDK line787
    {"GetNegativeReligiousConversionPoints",native_gamecore_slot_method_owned<false,614>}, // SDK line788
    {"GetNegativeArchaeologyPoints",native_gamecore_slot_method_owned<false,615>}, // SDK line789
    {"HasOthersReligionInMostCities",native_gamecore_slot_method_owned<false,616>}, // SDK line790
    {"IsPlayerBrokenMilitaryPromise",native_gamecore_slot_method_owned<false,617>}, // SDK line791
    {"IsPlayerIgnoredMilitaryPromise",native_gamecore_slot_method_owned<false,618>}, // SDK line792
    {"IsPlayerBrokenExpansionPromise",native_gamecore_slot_method_owned<false,619>}, // SDK line793
    {"IsPlayerIgnoredExpansionPromise",native_gamecore_slot_method_owned<false,620>}, // SDK line794
    {"IsPlayerBrokenBorderPromise",native_gamecore_slot_method_owned<false,621>}, // SDK line795
    {"IsPlayerIgnoredBorderPromise",native_gamecore_slot_method_owned<false,622>}, // SDK line796
    {"IsPlayerBrokenAttackCityStatePromise",native_gamecore_slot_method_owned<false,623>}, // SDK line797
    {"IsPlayerIgnoredAttackCityStatePromise",native_gamecore_slot_method_owned<false,624>}, // SDK line798
    {"IsPlayerBrokenBullyCityStatePromise",native_gamecore_slot_method_owned<false,625>}, // SDK line799
    {"IsPlayerIgnoredBullyCityStatePromise",native_gamecore_slot_method_owned<false,626>}, // SDK line800
    {"IsPlayerBrokenSpyPromise",native_gamecore_slot_method_owned<false,627>}, // SDK line801
    {"IsPlayerIgnoredSpyPromise",native_gamecore_slot_method_owned<false,628>}, // SDK line802
    {"IsPlayerForgivenForSpying",native_gamecore_slot_method_owned<false,629>}, // SDK line803
    {"IsPlayerBrokenNoConvertPromise",native_gamecore_slot_method_owned<false,630>}, // SDK line804
    {"IsPlayerIgnoredNoConvertPromise",native_gamecore_slot_method_owned<false,631>}, // SDK line805
    {"IsPlayerBrokenNoDiggingPromise",native_gamecore_slot_method_owned<false,632>}, // SDK line806
    {"IsPlayerIgnoredNoDiggingPromise",native_gamecore_slot_method_owned<false,633>}, // SDK line807
    {"IsPlayerBrokenCoopWarPromise",native_gamecore_slot_method_owned<false,634>}, // SDK line808
    {"GetOtherPlayerNumProtectedMinorsKilled",native_gamecore_slot_method_owned<false,635>}, // SDK line809
    {"GetOtherPlayerNumProtectedMinorsAttacked",native_gamecore_slot_method_owned<false,636>}, // SDK line810
    {"GetTurnsSincePlayerBulliedProtectedMinor",native_gamecore_slot_method_owned<false,637>}, // SDK line811
    {"IsHasPlayerBulliedProtectedMinor",native_gamecore_slot_method_owned<false,638>}, // SDK line812
    {"IsDenouncedPlayer",native_gamecore_slot_method_owned<false,639>}, // SDK line813
    {"GetDenouncedPlayerCounter",native_gamecore_slot_method_owned<false,640>}, // SDK line814
    {"IsDenouncingPlayer",native_gamecore_slot_method_owned<false,641>}, // SDK line815
    {"IsPlayerRecklessExpander",native_gamecore_slot_method_owned<false,642>}, // SDK line816
    {"GetRecentTradeValue",native_gamecore_slot_method_owned<false,643>}, // SDK line817
    {"GetCommonFoeValue",native_gamecore_slot_method_owned<false,644>}, // SDK line818
    {"GetRecentAssistValue",native_gamecore_slot_method_owned<false,645>}, // SDK line819
    {"IsGaveAssistanceTo",native_gamecore_slot_method_owned<false,646>}, // SDK line820
    {"IsHasPaidTributeTo",native_gamecore_slot_method_owned<false,647>}, // SDK line821
    {"IsNukedBy",native_gamecore_slot_method_owned<false,648>}, // SDK line822
    {"IsCapitalCapturedBy",native_gamecore_slot_method_owned<false,649>}, // SDK line823
    {"IsAngryAboutProtectedMinorKilled",native_gamecore_slot_method_owned<false,650>}, // SDK line824
    {"IsAngryAboutProtectedMinorAttacked",native_gamecore_slot_method_owned<false,651>}, // SDK line825
    {"IsAngryAboutProtectedMinorBullied",native_gamecore_slot_method_owned<false,652>}, // SDK line826
    {"IsAngryAboutSidedWithTheirProtectedMinor",native_gamecore_slot_method_owned<false,653>}, // SDK line827
    {"GetNumTimesRobbedBy",native_gamecore_slot_method_owned<false,654>}, // SDK line828
    {"GetNumTimesIntrigueSharedBy",native_gamecore_slot_method_owned<false,655>}, // SDK line829
    {"DoForceDoF",native_gamecore_slot_method_owned<false,656>}, // SDK line831
    {"DoForceDenounce",native_gamecore_slot_method_owned<false,657>}, // SDK line832
    {"GetNumNotifications",native_gamecore_slot_method_owned<false,658>}, // SDK line834
    {"GetNotificationStr",native_gamecore_slot_method_owned<false,659>}, // SDK line835
    {"GetNotificationSummaryStr",native_gamecore_slot_method_owned<false,660>}, // SDK line836
    {"GetNotificationIndex",native_gamecore_slot_method_owned<false,661>}, // SDK line837
    {"GetNotificationTurn",native_gamecore_slot_method_owned<false,662>}, // SDK line838
    {"GetNotificationDismissed",native_gamecore_slot_method_owned<false,663>}, // SDK line839
    {"AddNotification",native_gamecore_slot_method_owned<false,664>}, // SDK line840
    {"GetRecommendedWorkerPlots",native_gamecore_slot_method_owned<false,665>}, // SDK line842
    {"GetRecommendedFoundCityPlots",native_gamecore_slot_method_owned<false,666>}, // SDK line843
    {"GetUnimprovedAvailableLuxuryResource",native_gamecore_slot_method_owned<false,667>}, // SDK line844
    {"IsAnyPlotImproved",native_gamecore_slot_method_owned<false,668>}, // SDK line845
    {"GetPlayerVisiblePlot",native_gamecore_slot_method_owned<false,669>}, // SDK line846
    {"GetEverPoppedGoody",native_gamecore_slot_method_owned<false,670>}, // SDK line848
    {"GetClosestGoodyPlot",native_gamecore_slot_method_owned<false,671>}, // SDK line849
    {"IsAnyGoodyPlotAccessible",native_gamecore_slot_method_owned<false,672>}, // SDK line850
    {"GetPlotHasOrder",native_gamecore_slot_method_owned<false,673>}, // SDK line851
    {"GetAnyUnitHasOrderToGoody",native_gamecore_slot_method_owned<false,674>}, // SDK line852
    {"GetEverTrainedBuilder",native_gamecore_slot_method_owned<false,675>}, // SDK line853
    {"GetNumFreeTechs",native_gamecore_slot_method_owned<false,676>}, // SDK line855
    {"SetNumFreeTechs",native_gamecore_slot_method_owned<false,677>}, // SDK line856
    {"GetNumFreePolicies",native_gamecore_slot_method_owned<false,678>}, // SDK line857
    {"SetNumFreePolicies",native_gamecore_slot_method_owned<false,679>}, // SDK line858
    {"ChangeNumFreePolicies",native_gamecore_slot_method_owned<false,680>}, // SDK line859
    {"GetNumFreeTenets",native_gamecore_slot_method_owned<false,681>}, // SDK line860
    {"SetNumFreeTenets",native_gamecore_slot_method_owned<false,682>}, // SDK line861
    {"ChangeNumFreeTenets",native_gamecore_slot_method_owned<false,683>}, // SDK line862
    {"GetNumFreeGreatPeople",native_gamecore_slot_method_owned<false,684>}, // SDK line863
    {"SetNumFreeGreatPeople",native_gamecore_slot_method_owned<false,685>}, // SDK line864
    {"ChangeNumFreeGreatPeople",native_gamecore_slot_method_owned<false,686>}, // SDK line865
    {"GetNumMayaBoosts",native_gamecore_slot_method_owned<false,687>}, // SDK line866
    {"SetNumMayaBoosts",native_gamecore_slot_method_owned<false,688>}, // SDK line867
    {"ChangeNumMayaBoosts",native_gamecore_slot_method_owned<false,689>}, // SDK line868
    {"GetNumFaithGreatPeople",native_gamecore_slot_method_owned<false,690>}, // SDK line869
    {"SetNumFaithGreatPeople",native_gamecore_slot_method_owned<false,691>}, // SDK line870
    {"ChangeNumFaithGreatPeople",native_gamecore_slot_method_owned<false,692>}, // SDK line871
    {"GetUnitBaktun",native_gamecore_slot_method_owned<false,693>}, // SDK line872
    {"IsFreeMayaGreatPersonChoice",native_gamecore_slot_method_owned<false,694>}, // SDK line873
    {"GetTraitGoldenAgeCombatModifier",native_gamecore_slot_method_owned<false,695>}, // SDK line875
    {"GetTraitCityStateCombatModifier",native_gamecore_slot_method_owned<false,696>}, // SDK line876
    {"GetTraitGreatGeneralExtraBonus",native_gamecore_slot_method_owned<false,697>}, // SDK line877
    {"GetTraitGreatScientistRateModifier",native_gamecore_slot_method_owned<false,698>}, // SDK line878
    {"IsTraitBonusReligiousBelief",native_gamecore_slot_method_owned<false,699>}, // SDK line879
    {"GetHappinessFromLuxury",native_gamecore_slot_method_owned<false,700>}, // SDK line880
    {"IsAbleToAnnexCityStates",native_gamecore_slot_method_owned<false,701>}, // SDK line881
    {"IsUsingMayaCalendar",native_gamecore_slot_method_owned<false,702>}, // SDK line882
    {"GetMayaCalendarString",native_gamecore_slot_method_owned<false,703>}, // SDK line883
    {"GetMayaCalendarLongString",native_gamecore_slot_method_owned<false,704>}, // SDK line884
    {"GetExtraBuildingHappinessFromPolicies",native_gamecore_slot_method_owned<false,705>}, // SDK line886
    {"GetNextCity",native_gamecore_slot_method_owned<false,706>}, // SDK line888
    {"GetPrevCity",native_gamecore_slot_method_owned<false,707>}, // SDK line889
    {"GetFreePromotionCount",native_gamecore_slot_method_owned<false,708>}, // SDK line891
    {"IsFreePromotion",native_gamecore_slot_method_owned<false,709>}, // SDK line892
    {"ChangeFreePromotionCount",native_gamecore_slot_method_owned<false,710>}, // SDK line893
    {"GetEmbarkedGraphicOverride",native_gamecore_slot_method_owned<false,711>}, // SDK line895
    {"SetEmbarkedGraphicOverride",native_gamecore_slot_method_owned<false,712>}, // SDK line896
    {"AddTemporaryDominanceZone",native_gamecore_slot_method_owned<false,713>}, // SDK line898
    {"GetNaturalWonderYieldModifier",native_gamecore_slot_method_owned<false,714>}, // SDK line900
    {"GetPolicyBuildingClassYieldModifier",native_gamecore_slot_method_owned<false,715>}, // SDK line902
    {"GetPolicyBuildingClassYieldChange",native_gamecore_slot_method_owned<false,716>}, // SDK line903
    {"GetPolicyEspionageModifier",native_gamecore_slot_method_owned<false,717>}, // SDK line904
    {"GetPolicyEspionageCatchSpiesModifier",native_gamecore_slot_method_owned<false,718>}, // SDK line905
    {"GetPlayerBuildingClassYieldChange",native_gamecore_slot_method_owned<false,719>}, // SDK line907
    {"GetPlayerBuildingClassHappiness",native_gamecore_slot_method_owned<false,720>}, // SDK line908
    {"WasResurrectedBy",native_gamecore_slot_method_owned<false,721>}, // SDK line910
    {"WasResurrectedThisTurnBy",native_gamecore_slot_method_owned<false,722>}, // SDK line911
    {"GetOpinionTable",native_gamecore_slot_method_owned<false,723>}, // SDK line913
    {"GetDealValue",native_gamecore_slot_method_owned<false,724>}, // SDK line914
    {"GetDealMyValue",native_gamecore_slot_method_owned<false,725>}, // SDK line915
    {"GetDealTheyreValue",native_gamecore_slot_method_owned<false,726>}, // SDK line916
    {"MayNotAnnex",native_gamecore_slot_method_owned<false,727>}, // SDK line917
    {"GetEspionageCityStatus",native_gamecore_slot_method_owned<false,728>}, // SDK line919
    {"GetNumSpies",native_gamecore_slot_method_owned<false,729>}, // SDK line920
    {"GetNumUnassignedSpies",native_gamecore_slot_method_owned<false,730>}, // SDK line921
    {"GetEspionageSpies",native_gamecore_slot_method_owned<false,731>}, // SDK line922
    {"HasSpyEstablishedSurveillance",native_gamecore_slot_method_owned<false,732>}, // SDK line923
    {"IsSpyDiplomat",native_gamecore_slot_method_owned<false,733>}, // SDK line924
    {"IsSpySchmoozing",native_gamecore_slot_method_owned<false,734>}, // SDK line925
    {"CanSpyStageCoup",native_gamecore_slot_method_owned<false,735>}, // SDK line926
    {"GetAvailableSpyRelocationCities",native_gamecore_slot_method_owned<false,736>}, // SDK line927
    {"GetNumTechsToSteal",native_gamecore_slot_method_owned<false,737>}, // SDK line928
    {"GetIntrigueMessages",native_gamecore_slot_method_owned<false,738>}, // SDK line929
    {"HasRecentIntrigueAbout",native_gamecore_slot_method_owned<false,739>}, // SDK line930
    {"GetRecentIntrigueInfo",native_gamecore_slot_method_owned<false,740>}, // SDK line931
    {"GetCoupChanceOfSuccess",native_gamecore_slot_method_owned<false,741>}, // SDK line932
    {"IsMyDiplomatVisitingThem",native_gamecore_slot_method_owned<false,742>}, // SDK line933
    {"IsOtherDiplomatVisitingMe",native_gamecore_slot_method_owned<false,743>}, // SDK line934
    {"GetTradeRouteRange",native_gamecore_slot_method_owned<false,744>}, // SDK line936
    {"GetInternationalTradeRoutePlotToolTip",native_gamecore_slot_method_owned<false,745>}, // SDK line937
    {"GetInternationalTradeRoutePlotMouseoverToolTip",native_gamecore_slot_method_owned<false,746>}, // SDK line938
    {"GetNumInternationalTradeRoutesUsed",native_gamecore_slot_method_owned<false,747>}, // SDK line939
    {"GetNumInternationalTradeRoutesAvailable",native_gamecore_slot_method_owned<false,748>}, // SDK line940
    {"GetPotentialInternationalTradeRouteDestinations",native_gamecore_slot_method_owned<false,749>}, // SDK line941
    {"GetInternationalTradeRouteBaseBonus",native_gamecore_slot_method_owned<false,750>}, // SDK line942
    {"GetInternationalTradeRouteGPTBonus",native_gamecore_slot_method_owned<false,751>}, // SDK line943
    {"GetInternationalTradeRouteResourceBonus",native_gamecore_slot_method_owned<false,752>}, // SDK line944
    {"GetInternationalTradeRouteResourceTraitModifier",native_gamecore_slot_method_owned<false,753>}, // SDK line945
    {"GetInternationalTradeRouteExclusiveBonus",native_gamecore_slot_method_owned<false,754>}, // SDK line946
    {"GetInternationalTradeRouteYourBuildingBonus",native_gamecore_slot_method_owned<false,755>}, // SDK line947
    {"GetInternationalTradeRouteTheirBuildingBonus",native_gamecore_slot_method_owned<false,756>}, // SDK line948
    {"GetInternationalTradeRoutePolicyBonus",native_gamecore_slot_method_owned<false,757>}, // SDK line949
    {"GetInternationalTradeRouteOtherTraitBonus",native_gamecore_slot_method_owned<false,758>}, // SDK line950
    {"GetInternationalTradeRouteRiverModifier",native_gamecore_slot_method_owned<false,759>}, // SDK line951
    {"GetInternationalTradeRouteDomainModifier",native_gamecore_slot_method_owned<false,760>}, // SDK line952
    {"GetInternationalTradeRouteTotal",native_gamecore_slot_method_owned<false,761>}, // SDK line953
    {"GetInternationalTradeRouteScience",native_gamecore_slot_method_owned<false,762>}, // SDK line954
    {"GetPotentialTradeUnitNewHomeCity",native_gamecore_slot_method_owned<false,763>}, // SDK line955
    {"GetPotentialAdmiralNewPort",native_gamecore_slot_method_owned<false,764>}, // SDK line956
    {"GetNumAvailableTradeUnits",native_gamecore_slot_method_owned<false,765>}, // SDK line957
    {"GetTradeUnitType",native_gamecore_slot_method_owned<false,766>}, // SDK line958
    {"GetTradeYourRoutesTTString",native_gamecore_slot_method_owned<false,767>}, // SDK line959
    {"GetTradeToYouRoutesTTString",native_gamecore_slot_method_owned<false,768>}, // SDK line960
    {"GetTradeRoutes",native_gamecore_slot_method_owned<false,769>}, // SDK line961
    {"GetTradeRoutesAvailable",native_gamecore_slot_method_owned<false,770>}, // SDK line962
    {"GetTradeRoutesToYou",native_gamecore_slot_method_owned<false,771>}, // SDK line963
    {"GetNumTechDifference",native_gamecore_slot_method_owned<false,772>}, // SDK line964
    {"GetGreatWorks",native_gamecore_slot_method_owned<false,773>}, // SDK line967
    {"GetSwappableGreatWriting",native_gamecore_slot_method_owned<false,774>}, // SDK line968
    {"GetSwappableGreatArt",native_gamecore_slot_method_owned<false,775>}, // SDK line969
    {"GetSwappableGreatArtifact",native_gamecore_slot_method_owned<false,776>}, // SDK line970
    {"GetSwappableGreatMusic",native_gamecore_slot_method_owned<false,777>}, // SDK line971
    {"GetOthersGreatWorks",native_gamecore_slot_method_owned<false,778>}, // SDK line972
    {"CanCommitVote",native_gamecore_slot_method_owned<false,779>}, // SDK line974
    {"GetCommitVoteDetails",native_gamecore_slot_method_owned<false,780>}, // SDK line975
    {"IsConnected",native_gamecore_slot_method_owned<false,781>}, // SDK line977
    {"IsObserver",native_gamecore_slot_method_owned<false,782>}, // SDK line978
    {"HasTurnTimerExpired",native_gamecore_slot_method_owned<false,783>}, // SDK line980
    {"HasUnitOfClassType",native_gamecore_slot_method_owned<false,784>}, // SDK line982
    {"GetWarmongerPreviewString",native_gamecore_slot_method_owned<false,785>}, // SDK line984
    {"GetLiberationPreviewString",native_gamecore_slot_method_owned<false,786>}, // SDK line985
};
static constexpr NativeGameCoreSlotMethod native_gamecore_team_methods[]={
    {"IsNone",native_gamecore_slot_method_owned<true,0>}, // SDK line38
    {"AddTeam",native_gamecore_slot_method_owned<true,1>}, // SDK line40
    {"CanChangeWarPeace",native_gamecore_slot_method_owned<true,2>}, // SDK line42
    {"CanDeclareWar",native_gamecore_slot_method_owned<true,3>}, // SDK line43
    {"DeclareWar",native_gamecore_slot_method_owned<true,4>}, // SDK line44
    {"MakePeace",native_gamecore_slot_method_owned<true,5>}, // SDK line45
    {"GetNumTurnsLockedIntoWar",native_gamecore_slot_method_owned<true,6>}, // SDK line46
    {"Meet",native_gamecore_slot_method_owned<true,7>}, // SDK line47
    {"GetScore",native_gamecore_slot_method_owned<true,8>}, // SDK line49
    {"GetPower",native_gamecore_slot_method_owned<true,9>}, // SDK line50
    {"GetDefensivePower",native_gamecore_slot_method_owned<true,10>}, // SDK line51
    {"GetNumNukeUnits",native_gamecore_slot_method_owned<true,11>}, // SDK line52
    {"GetTeamVotingForInDiplo",native_gamecore_slot_method_owned<true,12>}, // SDK line54
    {"GetProjectedVotesFromMinorAllies",native_gamecore_slot_method_owned<true,13>}, // SDK line55
    {"GetProjectedVotesFromLiberatedMinors",native_gamecore_slot_method_owned<true,14>}, // SDK line56
    {"GetProjectedVotesFromCivs",native_gamecore_slot_method_owned<true,15>}, // SDK line57
    {"GetTotalProjectedVotes",native_gamecore_slot_method_owned<true,16>}, // SDK line58
    {"GetTotalSecuredVotes",native_gamecore_slot_method_owned<true,17>}, // SDK line59
    {"GetAtWarCount",native_gamecore_slot_method_owned<true,18>}, // SDK line61
    {"GetHasMetCivCount",native_gamecore_slot_method_owned<true,19>}, // SDK line62
    {"HasMetHuman",native_gamecore_slot_method_owned<true,20>}, // SDK line63
    {"GetDefensivePactCount",native_gamecore_slot_method_owned<true,21>}, // SDK line64
    {"GetUnitClassMaking",native_gamecore_slot_method_owned<true,22>}, // SDK line66
    {"GetUnitClassCountPlusMaking",native_gamecore_slot_method_owned<true,23>}, // SDK line67
    {"GetBuildingClassMaking",native_gamecore_slot_method_owned<true,24>}, // SDK line68
    {"GetBuildingClassCountPlusMaking",native_gamecore_slot_method_owned<true,25>}, // SDK line69
    {"CountNumUnitsByArea",native_gamecore_slot_method_owned<true,26>}, // SDK line71
    {"CountNumCitiesByArea",native_gamecore_slot_method_owned<true,27>}, // SDK line72
    {"CountTotalPopulationByArea",native_gamecore_slot_method_owned<true,28>}, // SDK line73
    {"CountEnemyDangerByArea",native_gamecore_slot_method_owned<true,29>}, // SDK line74
    {"IsHuman",native_gamecore_slot_method_owned<true,30>}, // SDK line76
    {"IsBarbarian",native_gamecore_slot_method_owned<true,31>}, // SDK line77
    {"IsMinorCiv",native_gamecore_slot_method_owned<true,32>}, // SDK line79
    {"IsMinorCivWarmonger",native_gamecore_slot_method_owned<true,33>}, // SDK line80
    {"GetLeaderID",native_gamecore_slot_method_owned<true,34>}, // SDK line82
    {"GetSecretaryID",native_gamecore_slot_method_owned<true,35>}, // SDK line83
    {"GetHandicapType",native_gamecore_slot_method_owned<true,36>}, // SDK line84
    {"GetName",native_gamecore_slot_method_owned<true,37>}, // SDK line85
    {"GetNameKey",native_gamecore_slot_method_owned<true,38>}, // SDK line86
    {"GetNumMembers",native_gamecore_slot_method_owned<true,39>}, // SDK line88
    {"IsAlive",native_gamecore_slot_method_owned<true,40>}, // SDK line89
    {"IsEverAlive",native_gamecore_slot_method_owned<true,41>}, // SDK line90
    {"GetNumCities",native_gamecore_slot_method_owned<true,42>}, // SDK line91
    {"GetTotalPopulation",native_gamecore_slot_method_owned<true,43>}, // SDK line92
    {"GetTotalLand",native_gamecore_slot_method_owned<true,44>}, // SDK line93
    {"GetNukeInterception",native_gamecore_slot_method_owned<true,45>}, // SDK line94
    {"ChangeNukeInterception",native_gamecore_slot_method_owned<true,46>}, // SDK line95
    {"GetForceTeamVoteEligibilityCount",native_gamecore_slot_method_owned<true,47>}, // SDK line97
    {"IsForceTeamVoteEligible",native_gamecore_slot_method_owned<true,48>}, // SDK line98
    {"ChangeForceTeamVoteEligibilityCount",native_gamecore_slot_method_owned<true,49>}, // SDK line99
    {"GetExtraWaterSeeFromCount",native_gamecore_slot_method_owned<true,50>}, // SDK line100
    {"IsExtraWaterSeeFrom",native_gamecore_slot_method_owned<true,51>}, // SDK line101
    {"ChangeExtraWaterSeeFromCount",native_gamecore_slot_method_owned<true,52>}, // SDK line102
    {"GetMapTradingCount",native_gamecore_slot_method_owned<true,53>}, // SDK line103
    {"IsMapTrading",native_gamecore_slot_method_owned<true,54>}, // SDK line104
    {"ChangeMapTradingCount",native_gamecore_slot_method_owned<true,55>}, // SDK line105
    {"GetTechTradingCount",native_gamecore_slot_method_owned<true,56>}, // SDK line106
    {"IsTechTrading",native_gamecore_slot_method_owned<true,57>}, // SDK line107
    {"ChangeTechTradingCount",native_gamecore_slot_method_owned<true,58>}, // SDK line108
    {"GetGoldTradingCount",native_gamecore_slot_method_owned<true,59>}, // SDK line109
    {"IsGoldTrading",native_gamecore_slot_method_owned<true,60>}, // SDK line110
    {"ChangeGoldTradingCount",native_gamecore_slot_method_owned<true,61>}, // SDK line111
    {"GetAllowEmbassyTradingAllowedCount",native_gamecore_slot_method_owned<true,62>}, // SDK line112
    {"IsAllowEmbassyTradingAllowed",native_gamecore_slot_method_owned<true,63>}, // SDK line113
    {"ChangeAllowEmbassyTradingAllowedCount",native_gamecore_slot_method_owned<true,64>}, // SDK line114
    {"GetOpenBordersTradingAllowedCount",native_gamecore_slot_method_owned<true,65>}, // SDK line115
    {"IsOpenBordersTradingAllowed",native_gamecore_slot_method_owned<true,66>}, // SDK line116
    {"IsOpenBordersTradingAllowedWithTeam",native_gamecore_slot_method_owned<true,67>}, // SDK line117
    {"ChangeOpenBordersTradingAllowedCount",native_gamecore_slot_method_owned<true,68>}, // SDK line118
    {"GetDefensivePactTradingAllowedCount",native_gamecore_slot_method_owned<true,69>}, // SDK line119
    {"IsDefensivePactTradingAllowed",native_gamecore_slot_method_owned<true,70>}, // SDK line120
    {"IsDefensivePactTradingAllowedWithTeam",native_gamecore_slot_method_owned<true,71>}, // SDK line121
    {"ChangeDefensivePactTradingAllowedCount",native_gamecore_slot_method_owned<true,72>}, // SDK line122
    {"IsResearchAgreementTradingAllowed",native_gamecore_slot_method_owned<true,73>}, // SDK line123
    {"IsResearchAgreementTradingAllowedWithTeam",native_gamecore_slot_method_owned<true,74>}, // SDK line124
    {"IsTradeAgreementTradingAllowed",native_gamecore_slot_method_owned<true,75>}, // SDK line125
    {"GetPermanentAllianceTradingCount",native_gamecore_slot_method_owned<true,76>}, // SDK line126
    {"IsPermanentAllianceTrading",native_gamecore_slot_method_owned<true,77>}, // SDK line127
    {"ChangePermanentAllianceTradingCount",native_gamecore_slot_method_owned<true,78>}, // SDK line128
    {"GetBridgeBuildingCount",native_gamecore_slot_method_owned<true,79>}, // SDK line129
    {"IsBridgeBuilding",native_gamecore_slot_method_owned<true,80>}, // SDK line130
    {"ChangeBridgeBuildingCount",native_gamecore_slot_method_owned<true,81>}, // SDK line131
    {"GetWaterWorkCount",native_gamecore_slot_method_owned<true,82>}, // SDK line132
    {"IsWaterWork",native_gamecore_slot_method_owned<true,83>}, // SDK line133
    {"ChangeWaterWorkCount",native_gamecore_slot_method_owned<true,84>}, // SDK line134
    {"GetBorderObstacleCount",native_gamecore_slot_method_owned<true,85>}, // SDK line136
    {"IsBorderObstacle",native_gamecore_slot_method_owned<true,86>}, // SDK line137
    {"ChangeBorderObstacleCount",native_gamecore_slot_method_owned<true,87>}, // SDK line138
    {"IsMapCentering",native_gamecore_slot_method_owned<true,88>}, // SDK line140
    {"SetMapCentering",native_gamecore_slot_method_owned<true,89>}, // SDK line141
    {"GetID",native_gamecore_slot_method_owned<true,90>}, // SDK line143
    {"GetTechShareCount",native_gamecore_slot_method_owned<true,91>}, // SDK line145
    {"IsTechShare",native_gamecore_slot_method_owned<true,92>}, // SDK line146
    {"ChangeTechShareCount",native_gamecore_slot_method_owned<true,93>}, // SDK line147
    {"GetExtraMoves",native_gamecore_slot_method_owned<true,94>}, // SDK line149
    {"ChangeExtraMoves",native_gamecore_slot_method_owned<true,95>}, // SDK line150
    {"CanEmbark",native_gamecore_slot_method_owned<true,96>}, // SDK line152
    {"CanDefensiveEmbark",native_gamecore_slot_method_owned<true,97>}, // SDK line153
    {"IsHasMet",native_gamecore_slot_method_owned<true,98>}, // SDK line155
    {"IsAtWar",native_gamecore_slot_method_owned<true,99>}, // SDK line156
    {"IsPermanentWarPeace",native_gamecore_slot_method_owned<true,100>}, // SDK line157
    {"SetPermanentWarPeace",native_gamecore_slot_method_owned<true,101>}, // SDK line158
    {"GetLiberatedByTeam",native_gamecore_slot_method_owned<true,102>}, // SDK line160
    {"GetKilledByTeam",native_gamecore_slot_method_owned<true,103>}, // SDK line161
    {"HasEmbassyAtTeam",native_gamecore_slot_method_owned<true,104>}, // SDK line163
    {"IsAllowsOpenBordersToTeam",native_gamecore_slot_method_owned<true,105>}, // SDK line164
    {"IsForcePeace",native_gamecore_slot_method_owned<true,106>}, // SDK line165
    {"IsDefensivePact",native_gamecore_slot_method_owned<true,107>}, // SDK line166
    {"GetRouteChange",native_gamecore_slot_method_owned<true,108>}, // SDK line167
    {"ChangeRouteChange",native_gamecore_slot_method_owned<true,109>}, // SDK line168
    {"GetProjectCount",native_gamecore_slot_method_owned<true,110>}, // SDK line169
    {"GetProjectDefaultArtType",native_gamecore_slot_method_owned<true,111>}, // SDK line170
    {"SetProjectDefaultArtType",native_gamecore_slot_method_owned<true,112>}, // SDK line171
    {"GetProjectArtType",native_gamecore_slot_method_owned<true,113>}, // SDK line172
    {"SetProjectArtType",native_gamecore_slot_method_owned<true,114>}, // SDK line173
    {"IsProjectMaxedOut",native_gamecore_slot_method_owned<true,115>}, // SDK line174
    {"IsProjectAndArtMaxedOut",native_gamecore_slot_method_owned<true,116>}, // SDK line175
    {"ChangeProjectCount",native_gamecore_slot_method_owned<true,117>}, // SDK line176
    {"GetProjectMaking",native_gamecore_slot_method_owned<true,118>}, // SDK line177
    {"GetUnitClassCount",native_gamecore_slot_method_owned<true,119>}, // SDK line178
    {"IsUnitClassMaxedOut",native_gamecore_slot_method_owned<true,120>}, // SDK line179
    {"GetBuildingClassCount",native_gamecore_slot_method_owned<true,121>}, // SDK line180
    {"IsBuildingClassMaxedOut",native_gamecore_slot_method_owned<true,122>}, // SDK line181
    {"GetObsoleteBuildingCount",native_gamecore_slot_method_owned<true,123>}, // SDK line182
    {"IsObsoleteBuilding",native_gamecore_slot_method_owned<true,124>}, // SDK line183
    {"IsHasResearchAgreement",native_gamecore_slot_method_owned<true,125>}, // SDK line185
    {"IsHasTradeAgreement",native_gamecore_slot_method_owned<true,126>}, // SDK line186
    {"IsHasTech",native_gamecore_slot_method_owned<true,127>}, // SDK line188
    {"SetHasTech",native_gamecore_slot_method_owned<true,128>}, // SDK line189
    {"GetTeamTechs",native_gamecore_slot_method_owned<true,129>}, // SDK line191
    {"GetImprovementYieldChange",native_gamecore_slot_method_owned<true,130>}, // SDK line193
    {"ChangeImprovementYieldChange",native_gamecore_slot_method_owned<true,131>}, // SDK line194
    {"IsHomeOfUnitedNations",native_gamecore_slot_method_owned<true,132>}, // SDK line196
    {"GetVictoryCountdown",native_gamecore_slot_method_owned<true,133>}, // SDK line198
    {"GetVictoryDelay",native_gamecore_slot_method_owned<true,134>}, // SDK line199
    {"CanLaunch",native_gamecore_slot_method_owned<true,135>}, // SDK line200
    {"GetVictoryPoints",native_gamecore_slot_method_owned<true,136>}, // SDK line202
    {"ChangeVictoryPoints",native_gamecore_slot_method_owned<true,137>}, // SDK line203
    {"GetCurrentEra",native_gamecore_slot_method_owned<true,138>}, // SDK line205
    {"SetCurrentEra",native_gamecore_slot_method_owned<true,139>}, // SDK line206
    {"UpdateEmbarkGraphics",native_gamecore_slot_method_owned<true,140>}, // SDK line208
};
static_assert(std::size(native_gamecore_player_methods)==787&&std::size(native_gamecore_team_methods)==141);
[[noreturn]]static void native_gamecore_slot_registration_refuse_owned(lua_State*L,const char*message){
    owner(L).record_failure();luaL_error(L,"%s",message);std::terminate();
}
static std::pair<std::size_t,std::size_t> native_gamecore_register_slot_collection_owned(lua_State*L,bool team){
    auto*const s=native_gamecore_slots_lua_current_owned(L,true);auto&n=s->namespaces[team?1:0];
    const int saved_top=lua_gettop(L);const auto*const entries=team?native_gamecore_team_methods:native_gamecore_player_methods;
    const std::size_t count=team?std::size(native_gamecore_team_methods):std::size(native_gamecore_player_methods);
    if(s->bootstrap->export_stage!=(team?6U:5U))native_gamecore_slot_registration_refuse_owned(L,"GameCore Player/Team entered outside genuine SDK export order");
    if(n.phase!=NativeGameCorePlayerTeamNamespace::Phase::empty&&n.phase!=NativeGameCorePlayerTeamNamespace::Phase::returned)
        native_gamecore_slot_registration_refuse_owned(L,"GameCore Player/Team partial constructor cannot replay");
    if(n.phase==NativeGameCorePlayerTeamNamespace::Phase::returned)native_gamecore_slot_namespace_identity_owned(L,team,true);
    if(n.keys_ref<0){
        lua_createtable(L,static_cast<int>(count)+7,0);native_gamecore_slots_lua_current_owned(L,true);
        for(int i=1;i<=7;++i){
            const char*const key=i==1?"__instance":i==2?"__index":i==3?"__instances":i==4?"__mode":i==5?"v":i==6?(team?"Team":"Player"):(team?"Teams":"Players");
            lua_pushstring(L,key);lua_rawseti(L,-2,i);native_gamecore_slots_lua_current_owned(L,true);
        }
        for(std::size_t i=0;i<count;++i){lua_pushstring(L,entries[i].name);lua_rawseti(L,-2,static_cast<int>(i)+8);native_gamecore_slots_lua_current_owned(L,true);}
        n.keys=lua_topointer(L,-1);n.keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);n.phase=NativeGameCorePlayerTeamNamespace::Phase::keys_retained;
        native_gamecore_slots_lua_current_owned(L,true);
    }
    // CvLuaScopedInstance::Push looks up its type in actual scoped GLOBALS;
    // only nil mints it. A nonnil table may be reused only with our real receipt.
    native_gamecore_slot_key_owned(L,team,6);lua_gettable(L,LUA_GLOBALSINDEX);native_gamecore_slots_lua_current_owned(L,true);
    if(lua_isnil(L,-1)){
        if(n.type_ref>=0)native_gamecore_slot_registration_refuse_owned(L,"GameCore authenticated Player/Team type was removed");
        lua_pop(L,1);lua_createtable(L,0,static_cast<int>(count)+1);native_gamecore_slots_lua_current_owned(L,true);
        n.type=lua_topointer(L,-1);lua_pushvalue(L,-1);n.type_ref=luaL_ref(L,LUA_REGISTRYINDEX);n.phase=NativeGameCorePlayerTeamNamespace::Phase::type_created;
        native_gamecore_slot_key_owned(L,team,6);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_gamecore_slots_lua_current_owned(L,true);
        const int type=lua_gettop(L);
        lua_createtable(L,0,0);native_gamecore_slots_lua_current_owned(L,true);const int cache=lua_gettop(L);
        n.cache=lua_topointer(L,cache);lua_pushvalue(L,cache);n.cache_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        lua_createtable(L,0,1);native_gamecore_slots_lua_current_owned(L,true);const int weak=lua_gettop(L);
        native_gamecore_slot_key_owned(L,team,4);native_gamecore_slot_key_owned(L,team,5);lua_rawset(L,weak);
        n.weak_meta=lua_topointer(L,weak);lua_pushvalue(L,weak);n.weak_meta_ref=luaL_ref(L,LUA_REGISTRYINDEX);lua_setmetatable(L,cache);
        native_gamecore_slot_key_owned(L,team,3);lua_pushvalue(L,cache);lua_rawset(L,type);lua_pop(L,1);
        n.phase=NativeGameCorePlayerTeamNamespace::Phase::cache_created;native_gamecore_slots_lua_current_owned(L,true);
        for(std::size_t i=0;i<count;++i){
            native_gamecore_slot_key_owned(L,team,static_cast<int>(i)+8);lua_pushcclosure(L,entries[i].body,0);lua_rawset(L,type);
            ++n.methods;native_gamecore_slots_lua_current_owned(L,true);
        }
        n.phase=NativeGameCorePlayerTeamNamespace::Phase::methods_returned;
    }else{
        if(n.type_ref<0||!lua_istable(L,-1)||lua_topointer(L,-1)!=n.type||lua_getmetatable(L,-1))
            native_gamecore_slot_registration_refuse_owned(L,"GameCore Player/Team existing type is not an authenticated real SDK namespace");
    }
    lua_pop(L,1);
    native_gamecore_slot_key_owned(L,team,7);lua_gettable(L,LUA_GLOBALSINDEX);native_gamecore_slots_lua_current_owned(L,true);
    if(lua_isnil(L,-1)){
        if(n.collection_ref>=0)native_gamecore_slot_registration_refuse_owned(L,"GameCore authenticated64 collection was removed");
        lua_pop(L,1);lua_createtable(L,64,0);native_gamecore_slots_lua_current_owned(L,true);
        n.collection=lua_topointer(L,-1);lua_pushvalue(L,-1);n.collection_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        native_gamecore_slot_key_owned(L,team,7);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_gamecore_slots_lua_current_owned(L,true);
        n.phase=NativeGameCorePlayerTeamNamespace::Phase::collection_created;
    }else if(n.collection_ref<0||!lua_istable(L,-1)||lua_topointer(L,-1)!=n.collection||lua_getmetatable(L,-1)){
        native_gamecore_slot_registration_refuse_owned(L,"GameCore existing64 collection is not its authenticated SDK table");
    }
    const int collection=lua_gettop(L);n.instances_written=0;n.phase=NativeGameCorePlayerTeamNamespace::Phase::instances_entered;
    native_gamecore_slot_namespace_identity_owned(L,team,true);
    for(std::size_t id=0;id<64;++id){
        const void*const pointer=native_gamecore_actual_slot_pointer_owned(*s,team,id);
        // This pointer is an actual source-owned SavedPlayer/Team cell, already
        // checked for complete0..63 identities. Never push null for an absent slot.
        if(!pointer)native_gamecore_slot_registration_refuse_owned(L,"GameCore genuine64 slot unexpectedly absent");
        lua_rawgeti(L,LUA_REGISTRYINDEX,n.cache_ref);const int cache=lua_gettop(L);
        lua_pushlightuserdata(L,const_cast<void*>(pointer));lua_rawget(L,cache);native_gamecore_slots_lua_current_owned(L,true);
        if(lua_isnil(L,-1)){
            // Strong authentic collection custody keeps earlier original slots
            // alive. A missing already-issued cache entry is mutation, not a new
            // instance incarnation earned by this Source.
            if(n.instances[id])native_gamecore_slot_registration_refuse_owned(L,"GameCore authenticated actual pointer cache entry was removed");
            lua_pop(L,1);lua_createtable(L,0,1);native_gamecore_slots_lua_current_owned(L,true);const int instance=lua_gettop(L);
            native_gamecore_slot_key_owned(L,team,1);lua_pushlightuserdata(L,const_cast<void*>(pointer));lua_rawset(L,instance);
            lua_createtable(L,0,1);native_gamecore_slots_lua_current_owned(L,true);
            native_gamecore_slot_key_owned(L,team,2);lua_rawgeti(L,LUA_REGISTRYINDEX,n.type_ref);lua_rawset(L,-3);
            n.instance_metas[id]=lua_topointer(L,-1);lua_setmetatable(L,instance);n.instances[id]=lua_topointer(L,instance);
            lua_pushlightuserdata(L,const_cast<void*>(pointer));lua_pushvalue(L,instance);lua_rawset(L,cache);native_gamecore_slots_lua_current_owned(L,true);
        }else{
            if(!lua_istable(L,-1)||lua_topointer(L,-1)!=n.instances[id]||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=n.instance_metas[id])
                native_gamecore_slot_registration_refuse_owned(L,"GameCore actual pointer cache returned another instance/metatable");lua_pop(L,1);
        }
        lua_pushvalue(L,-1);lua_rawseti(L,collection,static_cast<int>(id));native_gamecore_slots_lua_current_owned(L,true);
        if(native_gamecore_slot_receiver_owned(L,team,true,-1)!=id)
            native_gamecore_slot_registration_refuse_owned(L,"GameCore actual64 slot publication differs");
        ++n.instances_written;lua_pop(L,2);
    }
    if(n.methods!=count||n.instances_written!=64)native_gamecore_slot_registration_refuse_owned(L,"GameCore original method/actual64 instance journal is incomplete");
    // Complete returned field identity, once; do not turn every callback into
    // an O(methods squared) method-table walk.
    lua_rawgeti(L,LUA_REGISTRYINDEX,n.type_ref);const int type=lua_gettop(L);
    for(std::size_t ordinal=0;ordinal<count;++ordinal){
        native_gamecore_slot_key_owned(L,team,static_cast<int>(ordinal)+8);std::size_t length=0;
        const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&length):nullptr;
        if(!key||length!=std::strlen(entries[ordinal].name)||std::memcmp(key,entries[ordinal].name,length))
            native_gamecore_slot_registration_refuse_owned(L,"GameCore original named instance method key changed");
        lua_rawget(L,type);if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=entries[ordinal].body||lua_getupvalue(L,-1,1))
            native_gamecore_slot_registration_refuse_owned(L,"GameCore original zero-upvalue instance method publication changed");lua_pop(L,1);
    }
    lua_pop(L,1);native_gamecore_slot_namespace_identity_owned(L,team,true);
    n.phase=NativeGameCorePlayerTeamNamespace::Phase::returned;
    lua_settop(L,saved_top);native_gamecore_slots_lua_current_owned(L,true);
    return {count,64};
}
static std::pair<std::size_t,std::size_t> native_gamecore_register_players_owned(lua_State*L){
    return native_gamecore_register_slot_collection_owned(L,false);
}
static std::pair<std::size_t,std::size_t> native_gamecore_register_teams_owned(lua_State*L){
    return native_gamecore_register_slot_collection_owned(L,true);
}
