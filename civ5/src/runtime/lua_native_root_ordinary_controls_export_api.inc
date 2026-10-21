// Outside Host::Impl. Real ordinary exported table/CData/metatype retained
// output only. Its original lexical UI loan is never renewed after v64 exit.
namespace civ5::ui::detail {
struct NotificationContextRootOrdinaryControlsExportReturn::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const std::weak_ptr<runtime::LuaScriptHost::Impl> host;
 const runtime::LuaScriptHost*const facade;
 const std::weak_ptr<const NotificationContextRootOrdinaryControlsExportLoan> operand;
 const NotificationContextRootOrdinaryControlsExportLoan::Guard operand_guard;
 const std::shared_ptr<runtime::LuaScriptHost::Impl::NativeRootOrdinaryControlsResult> actual;
 Impl(std::weak_ptr<runtime::LuaScriptHost::Impl> h,const runtime::LuaScriptHost* f,
  std::weak_ptr<const NotificationContextRootOrdinaryControlsExportLoan> loan,
  NotificationContextRootOrdinaryControlsExportLoan::Guard guard,
  std::shared_ptr<runtime::LuaScriptHost::Impl::NativeRootOrdinaryControlsResult> r)
  :host(std::move(h)),facade(f),operand(std::move(loan)),operand_guard(std::move(guard)),actual(std::move(r)){}
 void first(const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextRootOrdinaryControlsExportLoan& offered)const {
  // SAME Root Source FIRST; strong Host custody and its actual facade lifetime
  // are then established before any historical facade method can be called.
  operand_guard.require_current_without_callbacks(op);
  const auto owner=host.lock();const auto loan=operand.lock();
  if(!owner||!loan||loan.get()!=&offered||creator!=std::this_thread::get_id()||!alive.load()||!actual||
   actual->host.lock()!=owner||actual->operand.lock()!=loan||actual->operation!=&op||
   creator!=owner->ui_thread||!owner->state||owner->failed||owner->calling||owner->end_turn_native_active||
   owner->worldview_native_host||owner->root_ordinary_controls_export_frame||actual->creator!=creator||
   actual->serial!=op.operation_serial()||actual->epoch!=owner->failure_epoch||actual->writes!=owner->failed.writes_||
   actual->uncertainty!=owner->execution_uncertainty||!actual->cdata_creator||
   actual->stage!=runtime::LuaScriptHost::Impl::NativeRootOrdinaryControlsResult::Stage::returned)
   throw runtime::ScriptError("ordinary export actual returned scope/VM retired or foreign");
  const auto lifetime=owner->end_turn_lifetime;const auto index=static_cast<std::size_t>(actual->cdata_creator->subtype);
  if(!lifetime||!lifetime->alive||lifetime->failed||lifetime->host!=facade||lifetime->vm!=owner->state||
   !facade||actual->slot>=owner->child_contexts.size()||index>=6||
   actual->cdata_creator!=owner->native_ordinary_cdata_creators[index]||!actual->cdata_creator->alive.load()||
   actual->cdata_creator->host!=owner.get()||actual->cdata_creator->creator!=creator||
   !actual->cdata_creator->push_metatype)
   throw runtime::ScriptError("ordinary export actual static subtype creator/facade lifetime retired");
  const auto tree=actual->tree.lock();
  if(!tree||!actual->concrete||actual->concrete.get()!=actual->cdata||
   operand_guard.parent_controls_without_callbacks()!=actual->parent||
   operand_guard.receiver_without_callbacks()!=actual->receiver||
   operand_guard.concrete_cdata_owner_without_callbacks()!=actual->concrete||
   operand_guard.subtype_without_callbacks()!=actual->cdata_creator->subtype||operand_guard.tree_without_callbacks()!=tree||
   std::strcmp(operand_guard.terminated_id_without_callbacks(),actual->id.data())||
   std::none_of(owner->root_ordinary_controls_export_results.begin(),owner->root_ordinary_controls_export_results.end(),
    [&](const auto& retained){return retained==actual;}))
   throw runtime::ScriptError("ordinary export real retained concrete state/subtype/parent Controls changed");
  const auto& current=owner->child_contexts[actual->slot];
  if(current.retired||!current.state_prepared||current.environment!=actual->environment||
   current.execution_thread!=actual->thread||current.source!=actual->parent_source||
   current.controls_table_pointer!=actual->parent_table||
   !runtime::LuaScriptHost::Impl::native_ordinary_export_physical_without_callbacks(*owner,*actual))
   throw runtime::ScriptError("ordinary export final actual global types/table/CData/metatype POST changed");
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextRootOrdinaryControlsExportLoan& loan)const {
  first(op,loan);const auto owner=host.lock();if(!owner)throw runtime::ScriptError("ordinary export Host retired before FULL custody");
  operand_guard.require_source_owned(op);first(op,loan);
  // LAST pure first above checked SAME live facade/vm with strong owner held.
  op.require_same_vm(*facade);first(op,loan);
 }
};
NotificationContextRootOrdinaryControlsExportReturn::NotificationContextRootOrdinaryControlsExportReturn(std::shared_ptr<Impl> p)
 :impl_(std::move(p)){}
