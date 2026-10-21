// Included inside SAME LuaScriptHost::Impl before actual Game read lender.
// Issued only around real protected invocation of an immutable registered
// callback. This scope has no health-instance, construction or frame authority.
struct WorldViewLoadedChildCallScope;
WorldViewLoadedChildCallScope* worldview_loaded_child_call=nullptr;
struct WorldViewLoadedChildCallScope final {
 enum class Kind {control,context,created,damage,showhide};
 Impl& p;const Kind kind;const std::size_t index;
 const std::thread::id creator=std::this_thread::get_id();
 WorldViewNativeHostScope*const native;const BnwWorldViewSourceOperationLoan*const loan;
 const std::uint64_t epoch,writes,uncertainty,serial;
 const bool calling;
 std::size_t child=graph_no_owner,compiled=graph_no_owner,graph_slot=graph_no_owner;
 int environment=LUA_NOREF,thread=LUA_NOREF;
 lua_State* registered=nullptr;
 const Proto* root=nullptr;const void* closure=nullptr;const void* environment_identity=nullptr;
 std::weak_ptr<const ScriptUiChildSource>source;const ScriptUiChildSource*source_get=nullptr;
 std::weak_ptr<ScriptUiControlGraph>graph;const ScriptUiControlGraph*graph_get=nullptr;
 bool returned=false;mutable bool checking=false;
 void fail()const noexcept{p.record_failure();if(p.end_turn_lifetime)p.end_turn_lifetime->failed=true;}
 WorldViewLoadedChildCallScope(Impl&actual,Kind issuer,std::size_t actual_index):p(actual),kind(issuer),index(actual_index),
  native(p.worldview_native_host),loan(p.worldview_operation),epoch(p.failure_epoch),writes(p.failed.writes_),
  uncertainty(p.execution_uncertainty),serial(native?native->serial:0),calling(p.calling){
  p.worldview_require_callback_origin();
  if(creator!=p.ui_thread||!native||!loan||!p.worldview_callback_dispatching||p.worldview_loaded_child_call){
   fail();throw ScriptError("WorldView loaded call lacks actual protected native callback issuer");}
  if(kind==Kind::control){
   if(index>=p.worldview_callbacks.size()){fail();throw ScriptError("WorldView control callback occurrence absent");}
   const auto&e=p.worldview_callbacks[index];
   child=e.child;compiled=e.compiled;graph_slot=e.graph_slot;environment=e.environment;thread=e.thread;
   registered=e.registered;root=e.root;closure=e.closure;environment_identity=e.environment_identity;
   source=e.source;source_get=e.source.get();graph=e.graph;
  }else if(kind==Kind::context){
   if(index>=p.worldview_context_callbacks.size()){fail();throw ScriptError("WorldView context callback occurrence absent");}
   const auto&e=p.worldview_context_callbacks[index];
   child=e.child;compiled=e.compiled;graph_slot=e.graph_slot;environment=e.environment;thread=e.thread;
   registered=e.registered;root=e.root;closure=e.closure;environment_identity=e.environment_identity;
   source=e.source;source_get=e.source.get();graph=e.graph;
  }else if(kind==Kind::showhide){
   if(index>=p.worldview_showhide_callbacks.size()){fail();throw ScriptError("Context showhide callback occurrence absent");}
   const auto&e=p.worldview_showhide_callbacks[index];
   child=e.child;compiled=e.compiled;graph_slot=e.graph_slot;environment=e.environment;thread=e.thread;
   registered=e.registered;root=e.root;closure=e.closure;environment_identity=e.environment_identity;
   source=e.source;source_get=e.source.get();graph=e.graph;
  }else if(kind==Kind::created){
   if(index>=p.worldview_created_registrations.size()){fail();throw ScriptError("WorldView created occurrence absent");}
   const auto&e=p.worldview_created_registrations[index];
   child=e.child;compiled=e.compiled;graph_slot=e.graph_slot;environment=e.environment;thread=e.thread;
   registered=e.registered;root=e.root;closure=e.closure;environment_identity=e.environment_identity;
   source=e.source;source_get=e.source.get();graph=e.graph;
  }else{
   if(index>=p.worldview_damage_registrations.size()){fail();throw ScriptError("WorldView damage occurrence absent");}
   const auto&r=p.worldview_damage_registrations[index];
   if(r.created>=p.worldview_created_registrations.size()){fail();throw ScriptError("WorldView damage original parent absent");}
   const auto&e=p.worldview_created_registrations[r.created];
   child=e.child;compiled=e.compiled;graph_slot=e.graph_slot;environment=e.environment;thread=r.thread;
   registered=r.registered;root=e.root;closure=r.closure;environment_identity=e.environment_identity;
   source=e.source;source_get=e.source.get();graph=e.graph;
  }
  if(graph_slot<p.control_graphs.size())graph_get=p.control_graphs[graph_slot].graph.get();
  // Publish only after storage exists. pure() still proves all genuine facts;
  // a caller-supplied Kind/index or loaded boolean alone cannot pass admission.
  p.worldview_loaded_child_call=this;
  try{current();}catch(...){if(p.worldview_loaded_child_call==this)p.worldview_loaded_child_call=nullptr;fail();throw;}
 }
 void pure()const{
  if(creator!=std::this_thread::get_id()){fail();throw ScriptError("WorldView loaded call foreign creator");}
  if(!native||native!=p.worldview_native_host){fail();throw ScriptError("WorldView loaded call native lender retired");}
  native->pure();
  const auto same=[](const auto&a,const auto&b){return !a.owner_before(b)&&!b.owner_before(a);};
  const auto entry=[&](const auto&e){
   return !e.retired&&e.epoch==epoch&&e.uncertainty==uncertainty&&e.child==child&&e.compiled==compiled&&
    e.graph_slot==graph_slot&&e.environment==environment&&e.thread==thread&&e.registered==registered&&
    e.root==root&&e.closure==closure&&e.environment_identity==environment_identity&&
    e.source.get()==source_get&&same(e.source,source)&&same(e.graph,graph);
  };
  bool owned=false;
  if(kind==Kind::control)owned=index<p.worldview_callbacks.size()&&entry(p.worldview_callbacks[index]);
  else if(kind==Kind::context)owned=index<p.worldview_context_callbacks.size()&&entry(p.worldview_context_callbacks[index])&&
   p.worldview_context_callbacks[index].published&&!p.worldview_context_callbacks[index].shutdown_returned&&p.worldview_context_dispatching;
  else if(kind==Kind::showhide)owned=index<p.worldview_showhide_callbacks.size()&&entry(p.worldview_showhide_callbacks[index])&&
   p.worldview_showhide_callbacks[index].published&&p.worldview_showhide_dispatching;
  else if(kind==Kind::created)owned=index<p.worldview_created_registrations.size()&&entry(p.worldview_created_registrations[index])&&p.worldview_created_dispatching;
  else if(index<p.worldview_damage_registrations.size()){
   const auto&r=p.worldview_damage_registrations[index];
   if(r.created<p.worldview_created_registrations.size()){
    const auto&e=p.worldview_created_registrations[r.created];
    owned=!r.retired&&!e.retired&&r.epoch==epoch&&r.uncertainty==uncertainty&&
     r.function>0&&r.update_health>0&&r.thread==thread&&r.registered==registered&&r.closure==closure&&
     e.epoch==epoch&&e.uncertainty==uncertainty&&e.child==child&&e.compiled==compiled&&e.graph_slot==graph_slot&&
     e.environment==environment&&e.registered==registered&&e.root==root&&e.environment_identity==environment_identity&&
     e.source.get()==source_get&&same(e.source,source)&&same(e.graph,graph)&&p.worldview_damage_dispatching;
   }
  }
  if(p.worldview_loaded_child_call!=this||p.worldview_operation!=loan||native->serial!=serial||native->child!=child||
   !p.worldview_callback_dispatching||p.calling!=calling||p.failure_epoch!=epoch||p.failed.writes_!=writes||
   p.failed.exhausted_||p.execution_uncertainty!=uncertainty||!owned||!registered||!root||!closure||
   environment==LUA_NOREF||thread==LUA_NOREF||source.expired()||graph.expired()||
   child>=p.child_contexts.size()||p.child_contexts[child].retired||!p.child_contexts[child].loaded||
   p.child_contexts[child].loading||!p.executed_contexts.contains(p.child_contexts[child].name)||
   compiled>=p.graph_compiled_sources.size()||p.graph_compiled_sources[compiled].root!=root||
   p.graph_compiled_sources[compiled].retained==LUA_NOREF||
   p.graph_compiled_sources[compiled].context!=p.child_contexts[child].name||
   p.child_contexts[child].execution_source!=compiled||p.child_contexts[child].environment!=environment||
   p.child_contexts[child].source.get()!=source_get||!same(source,p.child_contexts[child].source)||
   graph_slot>=p.control_graphs.size()||p.control_graphs[graph_slot].graph.get()!=graph_get||
   !same(graph,p.control_graphs[graph_slot].graph)||!p.worldview_native_child_controls_current(child)){
   fail();throw ScriptError("WorldView loaded call SAME actual callback/child/VM/source changed");}
  native->pure();
 }
 void current()const{
  pure();if(checking){fail();throw ScriptError("WorldView loaded call source callback reentry");}
  checking=true;struct End{bool&v;~End(){v=false;}}end{checking};
  try{
   const auto frame=p.worldview_callback_frame();pure();
   if(kind==Kind::control)p.worldview_callback_owner(index,frame);
   else if(kind==Kind::context)p.worldview_context_callback_owner(index,frame,true);
   else if(kind==Kind::showhide)p.worldview_showhide_owner(index,frame,true);
   else if(kind==Kind::created)p.worldview_created_owner(index,frame);
   else p.worldview_damage_owner(index,frame);
   pure();
  }catch(...){fail();throw;}
 }
 void accept(bool genuine_invoked,bool genuine_returned){
  current();if(!genuine_invoked||!genuine_returned||returned){fail();throw ScriptError("WorldView loaded callback did not genuinely return");}
  returned=true;pure();
 }
 ~WorldViewLoadedChildCallScope(){
  if(creator!=std::this_thread::get_id())std::terminate();
  if(p.worldview_loaded_child_call!=this||!returned)fail();
  if(p.worldview_loaded_child_call==this)p.worldview_loaded_child_call=nullptr;
 }
 WorldViewLoadedChildCallScope(const WorldViewLoadedChildCallScope&)=delete;
 WorldViewLoadedChildCallScope&operator=(const WorldViewLoadedChildCallScope&)=delete;
};
