// Inside actual Host::Impl, AFTER the returned postread tail owner.
// C++ owning storage and lexical frames remain OUTSIDE lua_cpcall.
struct NativeDirectLuaTailControl final {
 const std::thread::id creator=std::this_thread::get_id();
 const std::shared_ptr<NativePostreadLoadTailControl> tail;
 const std::weak_ptr<const ui::detail::NotificationContextDirectLuaTailInput> input;
 const std::weak_ptr<const void> core;
 const std::uint64_t serial;
 ContextLoaderTemporaries* resources=nullptr;
 std::size_t action=std::numeric_limits<std::size_t>::max(),attempt=graph_no_owner;
 bool compile_started=false,compiled=false,lua_returned=false,gc_returned=false;
 bool showhide_started=false,showhide_returned=false;
 bool cleanup_started=false,cleanup_returned=false;
 bool environment_unref_started=false,environment_unref_returned=false;
 int entry_top=0,return_top=0,status=-1;
 NativeDirectLuaTailControl(std::shared_ptr<NativePostreadLoadTailControl> r,
  std::shared_ptr<const ui::detail::NotificationContextDirectLuaTailInput> i,
  std::weak_ptr<const void> c,std::uint64_t s):tail(std::move(r)),input(std::move(i)),core(std::move(c)),serial(s){}
};
std::deque<std::unique_ptr<NativeDirectLuaTailControl>> native_direct_lua_tail_controls;
using NativeDirectField524Dispatcher=void(*)(lua_State*,bool);
NativeDirectField524Dispatcher native_direct_field524_dispatcher=nullptr;
NativeDirectLuaTailControl* native_direct_lua_tail_active=nullptr;
void native_direct_root_cdata_first_owned(const NativePostreadRootCDataFrame&f)const{
 if(!f.direct_input)throw ScriptError("Direct returned RootCData scope lacks typed input");
 f.direct_input->require_current_without_callbacks(f.operation);
 if(postread_root_cdata_frame!=&f||!postread_load_tail_frame||
  postread_load_tail_frame->direct_input!=f.direct_input||
  postread_load_tail_frame->control.cdata.get()!=&f.control||f.control.closed||
  f.control.creator!=std::this_thread::get_id()||!f.held_root||f.control.root.lock()!=f.held_root||
  f.held_root.get()!=f.control.cdata||f.control.stage!=NativePostreadRootCDataControl::Stage::returned||
  !f.control.type||f.control.type!=native_context_cdata_creator||f.control.type->host!=this||
  !f.control.type->alive.load()||f.control.type->creator!=f.control.creator||
  std::strcmp(f.control.type->type_name,"LuaContext")||
  f.control.type->push_metatype!=&Impl::native_child_context_push_existing_metatable)
  throw ScriptError("Direct postLua genuine returned RootCData frame/creator changed FIRST");
 postread_load_tail_frame->first();
}
void native_direct_root_cdata_full_owned(const NativePostreadRootCDataFrame&f)const{
 native_direct_root_cdata_first_owned(f);postread_load_tail_frame->full();native_direct_root_cdata_first_owned(f);
}