NotificationContextRootOrdinaryControlsExportReturn::~NotificationContextRootOrdinaryControlsExportReturn(){impl_->alive.store(false);}
NotificationContextRootOrdinaryControlsExportReturn::Guard::Guard(std::shared_ptr<Impl> p):impl_(std::move(p)){}
NotificationContextRootOrdinaryControlsExportReturn::Guard
NotificationContextRootOrdinaryControlsExportReturn::capture_guard_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextRootOrdinaryControlsExportLoan& loan)const {
 Guard g(impl_);g.require_current_without_callbacks(op,loan);return g;
}
void NotificationContextRootOrdinaryControlsExportReturn::Guard::require_current_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextRootOrdinaryControlsExportLoan& loan)const {
 const auto saved=impl_;if(!saved)throw runtime::ScriptError("ordinary export actual return absent");saved->first(op,loan);
}
void NotificationContextRootOrdinaryControlsExportReturn::Guard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextRootOrdinaryControlsExportLoan& loan)const {
 const auto saved=impl_;require_current_without_callbacks(op,loan);saved->full(op,loan);require_current_without_callbacks(op,loan);
}
}
namespace civ5::runtime {
std::shared_ptr<const ui::detail::NotificationContextRootOrdinaryControlsExportReturn>
LuaScriptHost::export_root_ordinary_controls_owned(const BnwWorldViewSourceOperationLoan& op,
 const std::shared_ptr<const ui::detail::NotificationContextRootOrdinaryControlsExportLoan>& input)const {
 using Return=ui::detail::NotificationContextRootOrdinaryControlsExportReturn;
 using Result=Impl::NativeRootOrdinaryControlsResult;
 const auto loan=input;const auto p=impl_;
 try{
  if(!loan)throw ScriptError("ordinary +7C needs real factory-derived lexical4FD460 operand");
  const auto guard=loan->capture_guard_without_callbacks(op);guard.require_source_owned(op);
  const auto parent=guard.parent_controls_without_callbacks();const auto physical=guard.concrete_cdata_owner_without_callbacks();
  const auto tree=guard.tree_without_callbacks();const auto receiver=guard.receiver_without_callbacks();
  const auto subtype=guard.subtype_without_callbacks();const auto index=static_cast<std::size_t>(subtype);
  if(!parent||!parent->impl_||!parent->impl_->table_return||!parent->impl_->table_return->impl_||index>=6)
   throw ScriptError("ordinary +7C actual parent Controls table or executed concrete subtype absent");
  const auto named=parent->impl_;const auto table_return=named->table_return;
  const auto table=table_return->impl_;const auto prepared=table->prepared;
  if(!prepared||!prepared->impl_||!physical||!tree||!receiver)
   throw ScriptError("ordinary +7C real prepared parent518/concrete state absent");
  const auto saved=prepared->impl_;const auto type=p->native_ordinary_cdata_creators[index];
  const auto pure=[&]{
   guard.require_current_without_callbacks(op);
   const auto lifetime=p->end_turn_lifetime;
   if(std::this_thread::get_id()!=p->ui_thread||!p->state||p->failed||p->calling||p->end_turn_native_active||
    !lifetime||!lifetime->alive||lifetime->failed||lifetime->host!=this||lifetime->vm!=p->state||
    named->creator!=std::this_thread::get_id()||named->facade!=this||named->host.expired()||
    named->host.owner_before(p)||p.owner_before(named->host)||named->child!=saved->child||
    named->table!=table->table||named->serial!=op.operation_serial()||named->epoch!=p->failure_epoch||
    named->writes!=p->failed.writes_||named->uncertainty!=p->execution_uncertainty||
    saved->facade!=this||saved->host.expired()||saved->host.owner_before(p)||p.owner_before(saved->host)||
    saved->child>=p->child_contexts.size()||p->child_contexts[saved->child].retired||
    p->child_contexts[saved->child].source!=saved->source||p->child_contexts[saved->child].environment!=saved->environment||
    p->child_contexts[saved->child].execution_thread!=saved->thread||
    p->child_contexts[saved->child].controls_table_return.lock()!=table_return||
    p->child_contexts[saved->child].controls_table_pointer!=named->table||
    !type||p->native_ordinary_cdata_creators[index]!=type||!type->alive.load()||type->host!=p.get()||
    type->creator!=std::this_thread::get_id()||type->subtype!=subtype||!type->push_metatype||
    guard.parent_controls_without_callbacks()!=parent||guard.concrete_cdata_owner_without_callbacks()!=physical||
    guard.tree_without_callbacks()!=tree||guard.receiver_without_callbacks()!=receiver||guard.subtype_without_callbacks()!=subtype)
    throw ScriptError("ordinary +7C actual parent table/518/static creator/concrete state changed");
  };
  const auto full=[&]{pure();guard.require_source_owned(op);pure();op.require_same_vm(*this);pure();};full();
  if(std::any_of(p->root_ordinary_controls_export_results.begin(),p->root_ordinary_controls_export_results.end(),
   [&](const auto& old){return old&&old->operand.lock()==loan;}))throw ScriptError("ordinary +7C operand replay");
  std::array<char,32> id{};const char*actual_id=guard.terminated_id_without_callbacks();
  const std::size_t id_length=std::strlen(actual_id);
  if(!id_length||id_length>=id.size())throw ScriptError("ordinary +7C actual positiveID outside native32 custody");
  std::memcpy(id.data(),actual_id,id_length+1);pure();
  // The original duplicate screen diagnostic reads real parent3C+B4.
  static_assert(sizeof(wchar_t)==sizeof(char16_t));
  const auto wide=guard.parent_screen_b4_without_callbacks();const auto* wide_pointer=reinterpret_cast<const wchar_t*>(wide.data());
  const int capacity=WideCharToMultiByte(CP_ACP,0,wide_pointer,-1,nullptr,0,nullptr,nullptr);full();
  if(capacity<=0)throw ScriptError("ordinary +7C actual parentB4 ACP conversion length failed");
  std::vector<char> narrow(static_cast<std::size_t>(capacity));full();
  const int written=WideCharToMultiByte(CP_ACP,0,wide_pointer,-1,narrow.data(),capacity,nullptr,nullptr);full();
  if(written!=capacity||narrow.back()!='\0')throw ScriptError("ordinary +7C actual parentB4 ACP conversion failed");
  std::string screen(narrow.data(),narrow.size()-1);full();
  auto actual=std::make_shared<Result>(p,loan,type,parent,physical,tree,saved->source,receiver,&op,saved->child,
   op.operation_serial(),p->failure_epoch,p->failed.writes_,p->execution_uncertainty,
   saved->actual_environment,saved->actual_thread,saved->environment,saved->thread,id,
   std::move(screen),named->reference,named->table);full();
  p->root_ordinary_controls_export_results.reserve(p->root_ordinary_controls_export_results.size()+1);full();
  p->root_ordinary_controls_export_results.push_back(actual);full();std::shared_ptr<const Return> result;
  {
   Impl::WorldViewNativeHostScope native(*p,*this,op);native.child=saved->child;native.child_source=saved->source;
   Impl::NativeRootOrdinaryControlsFrame frame(*p,*this,op,loan,guard,actual,physical,tree);frame.full();
   const auto context_view=saved->source->context();frame.full();const std::string context(context_view);frame.full();
   if(p->child_contexts[saved->child].name!=context)throw ScriptError("ordinary +7C actual parent context name changed");
   Impl::Job job{p.get(),Impl::Operation::native_root_ordinary_controls_export,context,{},{},{},{},{}};
   job.worldview_child=saved->child;frame.full();p->call(job);frame.full();
   if(actual->stage!=Result::Stage::assigned||!Impl::native_ordinary_export_physical_without_callbacks(*p,*actual))
    throw ScriptError("ordinary +7C actual table/CData/metatype job did not return");
   auto fields=std::make_shared<Return::Impl>(p,this,loan,guard,actual);frame.full();
   result=std::shared_ptr<const Return>(new Return(fields));frame.full();native.returned=true;
  }
  full();if(p->worldview_native_host||p->root_ordinary_controls_export_frame||!result||
   !Impl::native_ordinary_export_physical_without_callbacks(*p,*actual))
   throw ScriptError("ordinary +7C actual LAST FULL/protected scope DTOR final POST failed");
  actual->stage=Result::Stage::returned;const auto returned=result->capture_guard_without_callbacks(op,*loan);
  returned.require_current_without_callbacks(op,*loan);return result;
 }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
