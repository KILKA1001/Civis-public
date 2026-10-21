// Outside Impl AFTER actual Preparation, RootCData and Cleanup return definitions.
// These operations reuse the actual selected518 and its typed native globals.
namespace civ5::ui::detail {
void NotificationContextPostreadRootCDataReturn::invoke_direct_initial_lua_owned(
 const runtime::BnwWorldViewSourceOperationLoan&op,
 const std::shared_ptr<const NotificationContextDirectLuaTailInput>&input)const{
 using Impl=runtime::LuaScriptHost::Impl;const auto fields=impl_;const auto p=fields?fields->host.lock():nullptr;
 std::size_t action=std::numeric_limits<std::size_t>::max();
 try{
  if(!fields||!p||!fields->facade||!fields->alive.load()||fields->creator!=std::this_thread::get_id()||
   p->ui_thread!=fields->creator||!p->state||p->failed||!p->end_turn_lifetime||
   !p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||p->end_turn_lifetime->host!=fields->facade||
   p->end_turn_lifetime->vm!=p->state)throw runtime::ScriptError("Direct compiler owning facade retired FIRST");
  auto&c=p->native_direct_lua_select_outer_owned(this,fields->actual,input,op,true);
  const auto root=fields->actual->root.lock();
  Impl::NativeDirectLuaTailCall scope(*p,*fields->facade,op,c,root,input);
  p->native_direct_lua_prepare_resources_owned(scope);scope.frame.full();
  auto&child=p->child_contexts[c.tail->globals->child];
  action=p->execution_begin(runtime::ScriptLuaAction::context_execute,child.name,child.chunk);c.action=action;
  const char*actual_text=child.bytes.empty()?"":reinterpret_cast<const char*>(child.bytes.data());
  std::size_t actual_length=child.bytes.size();
  if(actual_length>=3&&std::memcmp(actual_text,"\xef\xbb\xbf",3)==0){actual_text+=3;actual_length-=3;}
  p->execution_source(action,{actual_text,actual_length}); // same real compiler text policy as run_child_context.
  scope.frame.full();
  {Impl::EndTurnChildLoaderScope loader(*p,p->state);loader.child=c.tail->globals->child;
   scope.run(Impl::Operation::native_context_direct_lua_execute);
   if(!c.lua_returned||!c.compiled||!child.loaded||child.loading||!p->executed_contexts.contains(child.name))
    throw runtime::ScriptError("Direct original selected Lua source did not execute and return");
   loader.returned=true;}
  p->execution_finish(action,runtime::ScriptLuaActionOutcome::succeeded);action=std::numeric_limits<std::size_t>::max();
  scope.frame.full();child.source->require_captured_bytes(child.bytes);scope.frame.full();
  scope.run(Impl::Operation::native_context_register_gc);
  if(!child.state_registered||!child.state_gc_returned)
   throw runtime::ScriptError("Direct actual4A31F0 and child lua_gc(step5,500) did not return");
  c.gc_returned=true;scope.frame.full();scope.run(Impl::Operation::native_context_initial_state_query);scope.accept();
 }catch(...){if(p){p->execution_finish(action,runtime::ScriptLuaActionOutcome::error);p->record_failure();
   if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;}throw;}
}
void NotificationContextPostreadRootCDataReturn::require_direct_initial_lua_state_owned(
 const runtime::BnwWorldViewSourceOperationLoan&op,
 const std::shared_ptr<const NotificationContextDirectLuaTailInput>&input)const{
 using Impl=runtime::LuaScriptHost::Impl;const auto fields=impl_;const auto p=fields?fields->host.lock():nullptr;
 try{
  if(!fields||!p||!fields->facade||!fields->alive.load()||fields->creator!=std::this_thread::get_id()||
   p->ui_thread!=fields->creator||!p->state||p->failed||!p->end_turn_lifetime||
   !p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||p->end_turn_lifetime->vm!=p->state)
   throw runtime::ScriptError("Direct Lua/GC query owner retired FIRST");
  auto&c=p->native_direct_lua_select_outer_owned(this,fields->actual,input,op,false);
  if(!c.lua_returned||!c.gc_returned||c.cleanup_returned)
   throw runtime::ScriptError("Direct completed Lua/GC state absent or normal cleanup already returned");
  Impl::NativeDirectLuaTailCall scope(*p,*fields->facade,op,c,fields->actual->root.lock(),input);
  scope.run(Impl::Operation::native_context_initial_state_query);scope.accept();
 }catch(...){if(p){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;}throw;}
}
void NotificationContextPostreadRootCDataReturn::invoke_direct_initial_showhide_owned(
 const runtime::BnwWorldViewSourceOperationLoan&op,
 const std::shared_ptr<const NotificationContextDirectLuaTailInput>&input,
 const std::shared_ptr<const NotificationContextCursorRestoreReturn>&restored,bool hidden)const{
 using Impl=runtime::LuaScriptHost::Impl;const auto fields=impl_;const auto p=fields?fields->host.lock():nullptr;
 try{
  if(!fields||!p||!fields->facade||!fields->alive.load()||fields->creator!=std::this_thread::get_id()||
   p->ui_thread!=fields->creator||!p->state||p->failed||!restored||!p->end_turn_lifetime||
   !p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||p->end_turn_lifetime->vm!=p->state)
   throw runtime::ScriptError("Direct initial showhide owning facade/restored cursor retired FIRST");
  auto&c=p->native_direct_lua_select_outer_owned(this,fields->actual,input,op,false);
  if(!c.lua_returned||!c.gc_returned||c.cleanup_started||c.showhide_started)
   throw runtime::ScriptError("Direct actual +524 suffix replay or pre-GC invocation");
  Impl::NativeDirectLuaTailCall scope(*p,*fields->facade,op,c,fields->actual->root.lock(),input);
  c.showhide_started=true;scope.run(Impl::Operation::native_context_direct_lua_showhide,hidden);
  if(!c.showhide_returned)throw runtime::ScriptError("Direct actual +524 field callback did not return");
  scope.run(Impl::Operation::native_context_initial_state_query);scope.accept();
 }catch(...){if(p){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;}throw;}
}
std::shared_ptr<const NotificationContextLoaderCleanupReturn>
NotificationContextPostreadRootCDataReturn::cleanup_direct_loader_temporaries_owned(
 const runtime::BnwWorldViewSourceOperationLoan&op,
 const std::shared_ptr<const NotificationContextDirectLuaTailInput>&input,
 const std::shared_ptr<const NotificationContextCursorRestoreReturn>&restored)const{
 using Impl=runtime::LuaScriptHost::Impl;using Return=NotificationContextLoaderCleanupReturn;
 const auto fields=impl_;const auto p=fields?fields->host.lock():nullptr;
 try{
  if(!fields||!p||!fields->facade||!fields->alive.load()||fields->creator!=std::this_thread::get_id()||
   p->ui_thread!=fields->creator||!p->state||p->failed||!restored||!p->end_turn_lifetime||
   !p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||p->end_turn_lifetime->vm!=p->state)
   throw runtime::ScriptError("Direct normal cleanup owning facade/restored cursor retired FIRST");
  auto&c=p->native_direct_lua_select_outer_owned(this,fields->actual,input,op,false);
  if(!c.lua_returned||!c.gc_returned||!c.resources||c.cleanup_started||c.cleanup_returned||
   (c.showhide_started&&!c.showhide_returned))throw runtime::ScriptError("Direct normal suffix incomplete/replayed");
  Impl::NativeDirectLuaTailCall scope(*p,*fields->facade,op,c,fields->actual->root.lock(),input);
  auto&temp=*c.resources;scope.frame.full();
  if(temp.creator!=std::this_thread::get_id()||temp.core.expired()||temp.serial!=c.serial||temp.cleanup_started||
   temp.cleanup_returned||temp.journal_size||temp.environment!=LUA_NOREF||temp.receiver!=LUA_NOREF||!temp.filename)
   throw runtime::ScriptError("Direct normal actual selected storage owner preimage changed");
  for(const auto&path:temp.paths)if(!path||!path->handle||!path->buffer||
   runtime::identify_bytes(*path->buffer).sha256!=path->fingerprint.sha256||
   runtime::identify_bytes(*path->buffer).bytes!=path->fingerprint.bytes)
   throw runtime::ScriptError("Direct normal actual path storage changed before release");
  c.cleanup_started=true;temp.cleanup_started=true;
  // Actual normal role order45779C..4577FC. The x64 owners release real
  // storage; they do not claim original FString allocator machine DTORs.
  temp.paths[0].reset();temp.record(Impl::ContextLoaderTemporaries::Step::script_path_destroyed);scope.frame.full();
  scope.run(Impl::Operation::native_context_direct_lua_cleanup);scope.frame.full();
  temp.paths[1].reset();temp.record(Impl::ContextLoaderTemporaries::Step::layout_path_destroyed);scope.frame.full();
  temp.paths[2].reset();temp.record(Impl::ContextLoaderTemporaries::Step::directory_path_destroyed);scope.frame.full();
  temp.filename.reset();temp.record(Impl::ContextLoaderTemporaries::Step::filename_destroyed);scope.frame.full();
  constexpr std::array<Impl::ContextLoaderTemporaries::Step,6> order{
   Impl::ContextLoaderTemporaries::Step::script_path_destroyed,Impl::ContextLoaderTemporaries::Step::receiver_unref,
   Impl::ContextLoaderTemporaries::Step::environment_unref,Impl::ContextLoaderTemporaries::Step::layout_path_destroyed,
   Impl::ContextLoaderTemporaries::Step::directory_path_destroyed,Impl::ContextLoaderTemporaries::Step::filename_destroyed};
  const auto&r=*c.tail;
  if(temp.journal_size!=order.size()||temp.journal!=order||temp.filename||temp.paths[0]||temp.paths[1]||temp.paths[2]||
   r.contextptr38.state||r.contextptr38.reference!=LUA_NOREF||!r.contextptr38.unref_started||!r.contextptr38.unref_returned||
   r.globals->globals_state||r.globals->globals_reference!=LUA_NOREF||!r.globals->globals_unref_started||
   !r.globals->globals_unref_returned||!c.environment_unref_started||!c.environment_unref_returned||
   r.globals->phase!=Impl::NativePostreadGlobalsControl::Phase::retired)
   throw runtime::ScriptError("Direct normal actual storage/38/1C destruction history incomplete");
  scope.run(Impl::Operation::native_context_initial_state_query);
  const auto body=std::make_shared<const Return::Impl>(p,c.core,r.globals->prepared,restored,&temp,c.serial,temp.generation,
   p->failure_epoch,r.globals->writes,p->execution_uncertainty);
  const auto returned=std::shared_ptr<const Return>(new Return(body));scope.frame.full();
  temp.cleanup_returned=true;c.cleanup_returned=true;scope.accept();return returned;
 }catch(...){if(p){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;}throw;}
}
}
