// Outside Host::Impl AFTER actual Preparation/StateName/Profile/Controls bodies.
// Closed Setup payload/guards only. Optional's real ordered registrar/setup
// issuer remains separate and currently stops before it can construct this.
namespace civ5::ui::detail {
struct NotificationContextLuaLoadSetupReturn::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const std::weak_ptr<runtime::LuaScriptHost::Impl> host;
 const runtime::LuaScriptHost*const facade;
 const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
 const std::shared_ptr<const NotificationContextLuaStatePreparation> prepared;
 const std::shared_ptr<const NotificationContextLoadNameLoan> name;
 const std::shared_ptr<const NotificationContextResizeRegistrationReturn> resize;
 const std::shared_ptr<const NotificationContextLuaStateNamePrefixReturn> state_name;
 const std::shared_ptr<const NotificationContextLuaProfileReturn> profile;
 const std::shared_ptr<const NotificationContextLuaControlsTableReturn> controls;
 const std::size_t slot;
 const std::uint64_t scope,serial,epoch,writes,uncertainty;
 std::weak_ptr<const NotificationContextLuaLoadSetupReturn> canonical;
 Impl(std::weak_ptr<runtime::LuaScriptHost::Impl> actual,const runtime::LuaScriptHost*f,
  std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> owner,
  std::shared_ptr<const NotificationContextLuaStatePreparation> installed,
  std::shared_ptr<const NotificationContextLoadNameLoan> physical_name,
  std::shared_ptr<const NotificationContextResizeRegistrationReturn> registration,
  std::shared_ptr<const NotificationContextLuaStateNamePrefixReturn> written_name,
  std::shared_ptr<const NotificationContextLuaProfileReturn> completed_profile,
  std::shared_ptr<const NotificationContextLuaControlsTableReturn> written_controls,
  std::size_t actual_slot,std::uint64_t active_scope,std::uint64_t q,std::uint64_t e,std::uint64_t w,std::uint64_t u)
  :host(std::move(actual)),facade(f),source(std::move(owner)),prepared(std::move(installed)),name(std::move(physical_name)),
   resize(std::move(registration)),state_name(std::move(written_name)),profile(std::move(completed_profile)),controls(std::move(written_controls)),
   slot(actual_slot),scope(active_scope),serial(q),epoch(e),writes(w),uncertainty(u){}
 void first(const runtime::BnwWorldViewSourceOperationLoan&op,
  const std::shared_ptr<const NotificationContextLuaStatePreparation>& installed)const {
  const auto p=host.lock();const auto actual=canonical.lock();
  if(!p||!actual||actual->impl_.get()!=this||creator!=std::this_thread::get_id()||!alive.load()||!facade||
   !p->end_turn_lifetime||!p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||
   p->end_turn_lifetime->host!=facade||p->end_turn_lifetime->vm!=p->state)
   throw runtime::ScriptError("completed Setup actual VM facade/return retired or issuer unavailable");
  facade->require_context_lua_load_setup_current_without_callbacks(op,installed,*actual);
 }
};
NotificationContextLuaLoadSetupReturn::NotificationContextLuaLoadSetupReturn(std::shared_ptr<Impl> p):impl_(std::move(p)){}
NotificationContextLuaLoadSetupReturn::~NotificationContextLuaLoadSetupReturn(){impl_->alive.store(false);}
NotificationContextLuaLoadSetupReturn::Guard::Guard(std::shared_ptr<Impl>p):impl_(std::move(p)){}
NotificationContextLuaLoadSetupReturn::Guard NotificationContextLuaLoadSetupReturn::capture_guard_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan&op,const std::shared_ptr<const NotificationContextLuaStatePreparation>&installed)const {
 const auto saved=impl_;Guard result(saved);result.require_current_without_callbacks(op,installed);return result;
}
void NotificationContextLuaLoadSetupReturn::Guard::require_current_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan&op,const std::shared_ptr<const NotificationContextLuaStatePreparation>&installed)const {
 const auto saved=impl_;if(!saved)throw runtime::ScriptError("completed Setup actual closed payload absent");saved->first(op,installed);
}
void NotificationContextLuaLoadSetupReturn::Guard::require_current_owned(
 const runtime::BnwWorldViewSourceOperationLoan&op,const std::shared_ptr<const NotificationContextLuaStatePreparation>&input)const {
 const auto saved=impl_;const auto installed=input;require_current_without_callbacks(op,installed);
 op.require_current();require_current_without_callbacks(op,installed);
 op.require_same_vm(*saved->facade);require_current_without_callbacks(op,installed);
 // actual installed Source descriptor, never a renamed cold Context/control ID.
 const auto p=saved->host.lock();if(!p)throw runtime::ScriptError("completed Setup actual Host retired before FULL");
 saved->first(op,installed);
 const auto descriptor=p->child_contexts[saved->slot].source;
 if(!descriptor)throw runtime::ScriptError("completed Setup actual installed Source descriptor retired");
 op.require_same_source(*descriptor);require_current_without_callbacks(op,installed);
}
NotificationContextLuaControlsNamedReference::Guard::Guard(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
NotificationContextLuaControlsNamedReference::Guard NotificationContextLuaControlsNamedReference::capture_guard_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan&op,const std::shared_ptr<const NotificationContextLuaStatePreparation>&prepared,
 const std::shared_ptr<const NotificationContextRootDefaultFieldsReturn>&defaults)const {
 const auto saved=impl_;Guard result(saved);result.require_current_without_callbacks(op,prepared,defaults);return result;
}
void NotificationContextLuaControlsNamedReference::Guard::require_current_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan&op,const std::shared_ptr<const NotificationContextLuaStatePreparation>&prepared,
 const std::shared_ptr<const NotificationContextRootDefaultFieldsReturn>&defaults)const {
 const auto saved=impl_;if(!saved)throw runtime::ScriptError("entered Controls actual payload absent");
 const auto opaque=saved->host.lock();const auto canonical=saved->entered_canonical.lock();
 if(!opaque||!canonical||canonical->impl_!=saved||!saved->facade)
  throw runtime::ScriptError("entered Controls actual canonical return unavailable; cold reference cannot become entered guard");
 const auto p=std::static_pointer_cast<const runtime::LuaScriptHost::Impl>(opaque);
 if(!p->end_turn_lifetime||!p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||
  p->end_turn_lifetime->host!=saved->facade||p->end_turn_lifetime->vm!=p->state)
  throw runtime::ScriptError("entered Controls actual VM facade retired");
 saved->facade->require_context_controls_entered_current_without_callbacks(op,prepared,defaults,*canonical);
}
void NotificationContextLuaControlsNamedReference::Guard::require_current_owned(
 const runtime::BnwWorldViewSourceOperationLoan&op,const std::shared_ptr<const NotificationContextLuaStatePreparation>&input,
 const std::shared_ptr<const NotificationContextRootDefaultFieldsReturn>&default_input)const {
 const auto saved=impl_;const auto prepared=input;const auto defaults=default_input;
 require_current_without_callbacks(op,prepared,defaults);
 saved->facade->require_context_controls_entered_current_owned(op,prepared,defaults,*saved->entered_canonical.lock());
 require_current_without_callbacks(op,prepared,defaults);
}
}
namespace civ5::runtime {
void LuaScriptHost::require_context_lua_load_setup_current_without_callbacks(const BnwWorldViewSourceOperationLoan&op,
 const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&input,
 const ui::detail::NotificationContextLuaLoadSetupReturn&completed)const {
 const auto p=impl_;const auto prepared=input;const auto r=completed.impl_;
 if(!r||!prepared||!prepared->impl_)throw ScriptError("completed Setup actual installed518/payload absent");
 const auto owner=p->worldview_sources.lock();const auto original=r->source.lock();
 if(!owner||owner!=original)throw ScriptError("completed Setup canonical Source owner retired/replaced FIRST");
 owner->require_operation_origin_without_callbacks(op,r->scope); // FIRST before old serial/op
 const auto saved=prepared->impl_;const auto canonical=r->canonical.lock();
 if(r->host.lock()!=p||r->facade!=this||!canonical||canonical.get()!=&completed||!r->alive.load()||r->creator!=std::this_thread::get_id()||
  p->ui_thread!=std::this_thread::get_id()||!p->state||p->failed||p->end_turn_native_active||
  r->prepared!=prepared||saved->facade!=this||saved->host.expired()||saved->host.owner_before(p)||p.owner_before(saved->host)||
  saved->core.expired()||saved->child>=p->child_contexts.size()||r->slot!=saved->child||r->serial!=op.operation_serial()||
  r->epoch!=p->failure_epoch||r->writes!=p->failed.writes_||r->uncertainty!=p->execution_uncertainty||
  !r->name||!r->resize||!r->state_name||!r->state_name->impl_||!r->profile||!r->profile->impl_||!r->controls||!r->controls->impl_)
  throw ScriptError("completed Setup SAME owner/installed518/earned effect returns changed");
 if(p->calling&&!p->worldview_native_host)throw ScriptError("completed Setup foreign protected call entered");
 if(p->worldview_native_host){p->worldview_require_callback_origin();
  if(p->worldview_native_host->loan!=&op||p->worldview_native_host->child!=saved->child)
   throw ScriptError("completed Setup foreign active native parent518");}
 const auto sn=r->state_name->impl_;const auto profile=r->profile->impl_;const auto controls=r->controls->impl_;
 const auto&current=p->child_contexts[saved->child];
 if(current.retired||current.loaded||current.loading||!current.state_prepared||
  current.load_setup_completed_return.lock()!=canonical||current.source!=saved->source||
  current.environment!=saved->environment||current.execution_thread!=saved->thread||
  current.setup_state_name_return.lock()!=r->state_name||current.profile_completed_return.lock()!=r->profile||
  current.controls_table_return.lock()!=r->controls||p->profile_gc_stopped||!p->profile_include_master_alive||
  sn->preparation!=prepared||sn->name.lock()!=r->name||sn->facade!=this||sn->host.expired()||sn->host.owner_before(p)||p.owner_before(sn->host)||
  sn->child!=saved->child||sn->serial!=r->serial||sn->epoch!=r->epoch||sn->writes!=r->writes||sn->uncertainty!=r->uncertainty||
  profile->preparation!=prepared||profile->name!=r->name||profile->state_name!=r->state_name||profile->facade!=this||
  profile->host.expired()||profile->host.owner_before(p)||p.owner_before(profile->host)||profile->serial!=r->serial||
  profile->epoch!=r->epoch||profile->writes!=r->writes||profile->uncertainty!=r->uncertainty||
  controls->prepared!=prepared||controls->name!=r->name||controls->state_name!=r->state_name||controls->facade!=this||
  controls->host.expired()||controls->host.owner_before(p)||p.owner_before(controls->host)||controls->child!=saved->child||
  controls->serial!=r->serial||controls->epoch!=r->epoch||controls->writes!=r->writes||controls->uncertainty!=r->uncertainty||
  !Impl::native_entered_controls_physical_without_callbacks(*p,saved->child,saved->environment,saved->thread,
   saved->actual_environment,saved->actual_thread,controls->table,controls->reference,sn->value))
  throw ScriptError("completed Setup genuine registrars/Controls output or final raw518/StateName/Controls POST absent");
}
void LuaScriptHost::require_context_controls_entered_current_without_callbacks(const BnwWorldViewSourceOperationLoan&op,
 const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&input,
 const std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn>&default_input,
 const ui::detail::NotificationContextLuaControlsNamedReference&named)const {
 const auto p=impl_;const auto prepared=input;const auto defaults=default_input;const auto r=named.impl_;
 if(!defaults||!prepared||!prepared->impl_||!r)throw ScriptError("entered Controls actual Root defaults/preparation/reference absent");
 const auto root=defaults->capture_guard_without_callbacks(op,prepared);root.require_current_without_callbacks(op,prepared);
 if(!r->entered_setup)throw ScriptError("entered Controls requires authentic completedSetup; cold reference has no authority");
 require_context_lua_load_setup_current_without_callbacks(op,prepared,*r->entered_setup);
 const auto saved=prepared->impl_;const auto setup=r->entered_setup->impl_;const auto table=setup->controls->impl_;
 if(r->creator!=std::this_thread::get_id()||r->facade!=this||r->host.expired()||r->host.owner_before(p)||p.owner_before(r->host)||
  r->entered_canonical.lock().get()!=&named||r->table_return!=setup->controls||r->defaults!=defaults||
  p->child_contexts[saved->child].controls_named_entered_return.lock().get()!=&named||r->child!=saved->child||
  r->serial!=op.operation_serial()||r->epoch!=p->failure_epoch||r->writes!=p->failed.writes_||r->uncertainty!=p->execution_uncertainty||
  r->table!=table->table||r->reference<0||
  r->local_lifetime!=ui::detail::NotificationContextLuaControlsNamedReference::Impl::LocalLifetime::live_entered||
  r->local_state!=p->state||r->local_reference!=r->reference||!r->local_retirement.expired()||
  !Impl::native_entered_controls_physical_without_callbacks(*p,saved->child,saved->environment,saved->thread,
   saved->actual_environment,saved->actual_thread,table->table,table->reference,setup->state_name->impl_->value,r->reference))
  throw ScriptError("entered Controls SAME Root/reference/518/StateName/table final physical POST changed");
}
void LuaScriptHost::require_context_controls_entered_current_owned(const BnwWorldViewSourceOperationLoan&op,
 const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&input,
 const std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn>&default_input,
 const ui::detail::NotificationContextLuaControlsNamedReference&named)const {
 const auto p=impl_;const auto prepared=input;const auto defaults=default_input;
 require_context_controls_entered_current_without_callbacks(op,prepared,defaults,named);
 const auto root=defaults->capture_guard_without_callbacks(op,prepared);
 // The genuine default-field loan expressly forbids its FULL in NativeScope.
 // Outer acquisition/export runs it before/after real protected scopes; while
 // a SAME native scope is active, call Source FULL then pure actual Root FIRST.
 if(!p->worldview_native_host)root.require_current_owned(op,prepared);
 require_context_controls_entered_current_without_callbacks(op,prepared,defaults,named);
 op.require_current();require_context_controls_entered_current_without_callbacks(op,prepared,defaults,named);
 op.require_same_vm(*this);require_context_controls_entered_current_without_callbacks(op,prepared,defaults,named);
 const auto descriptor=prepared->impl_->source;
 op.require_same_source(*descriptor);require_context_controls_entered_current_without_callbacks(op,prepared,defaults,named);
}
std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedReference>
LuaScriptHost::acquire_context_controls_named_reference_owned(const BnwWorldViewSourceOperationLoan&op,
 const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>&prepared_input,
 const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>&name_input,
 const std::shared_ptr<const ui::detail::NotificationContextLuaLoadSetupReturn>&setup_input,
 const std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn>&default_input)const {
 using Return=ui::detail::NotificationContextLuaControlsNamedReference;
 const auto p=impl_;const auto prepared=prepared_input;const auto name=name_input;const auto setup=setup_input;const auto defaults=default_input;
 try{
  if(!defaults||!setup)throw ScriptError("entered Controls actual Root default/completedSetup required");
  const auto root=defaults->capture_guard_without_callbacks(op,prepared);root.require_current_owned(op,prepared);
  require_context_lua_load_setup_current_without_callbacks(op,prepared,*setup);
  const auto saved=prepared->impl_;const auto completed=setup->impl_;const auto controls=completed->controls;
  const auto table=controls->impl_;const auto state_name=completed->state_name;
  if(completed->name!=name||!p->child_contexts[saved->child].controls_named_entered_return.expired())
   throw ScriptError("entered Controls actual completed name mismatch or named-ref replay");
  const auto full=[&]{root.require_current_without_callbacks(op,prepared);
   require_context_lua_load_setup_current_without_callbacks(op,prepared,*setup);
   root.require_current_owned(op,prepared);root.require_current_without_callbacks(op,prepared);
   require_context_lua_load_setup_current_without_callbacks(op,prepared,*setup);};
  full();std::shared_ptr<const Return> result;
  {
   Impl::WorldViewNativeHostScope native(*p,*this,op);native.child=saved->child;native.child_source=saved->source;
   Impl::NativeEnteredControlsFrame frame(*p,*this,op,prepared,setup,root,saved->source,saved->child,saved->environment,
    saved->thread,table->reference,saved->actual_environment,saved->actual_thread,table->table,state_name->impl_->value);
   frame.full();const auto context_view=saved->source->context();frame.full();const std::string context(context_view);frame.full();
   Impl::Job job{p.get(),Impl::Operation::native_context_entered_controls_named_lookup,context,{},{},{},{},{}};
   job.worldview_child=saved->child;frame.full();p->call(job);frame.full();
   if(job.context_controls_named_reference<0||job.context_controls_table!=table->table||job.context_controls_reference!=table->reference||
    job.context_state_environment!=saved->actual_environment||job.context_state_thread!=saved->actual_thread)
    throw ScriptError("entered Controls actual8F85A0 protected named-ref return absent");
   auto fields=std::make_shared<Return::Impl>(p,controls,defaults,this,saved->child,op.operation_serial(),
    p->failure_epoch,p->failed.writes_,p->execution_uncertainty,table->table,job.context_controls_named_reference,setup);frame.full();
   result=std::shared_ptr<const Return>(new Return(fields));frame.full();
   fields->local_state=p->state;fields->local_reference=job.context_controls_named_reference;
   fields->local_lifetime=Return::Impl::LocalLifetime::live_entered;
   fields->entered_canonical=result;p->child_contexts[saved->child].controls_named_entered_return=result;native.returned=true;
  }
  full();require_context_controls_entered_current_without_callbacks(op,prepared,defaults,*result);return result;
 }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