static void native_context_direct_lua_full_owned(lua_State*L){
 native_postread_load_tail_full_owned(L);auto&p=owner(L);
 auto*c=p.native_direct_lua_tail_active;auto*f=p.postread_load_tail_frame;
 if(!c||!f||!f->direct_input||!c->tail||&f->control!=c->tail.get()||
  c->creator!=std::this_thread::get_id()||c->serial!=f->operation.operation_serial()||
  c->input.lock()!=f->direct_input||c->core.expired()||!f->actual_job||
  f->actual_job->context_direct_lua_tail!=c||f->actual_job->context_postread_load_tail!=c->tail.get()||
  f->actual_job->owner!=&p||f->actual_job->worldview_child!=c->tail->globals->child||
  !p.calling||!p.native_master_protected_boundary||
  !std::any_of(p.native_direct_lua_tail_controls.begin(),p.native_direct_lua_tail_controls.end(),
    [c](const auto&owned){return owned.get()==c;}))
  return (void)luaL_error(L,"Direct initialLua actual protected compiler/input/Job keeper absent");
 native_postread_load_tail_full_owned(L);
}
static const ui::detail::NotificationContextDirectLuaTailInput*
native_context_direct_lua_input_current_owned(lua_State*L){
 native_context_direct_lua_full_owned(L);auto&p=owner(L);
 if(L!=p.native_direct_lua_tail_active->tail->globals->thread)
  luaL_error(L,"Direct Lua input is not the genuine retained518 executing coroutine");
 return p.postread_load_tail_frame->direct_input.get();
}
static bool native_context_direct_lua_physical_owned(lua_State*L,NativeDirectLuaTailControl&c,bool refs){
 auto&p=owner(L);auto&r=*c.tail;const auto child=r.globals->child;
 if(!native_postread_globals_physical(L,*r.globals,false)||
  !native_postread_root_cdata_physical(L,*r.cdata)||child>=p.child_contexts.size())return false;
 const int top=lua_gettop(L);const auto&entry=p.child_contexts[child];
 if(entry.receiver>=0){lua_rawgeti(L,LUA_REGISTRYINDEX,entry.receiver);
  const bool same=lua_istable(L,-1)&&lua_topointer(L,-1)==r.cdata->table;lua_pop(L,1);if(!same)return false;}
 else if(c.compiled)return false;
 if(refs&&!native_postread_load_tail_contextptr_physical_owned(L,r))return false;
 return lua_gettop(L)==top;
}
static int native_context_direct_lua_coroutine_body_owned(lua_State*L){
 auto*job=static_cast<Job*>(lua_touserdata(L,1));auto&p=owner(L);
 auto*c=job?job->context_direct_lua_tail:nullptr;
 if(!job||job->owner!=&p||!c||!c->tail||L!=c->tail->globals->thread||lua_gettop(L)!=1||
  !p.postread_load_tail_frame||p.postread_load_tail_frame->actual_job!=job)
  return luaL_error(L,"Direct initialLua SAME actual child protected operand changed");
 if(!lua_checkstack(L,32))return luaL_error(L,"Direct initialLua genuine coroutine stack allocation failed");
 native_context_direct_lua_full_owned(L);auto&r=*c->tail;auto&child=p.child_contexts[r.globals->child];
 if(job->operation==Operation::native_context_direct_lua_cleanup){
  if(!c->lua_returned||!c->gc_returned||!c->cleanup_started||c->cleanup_returned||!c->resources||
   !child.loaded||child.loading||c->resources->paths[0]||c->resources->journal_size!=1||
   !native_context_direct_lua_physical_owned(L,*c,true))
   return luaL_error(L,"Direct normal cleanup lacks actual Lua/GC/restoration/resources return");
  // Original4577AC8F8820: ContextPtr local38, then4577B5 Globals1C.
  native_postread_load_tail_unref_8f8820_owned(L,r.contextptr38);
  c->resources->record(ContextLoaderTemporaries::Step::receiver_unref);
  native_context_direct_lua_full_owned(L);
  if(r.globals->globals_state!=L||r.globals->globals_reference<0||
   r.globals->globals_unref_started||c->environment_unref_started)
   return luaL_error(L,"Direct normal Globals1C original ref is uncertain or already retired");
  c->environment_unref_started=true;r.globals->globals_unref_started=true;
  luaL_unref(L,LUA_REGISTRYINDEX,r.globals->globals_reference);
  c->environment_unref_returned=true;r.globals->globals_unref_returned=true;
  r.globals->globals_state=nullptr;r.globals->globals_reference=LUA_NOREF;
  r.globals->phase=NativePostreadGlobalsControl::Phase::retired;
  c->resources->record(ContextLoaderTemporaries::Step::environment_unref);
  native_context_direct_lua_full_owned(L);
  if(!native_context_direct_lua_physical_owned(L,*c,false))
   return luaL_error(L,"Direct normal ref cleanup changed permanent actual518/env/CData owners");
  return 0;
 }
 if(!native_context_direct_lua_physical_owned(L,*c,true))
  return luaL_error(L,"Direct initialLua actual root Context/Globals/518 referents changed");
 if(job->operation==Operation::native_context_direct_lua_showhide){
  if(!child.loaded||child.loading||!c->gc_returned||!c->showhide_started||c->showhide_returned)
   return luaL_error(L,"Direct +524 invocation requires real Lua/GC/cursor and fresh suffix");
  // The real setter publishes this dispatcher only with its genuine field.
  // Its absence is a real invocation frontier, never IsFunction=false.
  if(!p.native_direct_field524_dispatcher)
   return luaL_error(L,"Direct initial showhide actual native field524 dispatcher unavailable");
  p.native_direct_field524_dispatcher(L,job->input_result);
  native_context_direct_lua_full_owned(L);c->showhide_returned=true;return 0;
 }
 if(job->operation!=Operation::native_context_direct_lua_execute||!c->compile_started||c->compiled||
  child.loaded||!child.loading||child.retired||!c->resources||!c->resources->filename||
  !c->resources->paths[0]||!c->resources->paths[0]->buffer||child.execution_source!=graph_no_owner)
  return luaL_error(L,"Direct4AF490 actual source compile occurrence absent or replayed");
 lua_rawgeti(L,LUA_REGISTRYINDEX,child.environment);const int environment=lua_gettop(L);
 native_context_print_delivery_check(L,r.globals->child,environment);
 native_context_include_delivery_full(L,r.globals->child);
 native_include_child_physical_query(L,environment,r.globals->child);
 native_context_direct_lua_full_owned(L);
 // Retain the SAME native Context table for existing callback borrowers.
 // No install_control_graph/create/install_child_context replacement runs.
 if(child.receiver!=LUA_NOREF)return luaL_error(L,"Direct compiler native Context receiver retention replay");
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.cdata->table_ref);child.receiver=luaL_ref(L,LUA_REGISTRYINDEX);
 if(p.child_binding_key==LUA_NOREF){lua_pushliteral(L,"ContextPtr");p.child_binding_key=luaL_ref(L,LUA_REGISTRYINDEX);}
 lua_pop(L,1);native_context_direct_lua_full_owned(L);
 const auto&bytes=child.bytes;
 const char*source=bytes.empty()?"":reinterpret_cast<const char*>(bytes.data());std::size_t length=bytes.size();
 if(length>=3&&std::memcmp(source,"\xef\xbb\xbf",3)==0){source+=3;length-=3;}
 if(length&&source[0]=='\x1b')
  return luaL_error(L,"Direct selected source bytecode is outside the text compiler policy");
 const int compile_status=luaL_loadbuffer(L,source,length,child.chunk.c_str());
 if(compile_status)return lua_error(L);
 native_context_direct_lua_full_owned(L);
 p.graph_execution_scope=p.graph_retain_compiled(L,child.name);child.execution_source=p.graph_execution_scope;
 c->compiled=true;
 if(child.execution_source==graph_no_owner||child.execution_source>=p.graph_compiled_sources.size())
  return luaL_error(L,"Direct compiler did not retain the actual Lua Proto");
 lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,child.environment);
 const bool same_environment=lua_rawequal(L,-1,-2);lua_pop(L,2);
 if(!same_environment)return luaL_error(L,"Direct compiled function is not in the same genuine518 environment");
 native_context_direct_lua_full_owned(L);
 lua_call(L,0,0); // the outer child cpcall protects allocation/error/actual script invocation.
 native_context_direct_lua_full_owned(L);
 if(lua_gettop(L)!=1)return luaL_error(L,"Direct initial Lua did not return its real offered stack");
 c->lua_returned=true;return 0;
}
static int native_context_direct_lua_coroutine_owned(lua_State*L){
 auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
 try{return native_context_direct_lua_coroutine_body_owned(L);}
 catch(...){p.source_exception=std::current_exception();p.source_error="Direct initialLua actual C++ supplier exception";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}return 0;
}
static void perform_native_context_direct_lua_owned(lua_State*L,Job&job){
 auto&p=owner(L);auto*c=job.context_direct_lua_tail;
 native_context_direct_lua_full_owned(L);
 if(L!=p.state||!c||!c->tail||!native_postread_globals_retained_thread_without_callbacks(p,*c->tail->globals))
  return (void)luaL_error(L,"Direct compiler requires genuine actual master/retained518");
 lua_State*const thread=c->tail->globals->thread;
 if(lua_status(thread)||lua_gettop(thread))return (void)luaL_error(L,"Direct compiler target518 is not idle");
 if(job.operation==Operation::native_context_direct_lua_execute){
  if(!p.end_turn_child_loader||p.end_turn_child_loader->entry!=L||
   p.end_turn_child_loader->child!=job.worldview_child||
   p.end_turn_child_loader->attempt!=p.child_attempt_succeeded.size())
   return (void)luaL_error(L,"Direct initialLua canonical native loader lender absent");
  c->attempt=p.child_attempt_succeeded.size();p.child_attempt_succeeded.push_back(false);
  if(!p.end_turn_child_loader->current())return (void)luaL_error(L,"Direct initialLua actual native loader origin changed");
  p.failed.set_loader_sentinel();p.ready=false;p.executed_contexts.emplace(p.child_contexts[job.worldview_child].name);
 }
 c->entry_top=lua_gettop(thread);
 if(p.budget.instructions)lua_sethook(thread,hook,LUA_MASKCOUNT,1000);
 c->status=lua_cpcall(thread,native_context_direct_lua_coroutine_owned,&job);
 lua_sethook(thread,nullptr,0,0);
 if(c->status){if(!lua_checkstack(L,1))return (void)luaL_error(L,"Direct error transfer stack allocation failed");
  lua_xmove(thread,L,1);lua_settop(thread,c->entry_top);return (void)lua_error(L);}
 c->return_top=lua_gettop(thread);
 if(c->entry_top||c->return_top)return (void)luaL_error(L,"Direct original child protected return changed stack");
 if(job.operation==Operation::native_context_direct_lua_execute){
  if(!c->lua_returned||!c->compiled)return (void)luaL_error(L,"Direct original source invocation did not return");
  for(std::size_t i=c->attempt+1;i<p.child_attempt_succeeded.size();++i)
   if(!p.child_attempt_succeeded[i])return (void)luaL_error(L,"Direct original source nested load failed");
  auto&child=p.child_contexts[job.worldview_child];
  child.loading=false;child.loaded=true;p.child_attempt_succeeded[c->attempt]=true;
  p.failed=p.end_turn_child_loader->prior_failed;p.ready=p.end_turn_child_loader->prior_ready;
 }
 native_context_direct_lua_full_owned(L);
}

