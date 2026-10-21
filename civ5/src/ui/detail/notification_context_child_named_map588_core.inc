// Genuine selected515120 after returned child664FB0/6768B0. The completed
// physical parent's OWN40F map588 supplies the numeric key and real receiver
// cell. Constructor count=0 remains historical; it is not a live-map query.
struct BnwWorldViewControlTree::ContextChildName588Registration final {
 using Read=detail::NotificationContextRootReadChildrenLoan;
 using Receiver=detail::NotificationLayoutOrdinaryReceiver;
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<ContextRootChildAttempt> attempt;
 const Read::Guard read;
 const std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> parent_constructor;
 const std::shared_ptr<const detail::NotificationLayoutControlID58> id58;
 const std::shared_ptr<const Receiver> selected_value;
 context_constructor_suffix_detail::NamedReceiverMap588*const map;
 const std::uint32_t key;
 const std::size_t before_count,after_count;
 const std::uint64_t before_revision,after_revision;
 const bool inserted_null,stored_child;
 ContextChildName588Registration(std::weak_ptr<ContextRootChildAttempt> a,Read::Guard g,
  std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> ctor,
  std::shared_ptr<const detail::NotificationLayoutControlID58> id,std::shared_ptr<const Receiver> value,
  context_constructor_suffix_detail::NamedReceiverMap588* actual_map,std::uint32_t hash,
  std::size_t n,std::size_t m,std::uint64_t q,std::uint64_t r,bool inserted,bool stored)
  :attempt(std::move(a)),read(std::move(g)),parent_constructor(std::move(ctor)),id58(std::move(id)),
   selected_value(std::move(value)),map(actual_map),key(hash),before_count(n),after_count(m),
   before_revision(q),after_revision(r),inserted_null(inserted),stored_child(stored){}
 void first()const {
  require(creator==std::this_thread::get_id(),"child515120 returned association foreign thread");
  read.require_current_without_callbacks();
  const auto a=attempt.lock();const auto root=read.impl_->evidence->core.lock();
  require(a&&root&&a->name588_registration.get()==this&&a->factory&&a->child_core&&a->receiver&&
   root->context_ordinary_state&&root->context_ordinary_state->impl_&&parent_constructor&&
   parent_constructor->impl_&&parent_constructor->impl_->alive.load()&&!parent_constructor->impl_->failed.load()&&
   root->context_ordinary_state->impl_->native_constructor_return==parent_constructor&&
   parent_constructor->impl_->port&&parent_constructor->impl_->port->map588&&
   &*parent_constructor->impl_->port->map588==map&&id58&&
   a->child_core->context_layout_baseline->base_.id_58==id58,
   "child515120 SAME actual parent constructor/map/child ID allocation changed");
  read.require_child_factory_return_identity_without_callbacks(*a->factory);
  parent_constructor->impl_->port->require_completed(); // completed construction, not empty live map
  require(id58->length&&id58->length<id58->bytes.size()&&id58->bytes[id58->length]=='\0'&&
   key==content::saved_info_type_hash(std::string_view(id58->bytes.data(),id58->length))&&
   after_count==before_count+(inserted_null?1u:0u)&&
   after_revision==before_revision+(inserted_null?1u:0u)+(stored_child?1u:0u)&&
   map->cells.size()>=after_count&&map->revision>=after_revision&&selected_value&&
   (!stored_child||selected_value==a->receiver),"child515120 authentic returned key/first-null mutation journal changed");
  const auto at=map->cells.find(key);
  // A non-NULL original cell remains non-NULL even if that receiver's weak
  // external owner retires. weak_ptr::expired is never the nativeNULL test.
  require(at!=map->cells.end()&&at->second==selected_value,
   "child515120 first-wins real parent588 cell replaced/removed");
  read.require_current_without_callbacks();
 }
};

