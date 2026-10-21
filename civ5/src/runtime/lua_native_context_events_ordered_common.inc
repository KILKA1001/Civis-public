// Inside Host::Impl. The pinned retail79AA90/46B010 constructors, lists,
// callback environments, seven methods and GC differ only in proved native
// address operands. This common body ports those instructions; the generated
// inventory supplies actual original names/constructors/slots, never aliases.
enum NativeOrderedKey : int {
    ordered_callback_thread=1,ordered_event_handle,ordered_event,ordered_index,
    ordered_newindex,ordered_gc,ordered_call_meta,ordered_metatable,
    ordered_dispatch,ordered_add,ordered_call,ordered_call_immediate,
    ordered_count,ordered_remove,ordered_remove_all,ordered_mode,ordered_weak
};
static constexpr const char*const native_ordered_keys[]={
    "callback_thread","event_handle","_Event","__index","__newindex",
    "__gc","__call","__metatable","Dispatch","Add","Call",
    "CallImmediate","Count","Remove","RemoveAll","__mode","v"
};
enum class NativeOrderedMarshal : unsigned char {unissued,zero,int32,int2,reverse_int2};
struct NativeOrderedSpec final {
    const char* name;std::uint32_t registrar,constructor,list,environment,methods,meta,
        add,remove,gc,callback,register_callback,unregister_callback,subscriber_slot,
        call,immediate;
    NativeOrderedMarshal marshal;
};
struct NativeOrderedHandle final {
    Impl* host;std::size_t channel;lua_State* thread;
    const void* list;const void* event;const void* environment;const void* gc_meta;
    bool constructed,registered;
    std::size_t native_child=graph_no_owner; //Actual constructor producer, never caller-selected.
};
struct NativeOrderedQueuedFields final {
    std::uint64_t occurrence;std::array<std::int32_t,2> native_fields;
};
struct NativeOrderedChannel final {
    const void* created=nullptr;const void* assigned=nullptr;
    std::uint64_t queue_occurrence=0;
    std::vector<NativeOrderedHandle*> subscribers;
    std::deque<NativeOrderedQueuedFields> queue;
};
template<std::size_t I>static int native_ordered_constructor_index(lua_State* L){
    return native_ordered_constructor(L,I);
}
#include "lua_native_context_events_ordered_inventory.inc"
static bool native_ordered_binding_active(std::size_t index,std::size_t assigned){
    for(std::size_t i=index+1;i<assigned;++i)
        if(std::strcmp(native_ordered_specs[i].name,native_ordered_specs[index].name)==0)return false;
    return true;
}
static std::size_t native_ordered_active_binding_count(std::size_t assigned){
    std::size_t count=0;for(std::size_t i=0;i<assigned;++i)if(native_ordered_binding_active(i,assigned))++count;return count;
}
std::array<NativeOrderedChannel,native_ordered_specs.size()> native_ordered_channels;
int native_ordered_keys_ref=LUA_NOREF,native_ordered_names_ref=LUA_NOREF,native_ordered_weak_ref=LUA_NOREF;
int native_ordered_generator_ref=LUA_NOREF;
const void* native_ordered_keys_table=nullptr;const void* native_ordered_names_table=nullptr;
const void* native_ordered_weak_table=nullptr;const void* native_ordered_weak_meta=nullptr;
const void* native_ordered_generator_table=nullptr;
std::uint64_t native_ordered_epoch=0,native_ordered_writes=0,native_ordered_uncertainty=0;
bool native_ordered_retired=false;
static void native_ordered_key(lua_State* L,NativeOrderedKey key){
    auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_ordered_keys_ref);
    lua_rawgeti(L,-1,static_cast<int>(key));lua_remove(L,-2);
}
static void native_ordered_name(lua_State* L,std::size_t index){
    auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_ordered_names_ref);
    lua_rawgeti(L,-1,static_cast<int>(index)+1);lua_remove(L,-2);
}
static void native_ordered_channel_physical(lua_State* L){
    auto& p=owner(L);const int top=lua_gettop(L);
    if(std::this_thread::get_id()!=p.ui_thread||!p.state||!p.calling||p.native_ordered_retired||
       p.native_ordered_keys_ref<0||p.native_ordered_names_ref<0||p.native_ordered_weak_ref<0||
       !p.native_ordered_keys_table||!p.native_ordered_names_table||!p.native_ordered_weak_table||!p.native_ordered_weak_meta)
        return (void)luaL_error(L,"ordered Events actual channel/VM owner is absent or retired");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_ordered_keys_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.native_ordered_keys_table||lua_objlen(L,-1)!=17||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"ordered Events actual retained keys changed");
    const int keys=lua_gettop(L);
    for(int i=0;i<17;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char*const value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_ordered_keys[i])||std::memcmp(value,native_ordered_keys[i],n))
            return (void)luaL_error(L,"ordered Events retained key changed");
        lua_pop(L,1);
    }
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_ordered_names_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.native_ordered_names_table||
       lua_objlen(L,-1)!=native_ordered_specs.size()||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"ordered Events actual name keeper changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_ordered_weak_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.native_ordered_weak_table||
       !lua_getmetatable(L,-1)||lua_topointer(L,-1)!=p.native_ordered_weak_meta)
        return (void)luaL_error(L,"ordered Events actual weak watcher changed");
    native_ordered_key(L,ordered_mode);lua_rawget(L,-2);native_ordered_key(L,ordered_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"ordered Events watcher lost weak values");
    lua_settop(L,top);
}
static void native_ordered_current(lua_State* L,std::size_t index){
    auto& p=owner(L);native_ordered_channel_physical(L);
    if(index>=native_ordered_specs.size()||p.native_ordered_generator_ref<0||!p.native_ordered_generator_table||
       p.failure_epoch!=p.native_ordered_epoch||p.failed.writes_!=p.native_ordered_writes||
       p.execution_uncertainty!=p.native_ordered_uncertainty||p.failed.exhausted_)
        return (void)luaL_error(L,"ordered Events genuine constructor/failure owner changed");
    const auto& channel=p.native_ordered_channels[index];
    if(!channel.assigned||channel.assigned!=channel.created)
        return (void)luaL_error(L,"ordered Events constructor has no real native assignment");
    p.source_exception={};p.source_error=nullptr;
    try{
        if(p.worldview_native_host)p.worldview_require_callback_origin();
        else if(p.failed||!p.ready)throw ScriptError("ordered Events has no healthy active Host operation");
    }catch(...){p.source_exception=std::current_exception();p.source_error="ordered Events active operation refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);native_ordered_name(L,index);std::size_t n=0;
    const char*const name=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
    if(!name||n!=std::strlen(native_ordered_specs[index].name)||std::memcmp(name,native_ordered_specs[index].name,n))
        return (void)luaL_error(L,"ordered Events genuine native name changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_ordered_generator_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.native_ordered_generator_table||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"ordered Events actual generator table changed");
    native_ordered_name(L,index);lua_rawget(L,-2);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=channel.assigned||
       lua_tocfunction(L,-1)!=native_ordered_constructors[index]||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"ordered Events actual native zero-upvalue constructor changed");
    lua_settop(L,top);native_ordered_channel_physical(L);
}
void native_ordered_unsubscribe(NativeOrderedHandle* h)noexcept{
    auto& channel=native_ordered_channels[native_ordered_channel_owners[h->channel]];
    const auto i=std::find(channel.subscribers.begin(),channel.subscribers.end(),h);
    if(i!=channel.subscribers.end())channel.subscribers.erase(i);h->registered=false;
}
void native_ordered_retire()noexcept{
    native_ordered_retired=true;
    for(auto& channel:native_ordered_channels){channel.subscribers.clear();channel.queue.clear();}
}
static int native_ordered_gc(lua_State* L){
    auto& p=owner(L);
    if(lua_type(L,1)!=LUA_TUSERDATA||lua_objlen(L,1)!=sizeof(NativeOrderedHandle))
        return luaL_error(L,"ordered Events __gc requires its actual wide handle");
    auto*const h=static_cast<NativeOrderedHandle*>(lua_touserdata(L,1));
    if(!h||h->host!=&p)return 0;
    if(h->channel>=native_ordered_specs.size())return luaL_error(L,"ordered Events real handle channel changed");
    if(h->registered)p.native_ordered_unsubscribe(h);return 0;
}
static NativeOrderedHandle* native_ordered_list_handle(lua_State* L){
    native_ordered_channel_physical(L);auto& p=owner(L);const int top=lua_gettop(L);
    if(!lua_istable(L,lua_upvalueindex(1))||lua_getmetatable(L,lua_upvalueindex(1)))
        return (luaL_error(L,"ordered Events actual list upvalue changed"),nullptr);
    const void*const list=lua_topointer(L,lua_upvalueindex(1));
    native_ordered_key(L,ordered_event_handle);lua_rawget(L,lua_upvalueindex(1));
    if(lua_type(L,-1)!=LUA_TUSERDATA||lua_objlen(L,-1)!=sizeof(NativeOrderedHandle))
        return (luaL_error(L,"ordered Events actual list handle changed"),nullptr);
    auto*const h=static_cast<NativeOrderedHandle*>(lua_touserdata(L,-1));
    if(!h||h->host!=&p||h->channel>=native_ordered_specs.size()||!h->constructed||h->list!=list||
       !h->thread||!h->environment||!h->event||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=h->gc_meta)
        return (luaL_error(L,"ordered Events list/thread/GC owner changed"),nullptr);
    native_ordered_current(L,h->channel);
    native_ordered_key(L,ordered_gc);lua_rawget(L,-2);
    if(lua_tocfunction(L,-1)!=native_ordered_gc)
        return (luaL_error(L,"ordered Events genuine native finalizer changed"),nullptr);
    lua_settop(L,top);native_ordered_key(L,ordered_callback_thread);lua_rawget(L,lua_upvalueindex(1));
    if(lua_tothread(L,-1)!=h->thread||&owner(h->thread)!=&p)
        return (luaL_error(L,"ordered Events genuine callback thread changed"),nullptr);
    lua_getfenv(L,-1);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=h->environment)
        return (luaL_error(L,"ordered Events genuine callback environment changed"),nullptr);
    native_ordered_key(L,ordered_event);lua_rawget(L,-2);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=h->event)
        return (luaL_error(L,"ordered Events callback _Event changed"),nullptr);
    lua_settop(L,top);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_ordered_weak_ref);
    lua_pushlightuserdata(L,h);lua_rawget(L,-2);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=list)
        return (luaL_error(L,"ordered Events actual weak lifecycle entry changed"),nullptr);
    lua_settop(L,top);return h;
}
static int native_ordered_add(lua_State* L){
    auto*const h=native_ordered_list_handle(L);auto& p=owner(L);auto& channel=p.native_ordered_channels[native_ordered_channel_owners[h->channel]];
    if(!h->registered){
        p.source_exception={};p.source_error=nullptr;
        try{channel.subscribers.push_back(h);}catch(...){p.source_exception=std::current_exception();p.source_error="ordered Events subscription allocation failed";}
        if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}h->registered=true;
    }
    lua_pushvalue(L,-1);const auto length=lua_objlen(L,lua_upvalueindex(1));
    if(length>=static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return luaL_error(L,"ordered Events list exceeds original rawseti width");
    lua_rawseti(L,lua_upvalueindex(1),static_cast<int>(length)+1);return 1;
}
static int native_ordered_count(lua_State* L){
    (void)native_ordered_list_handle(L);lua_pushinteger(L,static_cast<lua_Integer>(lua_objlen(L,lua_upvalueindex(1))));return 1;
}
static int native_ordered_remove(lua_State* L){
    auto*const h=native_ordered_list_handle(L);auto& p=owner(L);const auto length=lua_objlen(L,lua_upvalueindex(1));
    if(length>=static_cast<std::size_t>(std::numeric_limits<int>::max()))return luaL_error(L,"ordered Events list exceeds original rawseti width");
    int i=0;lua_pushnil(L);
    do{
        if(lua_rawequal(L,-2,-1))for(int j=i;j<=static_cast<int>(length);++j){lua_rawgeti(L,lua_upvalueindex(1),j+1);lua_rawseti(L,lua_upvalueindex(1),j);}
        ++i;lua_pop(L,1);lua_rawgeti(L,lua_upvalueindex(1),i);
    }while(!lua_isnil(L,-1));
    (void)lua_objlen(L,lua_upvalueindex(1));
    // Every proved native Remove tests initial EBX, retaining duplicate skip.
    if(length==0&&h->registered)p.native_ordered_unsubscribe(h);return 0;
}
static int native_ordered_remove_all(lua_State* L){
    (void)native_ordered_list_handle(L);const auto length=lua_objlen(L,lua_upvalueindex(1));
    if(length>=static_cast<std::size_t>(std::numeric_limits<int>::max()))return luaL_error(L,"ordered Events list exceeds original rawseti width");
    for(int i=1;i<=static_cast<int>(length);++i){lua_pushnil(L);lua_rawseti(L,lua_upvalueindex(1),i);}return 0;
}
static int native_ordered_dispatch_body(lua_State* L){
    auto*const dispatched=native_ordered_list_handle(L);const int args=lua_gettop(L);
    const auto length=lua_objlen(L,lua_upvalueindex(1));
    if(length>=static_cast<std::size_t>(std::numeric_limits<int>::max())||args==std::numeric_limits<int>::max()||!lua_checkstack(L,args+1))
        return luaL_error(L,"ordered Events original Dispatch stack/index width unavailable");
    auto& p=owner(L);const auto previous=p.graph_execution_scope;
    for(int i=1;i<=static_cast<int>(length);++i){
        lua_rawgeti(L,lua_upvalueindex(1),i);const auto compiled=p.graph_compiled_owner(L,-1);
        for(int j=1;j<=args;++j)lua_pushvalue(L,j);
        p.graph_execution_scope=compiled;lua_call(L,args,0);p.graph_execution_scope=previous;
        if(native_ordered_list_handle(L)!=dispatched)
            return luaL_error(L,"actual ordered Events Dispatch handle changed after listener return");
    }return 0;
}
static int native_ordered_dispatch(lua_State* L){
    (void)native_ordered_list_handle(L);const int args=lua_gettop(L);auto& p=owner(L);
    if(args==std::numeric_limits<int>::max()||!lua_checkstack(L,2))return luaL_error(L,"ordered Events protected Dispatch stack unavailable");
    const auto previous=p.graph_execution_scope;
    lua_pushvalue(L,lua_upvalueindex(1));lua_pushcclosure(L,native_ordered_dispatch_body,1);lua_insert(L,1);
    const int status=lua_pcall(L,args,0,0);p.graph_execution_scope=previous;if(status)return lua_error(L);return 0;
}
static std::int32_t native_ordered_integer(lua_State* L,int arg){
    const auto value=lua_tointeger(L,arg);
    if(value<std::numeric_limits<std::int32_t>::min()||value>std::numeric_limits<std::int32_t>::max())
        luaL_error(L,"ordered Events actual native32 conversion is outside proven range");
    return static_cast<std::int32_t>(value);
}
static std::array<std::int32_t,2> native_ordered_value(lua_State* L,const NativeOrderedSpec& spec){
    switch(spec.marshal){
    case NativeOrderedMarshal::zero:return {};
    case NativeOrderedMarshal::int32:return {native_ordered_integer(L,1),0};
    case NativeOrderedMarshal::int2:{const auto a=native_ordered_integer(L,1);const auto b=native_ordered_integer(L,2);return {a,b};}
    case NativeOrderedMarshal::reverse_int2:{const auto a=native_ordered_integer(L,1);const auto b=native_ordered_integer(L,2);return {b,a};}
    case NativeOrderedMarshal::unissued:
        luaL_error(L,"%s requires its original Call %08X argument marshaller; no invented payload",spec.name,spec.call);
    }return {};
}
static int native_ordered_call(lua_State* L){
    auto*const h=native_ordered_list_handle(L);auto& p=owner(L);auto& channel=p.native_ordered_channels[native_ordered_channel_owners[h->channel]];
    if(h->channel==native_clear_highlight_channel){
        //597190/461400 CString ABI, actual selected Scene queue only.
        graphics::BnwClearHexHighlightOwner::Payload payload;
        const char*const text=lua_tolstring(L,1,nullptr);
        if(!text)return luaL_error(L,"ClearHexHighlightStyle NULL/nonstrings original payload is UNISSUED; no invented empty style");
        if(text)for(std::size_t i=0;i<127&&text[i];++i)payload.style[i]=text[i];
        p.source_exception={};p.source_error=nullptr;
        try{
            p.worldview_require_callback_origin();
            const BnwWorldViewSourceOperationLoan* current=nullptr;
            {const auto source=p.worldview_sources.lock();
             if(!source)throw ScriptError("Highlight actual Source retired");
             current=&source->current_operation_loan();
             if(current!=p.worldview_operation)throw ScriptError("Highlight actual current Source operation replaced");}
            p.worldview_require_callback_origin();current->require_same_vm(*p.worldview_host_owner);p.worldview_require_callback_origin();
            {const auto receiver=current->clear_hex_highlight_owner_owned();
             p.worldview_require_callback_origin();receiver->enqueue_owned(*current,payload);p.worldview_require_callback_origin();}
            //Borrower retired before final Source/Host POST and any lua_error.
            p.worldview_require_callback_origin();current->require_current();p.worldview_require_callback_origin();
        }catch(...){p.source_exception=std::current_exception();p.source_error="ClearHexHighlightStyle actual payload/Scene queue refused";}
        if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
        return 0;
    }
    const auto fields=native_ordered_value(L,native_ordered_specs[h->channel]);
    if(channel.queue_occurrence==UINT64_MAX)return luaL_error(L,"ordered Events actual queue occurrence exhausted");
    p.source_exception={};p.source_error=nullptr;
    try{channel.queue.push_back({channel.queue_occurrence+1,fields});}
    catch(...){p.source_exception=std::current_exception();p.source_error="ordered Events deferred allocation failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}++channel.queue_occurrence;return 0;
}
static int native_ordered_call_immediate(lua_State* L){
    auto*const h=native_ordered_list_handle(L);(void)native_ordered_value(L,native_ordered_specs[h->channel]);
    return luaL_error(L,"%s CallImmediate requires its authentic client dispatcher owner; emitter unissued",native_ordered_specs[h->channel].name);
}
static int native_ordered_call_meta(lua_State* L){
    const int count=lua_gettop(L);lua_pushvalue(L,lua_upvalueindex(1));lua_insert(L,2);lua_call(L,count-1,0);return 0;
}
static int native_ordered_constructor(lua_State* L,std::size_t index){
    native_ordered_current(L,index);
    if(!lua_checkstack(L,24))return luaL_error(L,"ordered Events real native constructor stack unavailable");
    lua_createtable(L,0,0);const int event=lua_gettop(L);const void*const event_pointer=lua_topointer(L,event);
    lua_createtable(L,0,1);const int list=lua_gettop(L);const void*const list_pointer=lua_topointer(L,list);
    native_ordered_key(L,ordered_callback_thread);lua_State*const thread=lua_newthread(L);
    lua_createtable(L,0,1);const int environment=lua_gettop(L);const void*const environment_pointer=lua_topointer(L,environment);
    native_ordered_key(L,ordered_event);lua_pushvalue(L,event);lua_rawset(L,environment);
    lua_createtable(L,0,2);const int environment_meta=lua_gettop(L);
    native_ordered_key(L,ordered_index);lua_pushvalue(L,LUA_GLOBALSINDEX);lua_rawset(L,environment_meta);
    native_ordered_key(L,ordered_newindex);lua_pushvalue(L,LUA_GLOBALSINDEX);lua_rawset(L,environment_meta);
    lua_setmetatable(L,environment);lua_setfenv(L,-2);lua_rawset(L,list);
    native_ordered_key(L,ordered_event_handle);
    auto*const h=static_cast<NativeOrderedHandle*>(lua_newuserdata(L,sizeof(NativeOrderedHandle)));
    *h={&owner(L),index,thread,list_pointer,event_pointer,environment_pointer,nullptr,false,false,
        owner(L).worldview_native_host?owner(L).worldview_native_host->child:graph_no_owner};
    lua_createtable(L,0,1);h->gc_meta=lua_topointer(L,-1);
    native_ordered_key(L,ordered_gc);lua_pushcclosure(L,native_ordered_gc,0);lua_rawset(L,-3);
    lua_setmetatable(L,-2);lua_rawset(L,list);
    static constexpr NativeOrderedKey fields[]={ordered_add,ordered_call,ordered_call_immediate,ordered_count,ordered_dispatch,ordered_remove,ordered_remove_all};
    static constexpr lua_CFunction methods[]={native_ordered_add,native_ordered_call,native_ordered_call_immediate,native_ordered_count,native_ordered_dispatch,native_ordered_remove,native_ordered_remove_all};
    for(int i=0;i<7;++i){native_ordered_key(L,fields[i]);lua_pushvalue(L,list);lua_pushcclosure(L,methods[i],1);lua_rawset(L,event);}
    lua_createtable(L,0,3);const int event_meta=lua_gettop(L);
    native_ordered_key(L,ordered_newindex);lua_pushnil(L);lua_rawset(L,event_meta);
    native_ordered_key(L,ordered_metatable);lua_createtable(L,0,0);lua_rawset(L,event_meta);
    native_ordered_key(L,ordered_call_meta);native_ordered_key(L,ordered_call);lua_rawget(L,event);
    lua_pushcclosure(L,native_ordered_call_meta,1);lua_rawset(L,event_meta);lua_setmetatable(L,event);
    auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_ordered_weak_ref);
    lua_pushlightuserdata(L,h);lua_pushvalue(L,list);lua_rawset(L,-3);lua_pop(L,1);
    h->constructed=true;lua_settop(L,event);native_ordered_current(L,index);return 1;
}