// Outer C++ scope: Job/frame/strong input/root remain alive across actual cpcall.
struct NativeDirectLuaTailCall final {
 Impl&host;NativeDirectLuaTailControl&control;
 WorldViewNativeHostScope native;NativePostreadLoadTailFrame frame;
 NativePostreadRootCDataFrame cdata_frame;
 NativeDirectLuaTailCall(Impl&p,const LuaScriptHost&facade,const BnwWorldViewSourceOperationLoan&op,
  NativeDirectLuaTailControl&c,std::shared_ptr<ui::detail::NotificationLayoutContextOrdinaryState>root,
  std::shared_ptr<const ui::detail::NotificationContextDirectLuaTailInput>input)
  :host(p),control(c),native(p,facade,op),frame(p,facade,op,*c.tail,std::move(root),std::move(input)),
   cdata_frame(p,facade,op,*c.tail->cdata,frame.held_root,frame.direct_input){
  if(p.native_direct_lua_tail_active)throw ScriptError("Direct initialLua private scope reentered");
  native.child=c.tail->globals->child;native.child_source=c.tail->globals->source;
  p.native_direct_lua_tail_active=&c;
  try{frame.full();}catch(...){p.native_direct_lua_tail_active=nullptr;throw;}
 }
 ~NativeDirectLuaTailCall(){if(host.native_direct_lua_tail_active==&control)host.native_direct_lua_tail_active=nullptr;
  else host.record_failure();}
 void run(Operation op,bool hidden=false){
  frame.full();const auto&child=host.child_contexts[control.tail->globals->child];
  Job job{&host,op,child.name,{},{},{},{},{}};job.worldview_child=control.tail->globals->child;
  job.context_postread_load_tail=control.tail.get();job.context_direct_lua_tail=&control;job.input_result=hidden;
  frame.actual_job=&job;
  try{host.call(job);frame.full();
   if(op==Operation::native_context_initial_state_query&&
    (job.context_state_thread!=control.tail->globals->thread||job.context_state_environment!=control.tail->globals->environment))
    throw ScriptError("Direct Lua/GC query returned another actual518/environment");
  }catch(...){frame.actual_job=nullptr;throw;}
  frame.actual_job=nullptr;
 }
 void accept(){frame.full();native.returned=true;}
};

