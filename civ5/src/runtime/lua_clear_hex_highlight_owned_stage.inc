//Inside the real Host::Impl after actual ordered Events common code.
//No owning C++ automatic local surrounds a Lua API in either CFunction.
static constexpr std::size_t native_clear_highlight_channel=35;
static_assert(std::string_view(native_ordered_specs[native_clear_highlight_channel].name)=="ClearHexHighlightStyle"&&
 native_ordered_specs[native_clear_highlight_channel].call==0x597190u&&
 native_ordered_specs[native_clear_highlight_channel].constructor==0x61EB20u&&
 native_ordered_channel_owners[native_clear_highlight_channel]==native_clear_highlight_channel);
struct NativeClearHighlightListener final {
 NativeOrderedHandle* handle=nullptr;int list_ref=LUA_NOREF;lua_State* thread=nullptr;
 std::size_t child=graph_no_owner;const void*list=nullptr;const void*environment=nullptr;const void*event=nullptr;
};
struct NativeClearHighlightJob final {
 Impl* owner=nullptr;NativeClearHighlightListener*listener=nullptr;
 graphics::BnwClearHexHighlightOwner::Payload payload;
 bool entered=false,returned=false,present=false;
};
struct NativeClearHighlightRelease final {Impl*owner;std::vector<NativeClearHighlightListener>*listeners;};
static int native_clear_highlight_pin_listener(lua_State* L){
 auto*const job=static_cast<NativeClearHighlightJob*>(lua_touserdata(L,1));auto&p=owner(L);
 if(lua_gettop(L)!=1||lua_type(L,1)!=LUA_TLIGHTUSERDATA||!job||job->owner!=&p||!job->listener)
  return luaL_error(L,"Highlight subscriber pin lacks actual protected Host job");
 native_ordered_current(L,native_clear_highlight_channel);
 auto& row=*job->listener;
 lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_ordered_weak_ref);lua_pushlightuserdata(L,row.handle);lua_rawget(L,-2);
 //The original weak lifecycle may have genuinely collected an unsubscribed
 //handle during earlier allocation. Never dereference a raw snapshot first.
 if(lua_isnil(L,-1)){
  const auto& subscribers=p.native_ordered_channels[native_clear_highlight_channel].subscribers;
  if(std::find(subscribers.begin(),subscribers.end(),row.handle)!=subscribers.end())
   return luaL_error(L,"Highlight registered subscriber lost its real weak list");
  job->entered=true;job->returned=true;return 0;
 }
 if(!lua_istable(L,-1)||lua_getmetatable(L,-1))return luaL_error(L,"Highlight subscriber actual list changed");
 const int list=lua_gettop(L);row.list=lua_topointer(L,list);
 native_ordered_key(L,ordered_event_handle);lua_rawget(L,list);
 if(lua_type(L,-1)!=LUA_TUSERDATA||lua_objlen(L,-1)!=sizeof(NativeOrderedHandle)||lua_touserdata(L,-1)!=row.handle)
  return luaL_error(L,"Highlight actual registered handle userdata changed");
 auto*const h=static_cast<NativeOrderedHandle*>(lua_touserdata(L,-1));
 if(h->host!=&p||h->channel!=native_clear_highlight_channel||!h->constructed||!h->registered||
    h->list!=row.list||!h->thread||!h->event||!h->environment||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=h->gc_meta)
  return luaL_error(L,"Highlight original subscriber/list/GC provenance changed");
 row.thread=h->thread;row.child=h->native_child;row.environment=h->environment;row.event=h->event;
 lua_pop(L,2);native_ordered_key(L,ordered_callback_thread);lua_rawget(L,list);
 if(!lua_isthread(L,-1)||lua_tothread(L,-1)!=row.thread||&owner(row.thread)!=&p)
  return luaL_error(L,"Highlight actual callback thread owner changed");
 lua_getfenv(L,-1);
 if(lua_topointer(L,-1)!=row.environment)return luaL_error(L,"Highlight actual callback environment changed");
 native_ordered_key(L,ordered_event);lua_rawget(L,-2);
 if(!lua_istable(L,-1)||lua_topointer(L,-1)!=row.event)return luaL_error(L,"Highlight actual callback Event changed");
 lua_pushvalue(L,list);row.list_ref=luaL_ref(L,LUA_REGISTRYINDEX);
 job->present=true;job->entered=true;job->returned=true;return 0;
}
static int native_clear_highlight_dispatch_listener(lua_State* L){
 auto*const job=static_cast<NativeClearHighlightJob*>(lua_touserdata(L,1));auto&p=owner(L);
 if(lua_gettop(L)!=1||lua_type(L,1)!=LUA_TLIGHTUSERDATA||!job||job->owner!=&p||!job->listener||
    job->listener->thread!=L||job->payload.style.back()!=0)
  return luaL_error(L,"Highlight actual713170 protected callback job changed");
 native_ordered_current(L,native_clear_highlight_channel);
 auto& row=*job->listener;
 if(row.list_ref<0)return luaL_error(L,"Highlight actual callback list keeper retired");
 lua_rawgeti(L,LUA_REGISTRYINDEX,row.list_ref);const int list=lua_gettop(L);
 if(!lua_istable(L,list)||lua_topointer(L,list)!=row.list||lua_getmetatable(L,list))
  return luaL_error(L,"Highlight callback actual retained list changed");
 native_ordered_key(L,ordered_event_handle);lua_rawget(L,list);
 if(lua_type(L,-1)!=LUA_TUSERDATA||lua_objlen(L,-1)!=sizeof(NativeOrderedHandle)||lua_touserdata(L,-1)!=row.handle)
  return luaL_error(L,"Highlight callback retained actual handle changed");
 auto*const h=static_cast<NativeOrderedHandle*>(lua_touserdata(L,-1));
 if(h->host!=&p||h->channel!=native_clear_highlight_channel||!h->constructed||h->list!=row.list||
    h->thread!=L||h->environment!=row.environment||h->event!=row.event||h->native_child!=row.child)
  return luaL_error(L,"Highlight callback actual handle/thread/producer identity changed");
 native_ordered_key(L,ordered_callback_thread);lua_rawget(L,list);lua_getfenv(L,-1);
 if(lua_tothread(L,-2)!=L||lua_topointer(L,-1)!=row.environment)
  return luaL_error(L,"Highlight callback real coroutine/fenv changed");
 native_ordered_key(L,ordered_event);lua_rawget(L,-2);
 if(!lua_istable(L,-1)||lua_topointer(L,-1)!=row.event)return luaL_error(L,"Highlight callback original _Event replaced");
 native_ordered_key(L,ordered_dispatch);lua_rawget(L,-2);
 if(lua_tocfunction(L,-1)!=native_ordered_dispatch||!lua_getupvalue(L,-1,1)||lua_topointer(L,-1)!=row.list)
  return luaL_error(L,"Highlight actual original Dispatch closure/list changed");
 lua_pop(L,1);job->entered=true;lua_pushstring(L,job->payload.style.data());
 lua_call(L,1,0); //4F77A0/43AD50: actual style CString is one Lua argument.
 native_ordered_current(L,native_clear_highlight_channel);
 job->returned=true;return 0;
}
static int native_clear_highlight_release_listeners(lua_State* L){
 auto*const job=static_cast<NativeClearHighlightRelease*>(lua_touserdata(L,1));auto&p=owner(L);
 if(!job||job->owner!=&p||!job->listeners||p.ui_thread!=std::this_thread::get_id())
  return luaL_error(L,"Highlight cleanup lost actual Host/creator");
 for(auto& row:*job->listeners){const int ref=row.list_ref;row.list_ref=LUA_NOREF;
  if(ref>=0)luaL_unref(L,LUA_REGISTRYINDEX,ref);}
 return 0;
}
void drain_clear_hex_highlight_owned(const BnwWorldViewSourceOperationLoan& op){
 worldview_require_callback_origin();op.require_same_vm(*worldview_host_owner);
 if(calling||!worldview_native_host||worldview_callback_dispatching||worldview_control_entry||worldview_control_validation)
  throw ScriptError("Highlight deferred drain requires returned genuine protected script call");
 //Only this newly implemented typed event can be consumed. A preceding
 //unsupported ordered record is refused; no hidden global FIFO skip.
 for(std::size_t i=0;i<native_ordered_channels.size();++i)
  if(!native_ordered_channels[i].queue.empty())throw ScriptError("Highlight drain cannot bypass another UNISSUED ordered Events record");
 auto receiver=op.clear_hex_highlight_owner_owned();worldview_require_callback_origin();
 const auto cutoff=receiver->begin_drain_owned(op);worldview_require_callback_origin();
 std::vector<NativeClearHighlightListener> listeners;
 const auto old_graph=graph_execution_scope;const auto old_remaining=remaining;const auto old_exhausted=exhausted;
 const auto old_child=worldview_native_host->child;const auto old_source=worldview_native_host->child_source;
 std::exception_ptr primary;
 try{
  const auto snapshot=native_ordered_channels[native_clear_highlight_channel].subscribers;
  listeners.reserve(snapshot.size());for(auto*h:snapshot)listeners.push_back({h});
  worldview_require_callback_origin();receiver->require_current_owned(op);
  for(auto& listener:listeners){
   NativeClearHighlightJob job{this,&listener};
   const int top=lua_gettop(state);const auto previous_hook=lua_gethook(state);
   const int mask=lua_gethookmask(state),count=lua_gethookcount(state);
   calling=true;remaining=budget.instructions;exhausted=false;
   lua_sethook(state,budget.instructions?hook:nullptr,budget.instructions?LUA_MASKCOUNT:0,1000);
   const int status=lua_cpcall(state,native_clear_highlight_pin_listener,&job);
   calling=false;lua_sethook(state,previous_hook,mask,count);graph_execution_scope=old_graph;
   if(status){const auto error=worldview_callback_lua_error(state);lua_settop(state,top);throw ScriptError(error);}
   lua_settop(state,top);
   if(exhausted||!job.entered||!job.returned)throw ScriptError("Highlight actual protected subscriber pin did not return");
   worldview_require_callback_origin();receiver->require_current_owned(op);
  }
  while(const auto row=receiver->peek_drain_owned(op,cutoff)){
   worldview_require_callback_origin();receiver->dispatch_native_owned(op,*row);worldview_require_callback_origin();
   for(auto& listener:listeners)if(listener.list_ref>=0){
    if(!listener.thread||lua_status(listener.thread)||lua_gettop(listener.thread)!=0)
     throw ScriptError("Highlight real callback coroutine is not idle");
    if(listener.child>=child_contexts.size()||child_contexts[listener.child].retired||!child_contexts[listener.child].source)
     throw ScriptError("Highlight actual listener producer child/source unavailable");
    worldview_native_host->child=listener.child;worldview_native_host->child_source=child_contexts[listener.child].source;
    worldview_require_callback_origin();op.require_same_source(*child_contexts[listener.child].source);worldview_require_callback_origin();
    NativeClearHighlightJob job{this,&listener,row->payload};auto*const L=listener.thread;
    const auto previous_hook=lua_gethook(L);const int mask=lua_gethookmask(L),count=lua_gethookcount(L);
    calling=true;remaining=budget.instructions;exhausted=false;
    lua_sethook(L,budget.instructions?hook:nullptr,budget.instructions?LUA_MASKCOUNT:0,1000);
    const int status=lua_cpcall(L,native_clear_highlight_dispatch_listener,&job);
    calling=false;lua_sethook(L,previous_hook,mask,count);graph_execution_scope=old_graph;
    worldview_native_host->child=old_child;worldview_native_host->child_source=old_source;
    if(status){const auto error=worldview_callback_lua_error(L);lua_settop(L,0);throw ScriptError(error);}
    lua_settop(L,0);
    if(exhausted||!job.entered||!job.returned)throw ScriptError("Highlight713170 actual protected subscriber did not return");
    worldview_require_callback_origin();receiver->require_current_owned(op);
   }
   receiver->commit_drain_owned(op,*row);worldview_require_callback_origin();
  }
 }catch(...){primary=std::current_exception();receiver->quarantine_owned();}
 calling=false;graph_execution_scope=old_graph;remaining=old_remaining;exhausted=old_exhausted;
 worldview_native_host->child=old_child;worldview_native_host->child_source=old_source;
 //All registry borrower refs retire under a genuine protected cleanup before
 //Source POST. No destructor attempts allocating Lua API after a longjmp.
 NativeClearHighlightRelease release{this,&listeners};const int cleanup_top=lua_gettop(state);
 const int cleanup_status=lua_cpcall(state,native_clear_highlight_release_listeners,&release);
 if(cleanup_status&&!primary){primary=std::make_exception_ptr(ScriptError(worldview_callback_lua_error(state)));receiver->quarantine_owned();}
 lua_settop(state,cleanup_top);listeners.clear();
 if(!primary){try{receiver->finish_drain_owned(op,cutoff);}catch(...){primary=std::current_exception();receiver->quarantine_owned();}}
 receiver.reset(); //Target borrower ends BEFORE fresh Source/VM POST.
 try{worldview_require_callback_origin();op.require_current();worldview_require_callback_origin();}
 catch(...){if(!primary)primary=std::current_exception();}
 if(primary){record_failure();if(end_turn_lifetime)end_turn_lifetime->failed=true;std::rethrow_exception(primary);}
}
