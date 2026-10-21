// Included inside runtime AFTER complete BnwSelectedUserOptionsOwner::Impl.
} //namespace civ5::runtime; UI-owned opaque type definitions need their namespace.
namespace civ5::ui::detail {
using ::civ5::runtime::need;
using ::civ5::runtime::BnwSelectedUserOptionsOwner;
using ::civ5::runtime::BnwSelectedWorldViewContextOwner;
using ::civ5::runtime::BnwWorldViewSourceOperationLoan;
struct ui::detail::NotificationContextRootSmallUiPolicyLoan::Guard::Capture final {
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<const BnwSelectedUserOptionsOwner> owner;
 const std::shared_ptr<BnwSelectedUserOptionsOwner::Impl> storage;
 const std::weak_ptr<BnwSelectedWorldViewContextOwner> source;
 const BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t scope,serial,revision;
 const bool branch=false;
 Capture(std::weak_ptr<const BnwSelectedUserOptionsOwner> o,
  std::shared_ptr<BnwSelectedUserOptionsOwner::Impl> s,
  std::weak_ptr<BnwSelectedWorldViewContextOwner> actual,
  const BnwWorldViewSourceOperationLoan*op,std::uint64_t n,std::uint64_t id,std::uint64_t r)
  :owner(std::move(o)),storage(std::move(s)),source(std::move(actual)),operation(op),scope(n),serial(id),revision(r){}
};
struct ui::detail::NotificationContextRootSmallUiPolicyLoan::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 const std::shared_ptr<const Guard::Capture> capture;
 explicit Impl(std::shared_ptr<const Guard::Capture> p):capture(std::move(p)){}
 void first(const BnwWorldViewSourceOperationLoan*offered)const{
  need(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),"SmallUI Loan retired/foreign/failed FIRST");
  const auto p=capture;need(p&&p->creator==std::this_thread::get_id()&&p->storage,"SmallUI capture foreign/absent");
  const auto s=p->storage;
  need(s->thread==std::this_thread::get_id()&&!s->retired.load(),"SmallUI selectedUserOptions retired/foreign FIRST");
  if(!offered)offered=p->operation; //Retained pointer DATA only AFTER own lifetime FIRST.
  need(offered&&offered==p->operation&&p->scope&&p->serial,"SmallUI SAME Source operand metadata absent");
  const auto source=p->source.lock();need(bool(source),"SmallUI Source facade retired FIRST");
  // Physical Source active pointer+scope FIRST. No raw op method precedes it.
  need(source->capture_active_operation_scope_serial_without_callbacks(offered)==p->scope,
   "SmallUI actual Source active operand/scope replaced FIRST");
  source->require_operation_origin_without_callbacks(*offered,p->scope);
  const auto owner=p->owner.lock();const auto canonical=s->self.lock();
  need(owner&&canonical&&owner.get()==canonical.get()&&!owner.owner_before(canonical)&&!canonical.owner_before(owner)&&
   owner->impl_.get()==s.get()&&!owner->impl_.owner_before(s)&&!s.owner_before(owner->impl_),
   "SmallUI actual UserOptions canonical facade/storage changed");
  need(s->input.file&&s->input.interface_small&&s->input.interface_auto&&
   *s->input.interface_small=="0"&&*s->input.interface_auto=="0"&&s->interface_registered&&
   !s->interface_small_value&&!s->interface_auto_value&&s->interface_revision==p->revision&&p->revision&&!p->branch,
   "SmallUI actual explicit Interface registry/input/revision changed");
  offered->require_origin_without_callbacks();
 }
 void full(const BnwWorldViewSourceOperationLoan& op)const{
  first(&op);const auto owner=capture->owner.lock();need(bool(owner),"SmallUI Input owner retired");
  owner->require_current();first(&op);op.require_operation(capture->serial);first(&op);
 }
};
ui::detail::NotificationContextRootSmallUiPolicyLoan::NotificationContextRootSmallUiPolicyLoan(std::shared_ptr<Impl> p):impl_(std::move(p)){}
void ui::detail::NotificationContextRootSmallUiPolicyLoan::retire_owned()const noexcept{impl_->alive.store(false);}
ui::detail::NotificationContextRootSmallUiPolicyLoan::~NotificationContextRootSmallUiPolicyLoan(){retire_owned();}
void ui::detail::NotificationContextRootSmallUiPolicyLoan::require_current_without_callbacks(const BnwWorldViewSourceOperationLoan& op)const{impl_->first(&op);}
void ui::detail::NotificationContextRootSmallUiPolicyLoan::require_current_owned(const BnwWorldViewSourceOperationLoan& op)const{
 const auto saved=impl_;try{saved->full(op);}catch(...){saved->failed.store(true);throw;}
}
bool ui::detail::NotificationContextRootSmallUiPolicyLoan::branch_taken_without_callbacks()const{
 const auto saved=impl_;saved->first(nullptr);return saved->capture->branch;
}
bool ui::detail::NotificationContextRootSmallUiPolicyLoan::small_ui_assets_without_callbacks()const{
 const auto saved=impl_;saved->first(nullptr);
 // Read the actual field published by the authenticated Interface producer;
 // branch_taken is a separate RootRead policy and is not the getter value.
 return saved->capture->storage->interface_small_value;
}
ui::detail::NotificationContextRootSmallUiPolicyLoan::Guard::Guard(std::shared_ptr<Impl> p,std::shared_ptr<const Capture> c):impl_(std::move(p)),capture_(std::move(c)){}
ui::detail::NotificationContextRootSmallUiPolicyLoan::Guard
ui::detail::NotificationContextRootSmallUiPolicyLoan::capture_guard_without_callbacks(const BnwWorldViewSourceOperationLoan& op)const{
 const auto saved=impl_;saved->first(&op);return Guard(saved,saved->capture); //VALUE, no allocation/callback.
}
void ui::detail::NotificationContextRootSmallUiPolicyLoan::Guard::require_current_without_callbacks(const BnwWorldViewSourceOperationLoan& op)const{
 impl_->first(&op);need(impl_->capture.get()==capture_.get()&&!impl_->capture.owner_before(capture_)&&!capture_.owner_before(impl_->capture),
  "SmallUI captured Guard allocation/control changed");
}
void ui::detail::NotificationContextRootSmallUiPolicyLoan::Guard::require_source_owned(const BnwWorldViewSourceOperationLoan& op)const{
 require_current_without_callbacks(op);try{impl_->full(op);}catch(...){impl_->failed.store(true);throw;}require_current_without_callbacks(op);
}
bool ui::detail::NotificationContextRootSmallUiPolicyLoan::Guard::branch_taken_without_callbacks()const{
 impl_->first(nullptr);need(impl_->capture.get()==capture_.get()&&!impl_->capture.owner_before(capture_)&&!capture_.owner_before(impl_->capture),
  "SmallUI captured Guard control changed");return capture_->branch;
}

} //namespace civ5::ui::detail
namespace civ5::runtime {
std::unique_ptr<const ui::detail::NotificationContextRootSmallUiPolicyLoan>
BnwSelectedUserOptionsOwner::context_small_ui_policy_owned(const BnwWorldViewSourceOperationLoan& op,
 const std::weak_ptr<BnwSelectedWorldViewContextOwner>& supplied,std::uint64_t scope)const{
 using Loan=ui::detail::NotificationContextRootSmallUiPolicyLoan;
 const auto saved=impl_;need(saved->thread==std::this_thread::get_id()&&!saved->retired.load(),"SmallUI UserOptions FIRST");
 const auto source=supplied.lock();need(bool(source),"SmallUI actual Source absent");
 source->require_operation_origin_without_callbacks(op,scope); //Before raw op methods.
 const auto owner=saved->self.lock();need(owner&&owner.get()==this&&owner->impl_.get()==saved.get(),"SmallUI canonical UserOptions absent");
 need(!saved->interface_querying,"SmallUI registry producer reentry");
 saved->interface_querying=true;struct End{Impl& s;~End(){s.interface_querying=false;}}end{*saved};
 const auto pure=[&]{need(saved->thread==std::this_thread::get_id()&&!saved->retired.load(),"SmallUI producer retired FIRST");
  source->require_operation_origin_without_callbacks(op,scope);op.require_origin_without_callbacks();};
 const auto full=[&]{pure();owner->require_current();pure();op.require_current();pure();};
 try{
  full();need(saved->input.file&&saved->input.interface_small&&saved->input.interface_auto&&
   *saved->input.interface_small=="0"&&*saved->input.interface_auto=="0",
   "SmallUI requires explicit SAME selected UserSettings Small=0 Auto=0; missing/true/unknown STOP");
  // Real normalized registry publication; present explicit keys only, no defaults.
  if(!saved->interface_registered){
   need(saved->interface_revision==0,"SmallUI incomplete/replayed Interface registration");
   saved->interface_small_value=false;saved->interface_auto_value=false;
   saved->interface_revision=1;saved->interface_registered=true;
  }
  full();need(!saved->interface_small_value&&!saved->interface_auto_value&&saved->interface_revision==1,
   "SmallUI effective Interface registration changed");
  const auto serial=op.operation_serial();const auto revision=saved->interface_revision;
  //Every operand is an owned VALUE before allocations, never field lvalueref.
  auto capture=std::make_shared<const Loan::Guard::Capture>(owner,saved,source,&op,scope,serial,revision);full();
  auto state=std::make_shared<Loan::Impl>(capture);full();
  auto result=std::unique_ptr<const Loan>(new Loan(state));full();result->require_current_without_callbacks(op);return result;
 }catch(...){saved->retired.store(true);throw;}
}
