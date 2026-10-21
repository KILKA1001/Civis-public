// Outside Impl, AFTER actual Preparation::Impl. This is a selected Civis
// resource-owner port; all owning allocations happen outside Lua C frames.
namespace civ5::ui::detail {
struct NotificationContextLoaderCleanupReturn::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<const void> host,core;
 const std::shared_ptr<const NotificationContextLuaStatePreparation> preparation;
 const std::shared_ptr<const NotificationContextCursorRestoreReturn> restore;
 const void*const temporary;
 const std::uint64_t serial,generation,epoch,writes,uncertainty;
 Impl(std::weak_ptr<const void> h,std::weak_ptr<const void> c,
  std::shared_ptr<const NotificationContextLuaStatePreparation> p,std::shared_ptr<const NotificationContextCursorRestoreReturn> r,
  const void*t,std::uint64_t op,std::uint64_t g,std::uint64_t e,std::uint64_t w,std::uint64_t u)
 :host(std::move(h)),core(std::move(c)),preparation(std::move(p)),restore(std::move(r)),temporary(t),serial(op),generation(g),epoch(e),writes(w),uncertainty(u){}
};
NotificationContextLoaderCleanupReturn::NotificationContextLoaderCleanupReturn(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
NotificationContextLoaderCleanupReturn::~NotificationContextLoaderCleanupReturn()=default;
}
namespace civ5::runtime {
std::shared_ptr<const ui::detail::NotificationContextLoaderCleanupReturn>
LuaScriptHost::cleanup_context_loader_temporaries_owned(const BnwWorldViewSourceOperationLoan& loan,
 const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared,
 const std::weak_ptr<const void>& core,
 const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>& cursor,
 const std::shared_ptr<const ui::detail::NotificationContextCursorRestoreReturn>& restored)const {
 using Return=ui::detail::NotificationContextLoaderCleanupReturn;const auto p=impl_;
 try{
  require_context_initial_lua_state_owned(loan,prepared,core,cursor);
  if(!restored)throw ScriptError("Selected Civis loader cleanup requires genuine restored cursor receipt");
  Impl::WorldViewNativeHostScope native(*p,*this,loan);p->worldview_require_callback_origin();
  const auto& saved=*prepared->impl_;native.child=saved.child;native.child_source=saved.source;
  auto& child=p->child_contexts[saved.child];
  if(!child.loader_temporaries)throw ScriptError("Selected Civis loader cleanup never allocated its temporary owners");
  auto& temp=*child.loader_temporaries;
  const auto operands=loan.context_loader_resource_operand_owned(saved.source);p->worldview_require_callback_origin();
  const auto pure=[&]{
   p->worldview_require_callback_origin();loan.require_origin_without_callbacks();
   if(temp.creator!=std::this_thread::get_id()||temp.core.expired()||core.expired()||
      temp.core.owner_before(core)||core.owner_before(temp.core)||temp.preparation.get()!=prepared.get()||
      temp.preparation.owner_before(prepared)||prepared.owner_before(temp.preparation)||
      temp.source.get()!=saved.source.get()||temp.source.owner_before(saved.source)||saved.source.owner_before(temp.source)||
      temp.bundle.get()!=operands.bundle_.get()||temp.bundle.owner_before(operands.bundle_)||operands.bundle_.owner_before(temp.bundle)||
      temp.serial!=loan.operation_serial()||temp.generation!=operands.generation_||
      temp.epoch!=p->failure_epoch||temp.writes!=p->failed.writes_||temp.uncertainty!=p->execution_uncertainty||
      child.loader_temporaries.get()!=&temp||temp.cleanup_returned)
      throw ScriptError("Selected Civis loader temporary SAME Core/Host/Source/generation lifetime changed");
  };
  pure();
  Impl::Job query{p.get(),Impl::Operation::native_context_loader_temporary_query,child.name,{},{},{},{},{}};
  query.worldview_child=saved.child;p->call(query);pure();
  if(!temp.filename||temp.filename->empty()||temp.cleanup_started||temp.journal_size||!temp.refs_returned)
      throw ScriptError("Selected Civis loader cleanup replay/partial owner");
  for(const auto& path:temp.paths)if(!path||!path->handle||!path->buffer||
      identify_bytes(*path->buffer).bytes!=path->fingerprint.bytes||identify_bytes(*path->buffer).sha256!=path->fingerprint.sha256)
      throw ScriptError("Selected Civis loader path allocation/preimage changed");
  temp.cleanup_started=true;
  // Actual role order corresponding to45779C..4577FC. Storage free has no
  // UI/gameplay callbacks: real C++ deleters are the supported port consumer.
  temp.paths[0].reset();temp.record(Impl::ContextLoaderTemporaries::Step::script_path_destroyed);pure();
  Impl::Job receiver{p.get(),Impl::Operation::native_context_loader_receiver_unref,child.name,{},{},{},{},{}};
  receiver.worldview_child=saved.child;p->call(receiver);pure();
  Impl::Job environment{p.get(),Impl::Operation::native_context_loader_environment_unref,child.name,{},{},{},{},{}};
  environment.worldview_child=saved.child;p->call(environment);pure();
  temp.paths[1].reset();temp.record(Impl::ContextLoaderTemporaries::Step::layout_path_destroyed);pure();
  temp.paths[2].reset();temp.record(Impl::ContextLoaderTemporaries::Step::directory_path_destroyed);pure();
  temp.filename.reset();temp.record(Impl::ContextLoaderTemporaries::Step::filename_destroyed);pure();
  constexpr std::array<Impl::ContextLoaderTemporaries::Step,6> order{
    Impl::ContextLoaderTemporaries::Step::script_path_destroyed,Impl::ContextLoaderTemporaries::Step::receiver_unref,
    Impl::ContextLoaderTemporaries::Step::environment_unref,Impl::ContextLoaderTemporaries::Step::layout_path_destroyed,
    Impl::ContextLoaderTemporaries::Step::directory_path_destroyed,Impl::ContextLoaderTemporaries::Step::filename_destroyed};
  if(temp.journal_size!=order.size()||temp.journal!=order||temp.receiver!=LUA_NOREF||temp.environment!=LUA_NOREF||
      temp.filename||temp.paths[0]||temp.paths[1]||temp.paths[2])throw ScriptError("Selected Civis loader actual destruction/unref history incomplete");
  Impl::Job after{p.get(),Impl::Operation::native_context_initial_state_query,child.name,{},{},{},{},{}};
  after.worldview_child=saved.child;p->call(after);pure();
  if(after.context_state_thread!=saved.actual_thread||after.context_state_environment!=saved.actual_environment)
      throw ScriptError("Selected Civis loader cleanup permanent coroutine/env changed");
  const auto body=std::make_shared<const Return::Impl>(p,core,prepared,restored,&temp,temp.serial,temp.generation,
      p->failure_epoch,p->failed.writes_,p->execution_uncertainty);
  const auto result=std::shared_ptr<const Return>(new Return(body));pure();
  temp.cleanup_returned=true;p->worldview_require_callback_origin();native.returned=true;return result;
 }catch(...){p->worldview_retire_callback_registrations();p->worldview_retire_context_callback_registrations();
   p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