NativeDirectLuaTailControl& native_direct_lua_select_outer_owned(
 const ui::detail::NotificationContextPostreadRootCDataReturn*view,
 const std::shared_ptr<NativePostreadRootCDataControl>&actual,
 const std::shared_ptr<const ui::detail::NotificationContextDirectLuaTailInput>&input,
 const BnwWorldViewSourceOperationLoan&op,bool create){
 if(!view||!actual||!input||calling||worldview_native_host||postread_load_tail_frame||native_direct_lua_tail_active)
  throw ScriptError("Direct Lua outer facade requires actual idle typed input and CData");
 input->require_current_owned(op);input->require_root_cdata_return_owned(op,*view);
 std::shared_ptr<NativePostreadLoadTailControl> tail;
 for(const auto&owned:postread_load_tail_controls)if(owned&&owned->cdata==actual){
  if(tail)throw ScriptError("Direct Lua canonical postread occurrence duplicated");tail=owned;}
 if(!tail||tail->root_return.get()!=view||tail->closed||tail->creator!=std::this_thread::get_id()||
  tail->stage!=NativePostreadLoadTailControl::Stage::registrations_returned||
  tail->entry_top||tail->returned_top||tail->protected_status||!actual->root.lock())
  throw ScriptError("Direct Lua requires actual returned ordered Context/Lookup/UIManager/TTManager");
 for(const auto&owned:native_direct_lua_tail_controls)if(owned&&owned->tail==tail){
  if(create||owned->input.lock()!=input||owned->serial!=op.operation_serial())
   throw ScriptError("Direct original Lua input occurrence reused/replayed");
  input->require_current_owned(op);return *owned;}
 if(!create)throw ScriptError("Direct original Lua compiler occurrence has never executed");
 auto owned=std::make_unique<NativeDirectLuaTailControl>(tail,input,input->actual_core_identity_owned(op),op.operation_serial());
 input->require_current_owned(op);auto*const result=owned.get();native_direct_lua_tail_controls.push_back(std::move(owned));
 input->require_current_owned(op);return *result;
}

