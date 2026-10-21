// Outside actual Host::Impl AFTER entered_controls_api and BEFORE setup API.
#include "civ5/ui/detail/notification_context_lua_controls_named_retirement.hpp"
namespace civ5::ui::detail {
struct NotificationContextLuaControlsNamedRetirement::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> alive{true};
 const std::weak_ptr<runtime::LuaScriptHost::Impl> host;
 const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
 const runtime::LuaScriptHost*const facade;
 const std::shared_ptr<const NotificationContextLuaStatePreparation> prepared;
 const std::shared_ptr<const NotificationContextRootDefaultFieldsReturn> defaults;
 const std::shared_ptr<const NotificationContextLuaControlsNamedReference> named;
 const std::shared_ptr<const runtime::ScriptUiChildSource> descriptor;
 const std::shared_ptr<const runtime::LuaScriptHost::Impl::NativeControlsNamedRetireFrame::AppliedUnref> actual_unref;
 lua_State*const original_state;
 const std::uint64_t scope,serial,epoch,writes,uncertainty;
 mutable std::weak_ptr<const NotificationContextLuaControlsNamedRetirement> canonical;
 Impl(std::weak_ptr<runtime::LuaScriptHost::Impl> owner,std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> s,
  const runtime::LuaScriptHost*f,std::shared_ptr<const NotificationContextLuaStatePreparation> prep,
  std::shared_ptr<const NotificationContextRootDefaultFieldsReturn> root,
  std::shared_ptr<const NotificationContextLuaControlsNamedReference> actual,
  std::shared_ptr<const runtime::ScriptUiChildSource> selected,
  std::shared_ptr<const runtime::LuaScriptHost::Impl::NativeControlsNamedRetireFrame::AppliedUnref> unref,lua_State*L,
  std::uint64_t active_scope,std::uint64_t q,std::uint64_t e,std::uint64_t w,std::uint64_t u)
  :host(std::move(owner)),source(std::move(s)),facade(f),prepared(std::move(prep)),defaults(std::move(root)),named(std::move(actual)),descriptor(std::move(selected)),
   actual_unref(std::move(unref)),original_state(L),scope(active_scope),serial(q),epoch(e),writes(w),uncertainty(u){}
};
NotificationContextLuaControlsNamedRetirement::NotificationContextLuaControlsNamedRetirement(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
NotificationContextLuaControlsNamedRetirement::~NotificationContextLuaControlsNamedRetirement(){impl_->alive.store(false);}
void NotificationContextLuaControlsNamedRetirement::require_actual_nil_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan&op,const NotificationContextLuaControlsNamedReference&named)const{
 const auto r=impl_;if(!r||!r->facade)throw runtime::ScriptError("actual local LuaRef nil return absent");
 //Retained payload is not a facade lifetime grant. Hold the actual weak Host
 //and Source strongly only inside this callback-free check, then prove the
 //same UI-thread tombstone before any raw facade member dispatch.
 const auto p=r->host.lock();const auto source=r->source.lock();const auto canonical=r->canonical.lock();
 if(!p||!source||!canonical||canonical.get()!=this||canonical->impl_!=r||
  r->creator!=std::this_thread::get_id()||!r->alive.load()||p->worldview_sources.lock()!=source)
  throw runtime::ScriptError("actual local LuaRef nil return canonical Host/Source retired FIRST");
 source->require_operation_origin_without_callbacks(op,r->scope); // FIRST before historical raw op/facade
 if(p->ui_thread!=r->creator||!p->state||p->failed||!p->end_turn_lifetime||
  !p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||
  p->end_turn_lifetime->host!=r->facade||p->end_turn_lifetime->vm!=p->state)
  throw runtime::ScriptError("actual local LuaRef nil return VM facade tombstone retired");
 //No callback/allocation can retire the facade between tombstone and call.
 r->facade->require_context_controls_named_retirement_current_without_callbacks(op,named,*this);
}
void NotificationContextLuaControlsNamedRetirement::require_actual_nil_owned(
 const runtime::BnwWorldViewSourceOperationLoan&op,const NotificationContextLuaControlsNamedReference&named)const{
 const auto r=impl_;const auto host=r?r->host.lock():nullptr;const auto source=r?r->source.lock():nullptr;
 if(!host||!source)throw runtime::ScriptError("actual local LuaRef nil return owner retired before FULL");
 //Lexical strong custody spans each FULL; callbacks still cannot resurrect
 //a dead facade because every POST repeats the actual tombstone FIRST.
 require_actual_nil_without_callbacks(op,named);op.require_current();require_actual_nil_without_callbacks(op,named);
 op.require_same_vm(*r->facade);require_actual_nil_without_callbacks(op,named);
 op.require_same_source(*r->descriptor);require_actual_nil_without_callbacks(op,named);
}
}
namespace civ5::runtime {
void LuaScriptHost::require_context_controls_named_retirement_current_without_callbacks(
 const BnwWorldViewSourceOperationLoan&op,const ui::detail::NotificationContextLuaControlsNamedReference&actual_named,
 const ui::detail::NotificationContextLuaControlsNamedRetirement&returned)const{
 using Named=ui::detail::NotificationContextLuaControlsNamedReference;
 const auto p=impl_;const auto r=returned.impl_;if(!r)throw ScriptError("actual local LuaRef retired payload missing");
 const auto source=p->worldview_sources.lock();const auto origin=r->source.lock();
 if(!source||source!=origin)throw ScriptError("actual local LuaRef nil return canonical Source retired FIRST");
 source->require_operation_origin_without_callbacks(op,r->scope); // genuine Source FIRST before historical op
 const auto named=r->named;const auto n=named?named->impl_:nullptr;const auto canonical=r->canonical.lock();
 const auto prepared=r->prepared;const auto prep=prepared?prepared->impl_:nullptr;
 const auto setup=n?n->entered_setup:nullptr;const auto completed=setup?setup->impl_:nullptr;
 if(r->creator!=std::this_thread::get_id()||!r->alive.load()||r->facade!=this||r->host.lock()!=p||
  !named||named.get()!=&actual_named||!n||!canonical||canonical.get()!=&returned||canonical->impl_!=r||
  !p->state||p->ui_thread!=r->creator||p->failed||p->end_turn_native_active||p->controls_named_retire_frame||
  !p->end_turn_lifetime||!p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||p->end_turn_lifetime->host!=this||
  p->end_turn_lifetime->vm!=p->state||r->original_state!=p->state||r->serial!=op.operation_serial()||
  r->epoch!=p->failure_epoch||r->writes!=p->failed.writes_||r->uncertainty!=p->execution_uncertainty||
  n->local_lifetime!=Named::Impl::LocalLifetime::nil_retired||n->local_state||n->local_reference!=LUA_NOREF||
  n->local_retirement.lock()!=canonical||n->entered_canonical.lock()!=named||!prep||!completed||completed->prepared!=prepared||
  n->defaults!=r->defaults||r->descriptor!=prep->source||!r->actual_unref||r->actual_unref->original_reference!=n->reference||n->reference<0||
  r->actual_unref->after_free_head!=n->reference||n->child!=prep->child||n->child>=p->child_contexts.size()||
  n->table_return!=completed->controls||!completed->controls||!completed->controls->impl_||!completed->state_name||!completed->state_name->impl_)
  throw ScriptError("actual local LuaRef nil state/reference/earned unref return/canonical Host changed");
 if(p->calling&&!p->worldview_native_host)throw ScriptError("actual local LuaRef nil return foreign Lua execution");
 const auto& current=p->child_contexts[n->child];const auto table=completed->controls->impl_;
 if(current.controls_named_entered_return.lock()!=named||current.retired||!current.state_prepared||
  current.environment!=prep->environment||current.execution_thread!=prep->thread||current.source!=prep->source||
  current.controls_table_pointer!=n->table||current.controls_table_ref!=table->reference||
  !Impl::native_entered_controls_physical_without_callbacks(*p,n->child,prep->environment,prep->thread,
   prep->actual_environment,prep->actual_thread,n->table,table->reference,completed->state_name->impl_->value))
  throw ScriptError("actual local LuaRef nil return permanent parent518/Controls table changed");
 // Later Lua allocations may reuse the freed registry slot. This checks the
 // actual nil local fields plus immutable earned unref history, not a live
 // claim that an old free-list head/slot remains unchanged forever.
}
std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedRetirement>
LuaScriptHost::retire_context_controls_named_reference_owned(const BnwWorldViewSourceOperationLoan&op,
 const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&input,
 const std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn>&default_input,
 const std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedReference>&named_input)const{
 using Return=ui::detail::NotificationContextLuaControlsNamedRetirement;
 using Named=ui::detail::NotificationContextLuaControlsNamedReference;
 const auto p=impl_;const auto prepared=input;const auto defaults=default_input;const auto named=named_input;
 try{
  if(!prepared||!defaults||!named)throw ScriptError("actual local LuaRef destructor operands absent");
  const auto root=defaults->capture_guard_without_callbacks(op,prepared);
  const auto live=named->capture_guard_without_callbacks(op,prepared,defaults);
  live.require_current_owned(op,prepared,defaults);root.require_current_owned(op,prepared);
  const auto n=named->impl_;const auto prep=prepared->impl_;const auto setup=n->entered_setup;
  const auto completed=setup->impl_;const auto table=completed->controls->impl_;
  const auto source=p->worldview_sources.lock();
  if(!source||n->local_state!=p->state||n->local_reference!=n->reference||n->reference<0||
   n->reference==table->reference||n->reference==prep->environment||n->reference==prep->thread||
   n->local_lifetime!=Named::Impl::LocalLifetime::live_entered||!n->local_retirement.expired())
   throw ScriptError("actual local LuaRef destructor is not the distinct retained Controls reference");
  const auto scope=source->capture_active_operation_scope_serial_without_callbacks(&op);
  lua_State*const original_state=p->state;
  std::shared_ptr<const Impl::NativeControlsNamedRetireFrame::AppliedUnref> applied;
  {
   Impl::WorldViewNativeHostScope native(*p,*this,op);native.child=prep->child;native.child_source=prep->source;
   Impl::NativeControlsNamedRetireFrame frame(*p,*this,op,prepared,setup,named,live,root,defaults,prep->source,prep->child,n->reference);
   frame.full();const auto context_view=prep->source->context();frame.full();const std::string context(context_view);frame.full();
   Impl::Job job{p.get(),Impl::Operation::native_context_controls_named_retire,context,{},{},{},{},{}};
   job.worldview_child=prep->child;p->call(job);frame.full();
   if(!frame.applied||frame.stage!=Impl::NativeControlsNamedRetireFrame::Stage::unref_applied)
    throw ScriptError("actual local LuaRef8F8838 did not return applied registry effects");
   applied=frame.applied;
   //8F8840/46 are actual local field stores after luaL_unref returned.
   n->local_state=nullptr;n->local_reference=LUA_NOREF;n->local_lifetime=Named::Impl::LocalLifetime::nil_retired;
   frame.full();native.returned=true;
  } // genuine protected/native/frame DTOR, no old live NamedGuard renewal
  const auto first=[&]{
   source->require_operation_origin_without_callbacks(op,scope);
   root.require_current_without_callbacks(op,prepared);require_context_lua_load_setup_current_without_callbacks(op,prepared,*setup);
   if(p->controls_named_retire_frame||p->worldview_native_host||p->calling||n->local_state||n->local_reference!=LUA_NOREF||
    n->local_lifetime!=Named::Impl::LocalLifetime::nil_retired||!n->local_retirement.expired()||
    p->state!=original_state||!Impl::native_controls_named_unref_physical_without_callbacks(*p,n->reference,applied->before_free_head))
    throw ScriptError("actual local LuaRef native/frame DTOR or final nil/raw registry POST changed");
  };
  const auto full=[&]{first();root.require_current_owned(op,prepared);first();op.require_current();first();
   op.require_same_vm(*this);first();op.require_same_source(*prep->source);first();};
  full(); // LAST FULL + actual nil/registry POST after real scope DTOR
  auto fields=std::make_shared<const Return::Impl>(p,source,this,prepared,defaults,named,prep->source,applied,original_state,scope,
   op.operation_serial(),p->failure_epoch,p->failed.writes_,p->execution_uncertainty);full();
  auto returned=std::shared_ptr<const Return>(new Return(fields));full();
  fields->canonical=returned;n->local_retirement=returned;
  require_context_controls_named_retirement_current_without_callbacks(op,*named,*returned);
  returned->require_actual_nil_owned(op,*named);return returned;
 }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