void BnwWorldViewControlTree::register_root_context_child_name588_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<ContextRootChildAttempt>& input,const std::shared_ptr<ContextChildControlsFrame>& lexical){
 using Registration=ContextChildName588Registration;
 using Receiver=detail::NotificationLayoutOrdinaryReceiver;
 const auto a=input;const auto frame=lexical;
 require(a&&frame&&a->controls_export_operand&&a->controls_export_return,
  "515120 requires genuine live4FD460 and actual returned664FB0/6768B0");
 frame->full();
 const auto export_loan=a->controls_export_operand;const auto exported=a->controls_export_return;
 const auto export_guard=exported->capture_guard_without_callbacks(operation,*export_loan);
 const auto read=a->parent;read.require_source_owned(operation);
 const auto root=read.impl_->evidence->core.lock();const auto parent=frame->parent_context;
 const auto constructor=parent->impl_->native_constructor_return;
 require(root&&root==core_&&root!=a->child_core&&constructor&&constructor->impl_&&
  constructor->impl_->port&&constructor->impl_->port->map588,
  "515120 real child3C parent OWN40F/map588 absent");
 constructor->impl_->port->require_completed();
 auto*const map=&*constructor->impl_->port->map588;
 const auto id=frame->id58;const auto receiver=a->receiver;
 // 515131 strlen ->838C90(seedFFFFFFFF) over the SAME terminated ID bytes.
 // Existing raw FString kernel uses EDB88320 and has no basename/uppercase
 // normalization or final XOR. The positive ASCII wrapper guarantees no NUL.
 const auto hash=content::saved_info_type_hash(std::string_view(id->bytes.data(),id->length));
 const auto old=map->cells.find(hash);
 const bool absent=old==map->cells.end();
 const auto old_value=absent?std::shared_ptr<const Receiver>{}:old->second;
 const auto before_count=map->cells.size();const auto before_revision=map->revision;
 bool inserted=false,stored=false;
 std::shared_ptr<const Registration> returned;
 const auto first=[&]{
  frame->first();export_guard.require_current_without_callbacks(operation,*export_loan);
  require(a->controls_export_operand==export_loan&&a->controls_export_return==exported&&
   a->name588_registration==returned&&frame->id58==id&&a->receiver==receiver&&
   root->context_ordinary_state==parent&&parent->impl_->native_constructor_return==constructor&&
   constructor->impl_->alive.load()&&!constructor->impl_->failed.load()&&constructor->impl_->port&&
   constructor->impl_->port->map588&&&*constructor->impl_->port->map588==map&&
   map->cells.size()==before_count+(inserted?1u:0u)&&
   map->revision==before_revision+(inserted?1u:0u)+(stored?1u:0u),
   "515120 real entered parent/map588 preimage or earned stage changed");
  constructor->impl_->port->require_completed();
  const auto current=map->cells.find(hash);
  require((absent&&!inserted)?current==map->cells.end():
   (current!=map->cells.end()&&current->second==(stored?receiver:old_value)),
   "515120 actual numeric cell changed across callback/allocation");
 };
 const auto full=[&]{first();frame->full();first();export_guard.require_source_owned(operation,*export_loan);first();};
 full();require(before_revision<=UINT64_MAX-(absent?2u:1u),"515120 live map mutation journal exhausted");
 //59EBD0 creates a realNULL cell before515162. Allocate a detached node of
 // the SAME map/allocator type, FULL after new_handler, then publish without
 // allocation; never borrow a std::map iterator/reference across callbacks.
 if(absent){
  context_constructor_suffix_detail::NamedReceiverMap588::Cells detached;
  detached.emplace(hash,std::shared_ptr<const Receiver>{});full();
  auto node=detached.extract(hash);full();
  const auto result=map->cells.insert(std::move(node));
  require(result.inserted&&result.node.empty(),"515120 first59EBD0 sameNULL insertion did not return");
  inserted=true;++map->revision;full();
 }
 // The actual first lookup has returned. Native CMP value,0 controls the
 // second59EBD0/store branch. Occupied keys (including CRC collisions) win.
 first();const bool native_null=map->cells.find(hash)->second==nullptr;
 if(native_null){
  full();auto second=map->cells.find(hash); // same59EBD0 existing-cell hit, no allocation
  require(second!=map->cells.end()&&!second->second,"515120 required second sameNULL lookup failed");
  second->second=receiver;stored=true;++map->revision;full();
 }
 const auto selected=map->cells.find(hash)->second;require(selected!=nullptr,"515120 returned nativeNULL unexpectedly");
 full();
 auto result=std::shared_ptr<const Registration>(new Registration(a,read,constructor,id,selected,map,hash,
  before_count,map->cells.size(),before_revision,map->revision,inserted,stored));full();
 // Actual515179..51517B return only. 4FD4A7/78 and wholev64 still have their
 // genuine ordered effects; no caller can construct/convert this private DATA.
 returned=result;a->name588_registration=returned;first();full();returned->first();
}