void native_direct_lua_prepare_resources_owned(NativeDirectLuaTailCall&scope){
 auto&c=scope.control;auto&r=*c.tail;const auto&op=scope.frame.operation;auto&child=child_contexts[r.globals->child];
 scope.frame.full();if(child.loader_temporaries||child.loaded||child.loading||child.retired||
  executed_contexts.contains(child.name))throw ScriptError("Direct selected original resource/compile occurrence replay");
 const auto operands=op.context_loader_resource_operand_owned(r.globals->source);scope.frame.full();
 if(!operands.bundle_||!operands.generation_||operands.serial_!=c.serial||operands.source_!=r.globals->source||operands.context_!=child.name)
  throw ScriptError("Direct compiler resource operand is not SAME canonical winner");
 child.loader_temporaries=std::make_unique<ContextLoaderTemporaries>(c.core,r.globals->prepared,r.globals->source,
  operands.bundle_,c.serial,operands.generation_,failure_epoch,failed.writes_,execution_uncertainty,
  child.name,operands.lua_path_,operands.xml_path_,operands.stem_);
 c.resources=child.loader_temporaries.get();auto&temp=*c.resources;
 const auto current=[&]{scope.frame.full();operands.bundle_->require_current();scope.frame.full();};
 const auto resolve=[&](ContextLoaderPathTemporary::Role role,const std::string&path,const std::string&expected){
  current();assets::AssetKey key(path);if(!key.safe_relative_path())throw ScriptError("Direct loader path outside canonical VFS");
  auto mounted=operands.bundle_->resources().resolve(key);current();auto*asset=std::get_if<assets::AssetHandle>(&mounted.asset);
  if(!asset)throw ScriptError("Direct loader actual selected VFS resource missing/ambiguous");
  auto handle=std::make_unique<assets::AssetHandle>(std::move(*asset));
  auto bytes=std::make_unique<assets::Bytes>(handle->read_bytes(child_source_limit));current();
  const auto identity=identify_bytes(*bytes);const auto bank=operands.bundle_->loaded(path);current();
  if(!bank.bytes||identity.sha256!=expected||identity.sha256!=bank.payload_identity.sha256||
   identity.bytes!=bank.payload_identity.bytes||identify_bytes(*bank.bytes).sha256!=identity.sha256||
   handle->candidate().normalized_name!=bank.candidate.normalized_name||
   handle->candidate().payload_size!=bank.candidate.payload_size||
   handle->candidate().provenance.kind!=bank.candidate.provenance.kind||
   handle->candidate().provenance.source_id!=bank.candidate.provenance.source_id||
   handle->candidate().provenance.source_relative_path!=bank.candidate.provenance.source_relative_path||
   handle->candidate().provenance.archive_relative_path!=bank.candidate.provenance.archive_relative_path)
   throw ScriptError("Direct fresh compiler/resource lifetime differs from canonical Bank winner");
  return std::make_unique<ContextLoaderPathTemporary>(role,path,identity,std::move(handle),std::move(bytes));};
 temp.paths[0]=resolve(ContextLoaderPathTemporary::Role::script_compile_path,operands.lua_path_,operands.lua_sha_);
 temp.paths[1]=resolve(ContextLoaderPathTemporary::Role::context_layout_source_path,operands.xml_path_,operands.xml_sha_);
 temp.paths[2]=resolve(ContextLoaderPathTemporary::Role::script_source_directory_path,operands.lua_path_,operands.lua_sha_);
 temp.filename=std::make_unique<std::string>(temp.paths[0]->handle->candidate().original_name);current();
 if(temp.filename->empty()||temp.filename->find('\0')!=std::string::npos||temp.paths[2]->source_directory.empty()||
  assets::normalize_asset_name(temp.paths[2]->source_directory)!=
  assets::normalize_asset_name(assets::path_utf8(std::filesystem::path(operands.stem_).parent_path())))
  throw ScriptError("Direct compiler actual selected filename/directory invalid");
 child.chunk="@"+*temp.filename;child.bytes=*temp.paths[0]->buffer;
 child.source->require_captured_bytes(child.bytes);current();c.compile_started=true;child.loading=true;scope.frame.full();
}
