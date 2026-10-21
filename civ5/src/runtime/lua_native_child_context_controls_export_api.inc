// Outside actual Host::Impl, after authentic Preparation/Controls definitions.
namespace civ5::runtime {
LuaScriptHost::Impl::NativeContextCDataCreator::NativeContextCDataCreator(const Impl* actual)
 :host(actual),type_name("LuaContext"),push_metatype(&Impl::native_child_context_push_existing_metatable){}
}
namespace civ5::ui::detail {
struct NotificationContextChildControlsExportReturn::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const std::weak_ptr<runtime::LuaScriptHost::Impl> host;
 const runtime::LuaScriptHost*const facade;
 const std::weak_ptr<const NotificationContextChildControlsExportLoan> operand;
 const NotificationContextChildControlsExportLoan::Guard operand_guard;
 const std::shared_ptr<runtime::LuaScriptHost::Impl::NativeChildContextControlsResult> actual;
 Impl(std::weak_ptr<runtime::LuaScriptHost::Impl> h,const runtime::LuaScriptHost* f,
  std::weak_ptr<const NotificationContextChildControlsExportLoan> loan,
  NotificationContextChildControlsExportLoan::Guard guard,
  std::shared_ptr<runtime::LuaScriptHost::Impl::NativeChildContextControlsResult> r)
  :host(std::move(h)),facade(f),operand(std::move(loan)),operand_guard(std::move(guard)),actual(std::move(r)){}
 void first(const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextChildControlsExportLoan& offered)const {
  // Actual Root Source FIRST before dereferencing historical op or facade.
  operand_guard.require_current_without_callbacks(op);
  const auto owner=host.lock();const auto loan=operand.lock();
  if(!owner||!loan||loan.get()!=&offered||creator!=std::this_thread::get_id()||!alive.load()||!actual||
   actual->host.lock()!=owner||actual->operand.lock()!=loan||actual->operation!=&op||
   creator!=owner->ui_thread||!owner->state||owner->failed||owner->calling||owner->end_turn_native_active||
   owner->worldview_native_host||owner->child_controls_export_frame||actual->creator!=creator||
   actual->serial!=op.operation_serial()||actual->epoch!=owner->failure_epoch||actual->writes!=owner->failed.writes_||
   actual->uncertainty!=owner->execution_uncertainty||
   actual->stage!=runtime::LuaScriptHost::Impl::NativeChildContextControlsResult::Stage::returned)
   throw runtime::ScriptError("Context child export sealed return retired/foreign/failed or protected scope not returned");
  const auto lifetime=owner->end_turn_lifetime;
  if(!lifetime||!lifetime->alive||lifetime->failed||lifetime->host!=facade||lifetime->vm!=owner->state||
   !facade||actual->slot>=owner->child_contexts.size()||!actual->cdata_creator||
   actual->cdata_creator!=owner->native_context_cdata_creator||!actual->cdata_creator->alive.load()||
   actual->cdata_creator->host!=owner.get()||actual->cdata_creator->creator!=creator||
   actual->cdata_creator->push_metatype!=&runtime::LuaScriptHost::Impl::native_child_context_push_existing_metatable)
   throw runtime::ScriptError("Context child export actual SAME VM facade/creator retired");
  const auto physical=actual->child.lock();const auto tree=actual->child_tree.lock();
  if(!physical||!tree||physical.get()!=actual->child_cdata||
   operand_guard.parent_controls_without_callbacks()!=actual->parent||
   operand_guard.child_source_without_callbacks()!=actual->child_source||
   operand_guard.child_receiver_without_callbacks()!=actual->receiver||
   operand_guard.child_context_without_callbacks()!=physical||operand_guard.child_tree_without_callbacks()!=tree||
   std::strcmp(operand_guard.terminated_id_without_callbacks(),actual->id.data())||
   std::none_of(owner->child_controls_export_results.begin(),owner->child_controls_export_results.end(),
    [&](const auto& retained){return retained==actual;}))
   throw runtime::ScriptError("Context child export real retained parent/child/source/CData/ID changed");
  const auto& current=owner->child_contexts[actual->slot];
  if(current.retired||!current.state_prepared||current.environment!=actual->environment||
   current.execution_thread!=actual->thread||current.source!=actual->parent_source||
   current.controls_table_pointer!=actual->parent_table||
   !runtime::LuaScriptHost::Impl::native_child_context_export_physical_without_callbacks(*owner,*actual))
   throw runtime::ScriptError("Context child export final physical parent Controls/child CData/metatype POST changed");
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextChildControlsExportLoan& loan)const {
  first(op,loan);operand_guard.require_source_owned(op);first(op,loan);
  op.require_same_vm(*facade);first(op,loan);
 }
};
NotificationContextChildControlsExportReturn::NotificationContextChildControlsExportReturn(std::shared_ptr<Impl> p):impl_(std::move(p)){}
NotificationContextChildControlsExportReturn::~NotificationContextChildControlsExportReturn(){impl_->alive.store(false);}
NotificationContextChildControlsExportReturn::Guard::Guard(std::shared_ptr<Impl> p):impl_(std::move(p)){}
NotificationContextChildControlsExportReturn::Guard NotificationContextChildControlsExportReturn::capture_guard_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextChildControlsExportLoan& loan)const {
 const auto saved=impl_;Guard result(saved);result.require_current_without_callbacks(op,loan);return result;
}
void NotificationContextChildControlsExportReturn::Guard::require_current_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextChildControlsExportLoan& loan)const {
 const auto saved=impl_;if(!saved)throw runtime::ScriptError("Context child export actual return control absent");saved->first(op,loan);
}
void NotificationContextChildControlsExportReturn::Guard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextChildControlsExportLoan& loan)const {
 const auto saved=impl_;require_current_without_callbacks(op,loan);saved->full(op,loan);require_current_without_callbacks(op,loan);
}
}
namespace civ5::runtime {
std::shared_ptr<const ui::detail::NotificationContextChildControlsExportReturn>
LuaScriptHost::export_context_child_controls_owned(const BnwWorldViewSourceOperationLoan& op,
 const std::shared_ptr<const ui::detail::NotificationContextChildControlsExportLoan>& input)const {
 using Return=ui::detail::NotificationContextChildControlsExportReturn;
 using Result=Impl::NativeChildContextControlsResult;
 const auto loan=input;const auto p=impl_;
 try{
  if(!loan)throw ScriptError("Context664FB0 requires actual lexical child4FD460 operand");
  const auto guard=loan->capture_guard_without_callbacks(op);guard.require_source_owned(op);
  const auto parent=guard.parent_controls_without_callbacks();
  const auto physical=guard.child_context_without_callbacks();const auto tree=guard.child_tree_without_callbacks();
  const auto source=guard.child_source_without_callbacks();const auto receiver=guard.child_receiver_without_callbacks();
  if(!parent||!parent->impl_||!parent->impl_->table_return||!parent->impl_->table_return->impl_)
   throw ScriptError("Context664FB0 real parent named-reference/table return absent");
  const auto named=parent->impl_;const auto table_return=named->table_return;
  const auto table=table_return->impl_;const auto prepared=table->prepared;
  if(!prepared||!prepared->impl_||!physical||!tree||!source||!receiver)
   throw ScriptError("Context664FB0 genuine parent518 or concrete child absent");
  const auto saved=prepared->impl_;const auto type=p->native_context_cdata_creator;
  const auto pure=[&]{
   guard.require_current_without_callbacks(op);
   if(std::this_thread::get_id()!=p->ui_thread||p->failed||p->calling||p->end_turn_native_active||
    named->creator!=std::this_thread::get_id()||named->facade!=this||named->host.expired()||
    named->host.owner_before(p)||p.owner_before(named->host)||named->child!=saved->child||
    named->table!=table->table||named->serial!=op.operation_serial()||named->epoch!=p->failure_epoch||
    named->writes!=p->failed.writes_||named->uncertainty!=p->execution_uncertainty||
    saved->facade!=this||saved->host.expired()||saved->host.owner_before(p)||p.owner_before(saved->host)||
    saved->child>=p->child_contexts.size()||p->child_contexts[saved->child].retired||
    p->child_contexts[saved->child].source!=saved->source||
    p->child_contexts[saved->child].environment!=saved->environment||
    p->child_contexts[saved->child].execution_thread!=saved->thread||
    p->child_contexts[saved->child].controls_table_return.lock()!=table_return||
    p->child_contexts[saved->child].controls_table_pointer!=named->table||
    !type||p->native_context_cdata_creator!=type||!type->alive.load()||type->host!=p.get()||
    type->creator!=std::this_thread::get_id()||type->push_metatype!=&Impl::native_child_context_push_existing_metatable||
    guard.parent_controls_without_callbacks()!=parent||guard.child_context_without_callbacks()!=physical||
    guard.child_tree_without_callbacks()!=tree||guard.child_source_without_callbacks()!=source||
    guard.child_receiver_without_callbacks()!=receiver)
    throw ScriptError("Context664FB0 actual parent Controls/518/type/physical operands changed");
  };
  const auto full=[&]{pure();guard.require_source_owned(op);pure();op.require_same_vm(*this);pure();};
  full();
  if(std::any_of(p->child_controls_export_results.begin(),p->child_controls_export_results.end(),
    [&](const auto& old){return old&&old->operand.lock()==loan;}))
   throw ScriptError("Context664FB0 actual child export attempt cannot replay");
  std::array<char,32> id{};const char*actual_id=guard.terminated_id_without_callbacks();
  const std::size_t id_length=std::strlen(actual_id);
  if(!id_length||id_length>=id.size())throw ScriptError("Context664FB0 actual constructorID58 outside native32 custody");
  std::memcpy(id.data(),actual_id,id_length+1);pure();
  // Original820000->820040 uses WideCharToMultiByte(CP_ACP,0,-1), proven
  // imported9C9358. SAME physical parent3C+B4 remains owned by loan Guard.
  static_assert(sizeof(wchar_t)==sizeof(char16_t));
  const auto wide=guard.parent_screen_b4_without_callbacks();
  const auto* wide_pointer=reinterpret_cast<const wchar_t*>(wide.data());
  const int capacity=WideCharToMultiByte(CP_ACP,0,wide_pointer,-1,nullptr,0,nullptr,nullptr);full();
  if(capacity<=0)throw ScriptError("Context664FB0 actual parentB4 ACP conversion length failed");
  std::vector<char> narrow(static_cast<std::size_t>(capacity));full();
  const int written=WideCharToMultiByte(CP_ACP,0,wide_pointer,-1,narrow.data(),capacity,nullptr,nullptr);full();
  if(written!=capacity||narrow.back()!='\0')throw ScriptError("Context664FB0 actual parentB4 ACP conversion failed");
  std::string screen(narrow.data(),narrow.size()-1);full();
  auto actual=std::make_shared<Result>(p,loan,type,parent,physical,tree,source,saved->source,receiver,
   &op,saved->child,op.operation_serial(),p->failure_epoch,p->failed.writes_,p->execution_uncertainty,
   saved->actual_environment,saved->actual_thread,saved->environment,saved->thread,physical.get(),id,
   std::move(screen),named->reference,named->table);full();
  p->child_controls_export_results.reserve(p->child_controls_export_results.size()+1);full();
  p->child_controls_export_results.push_back(actual);full();
  std::shared_ptr<const Return> result;
  {
   Impl::WorldViewNativeHostScope native(*p,*this,op);native.child=saved->child;native.child_source=saved->source;
   Impl::NativeChildContextControlsFrame frame(*p,*this,op,loan,guard,actual,physical,tree);frame.full();
   // Retain actual immutable descriptor before callbacks/allocations; never
   // borrow a vector element string across a handler which could grow it.
   const auto context_view=saved->source->context();frame.full();
   const std::string context(context_view);frame.full();
   if(p->child_contexts[saved->child].name!=context)
    throw ScriptError("Context child export actual parent context name changed");
   Impl::Job job{p.get(),Impl::Operation::native_child_context_controls_export,context,{},{},{},{},{}};
   job.worldview_child=saved->child;frame.full();p->call(job);frame.full();
   if(actual->stage!=Result::Stage::assigned||!Impl::native_child_context_export_physical_without_callbacks(*p,*actual))
    throw ScriptError("Context664FB0/6768B0 actual job did not return assigned SAME table/CData/metatype");
   auto fields=std::make_shared<Return::Impl>(p,this,loan,guard,actual);frame.full();
   result=std::shared_ptr<const Return>(new Return(fields));frame.full();
   native.returned=true;
  } // real protected job + frame/native scope DTOR before return authority.
  full();
  if(p->worldview_native_host||p->child_controls_export_frame||!result||
   !Impl::native_child_context_export_physical_without_callbacks(*p,*actual))
   throw ScriptError("Context664FB0/6768B0 final physical POST after LAST FULL/scope DTOR failed");
  actual->stage=Result::Stage::returned;
  const auto returned=result->capture_guard_without_callbacks(op,*loan);
  returned.require_current_without_callbacks(op,*loan);return result;
 }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
