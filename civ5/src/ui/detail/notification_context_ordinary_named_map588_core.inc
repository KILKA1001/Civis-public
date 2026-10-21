// Actual ordinary4FD485 true flag and already parsed nonNULL3C reaches515120.
// Same raw CRC and first-NULL/store semantics as the proven native body.
struct BnwWorldViewControlTree::RootOrdinaryName588Registration final {
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<RootOrdinaryControlsFrame> frame;
 const std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> constructor;
 context_constructor_suffix_detail::NamedReceiverMap588*const map;
 const std::shared_ptr<const detail::NotificationLayoutControlID58> id58;
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> value;
 const std::uint32_t key;
 const std::size_t before_count,after_count;
 const std::uint64_t before_revision,after_revision;
 const bool inserted_null,stored_receiver;
 RootOrdinaryName588Registration(std::weak_ptr<RootOrdinaryControlsFrame> f,
  std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> c,
  context_constructor_suffix_detail::NamedReceiverMap588* m,
  std::shared_ptr<const detail::NotificationLayoutControlID58> id,
  std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> selected,
  std::uint32_t hash,std::size_t n,std::size_t a,std::uint64_t q,std::uint64_t r,bool inserted,bool stored)
  :frame(std::move(f)),constructor(std::move(c)),map(m),id58(std::move(id)),value(std::move(selected)),key(hash),
   before_count(n),after_count(a),before_revision(q),after_revision(r),inserted_null(inserted),stored_receiver(stored){}
 void first()const {physical(true);}
 void after_scope_without_callbacks()const {physical(false);}
 void physical(bool live)const {
  require(creator==std::this_thread::get_id(),"ordinary515120 foreign returned registration");
  const auto f=frame.lock();require(f!=nullptr,"ordinary515120 actual frame retired");f->physical_without_callbacks(live);
  const auto actual=f->core.lock();require(actual&&actual->context_ordinary_state&&actual->context_ordinary_state->impl_&&
   f->name588_registration.get()==this&&constructor&&constructor->impl_&&
   constructor->impl_->alive.load()&&!constructor->impl_->failed.load()&&constructor->impl_->port&&
   constructor->impl_->port->map588&&&*constructor->impl_->port->map588==map&&
   actual->context_ordinary_state->impl_->native_constructor_return==constructor&&f->id58==id58&&
   id58&&id58->length&&id58->length<id58->bytes.size()&&id58->bytes[id58->length]=='\0',
   "ordinary515120 actual physical parent constructor/map/ID58 changed");
  constructor->impl_->port->require_completed();
  require(key==content::saved_info_type_hash(std::string_view(id58->bytes.data(),id58->length))&&
   after_count==before_count+(inserted_null?1u:0u)&&
   after_revision==before_revision+(inserted_null?1u:0u)+(stored_receiver?1u:0u)&&
   map->cells.size()>=after_count&&map->revision>=after_revision&&value&&(!stored_receiver||value==f->receiver),
   "ordinary515120 actual first-NULL/store mutation journal changed");
  const auto current=map->cells.find(key);require(current!=map->cells.end()&&current->second==value,
   "ordinary515120 first-wins numeric receiver cell changed");f->physical_without_callbacks(live);
 }
};
void BnwWorldViewControlTree::register_root_ordinary_name588_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<RootOrdinaryControlsFrame>& frame,
 const std::shared_ptr<const detail::NotificationContextRootOrdinaryControlsExportLoan>& loan,
 const std::shared_ptr<const detail::NotificationContextRootOrdinaryControlsExportReturn>& exported){
 using Registration=RootOrdinaryName588Registration;using Receiver=detail::NotificationLayoutOrdinaryReceiver;
 require(frame&&loan&&exported&&!frame->name588_registration,"ordinary515120 actual returned +7C or live frame absent/replayed");
 frame->full();const auto eg=exported->capture_guard_without_callbacks(operation,*loan);eg.require_source_owned(operation,*loan);
 const auto actual=frame->core.lock();require(actual&&actual==core_&&frame->realm&&frame->realm->impl_&&
  frame->realm->impl_->context.lock()==actual,"ordinary515120 physical nonNULL3C parent differs");
 const auto parent=actual->context_ordinary_state;require(parent&&parent->impl_,"ordinary515120 real parent Context absent");
 const auto constructor=parent->impl_->native_constructor_return;
 require(constructor&&constructor->impl_&&constructor->impl_->port&&constructor->impl_->port->map588,
  "ordinary515120 completed parent OWN40F map588 absent");constructor->impl_->port->require_completed();
 auto*const map=&*constructor->impl_->port->map588;const auto id=frame->id58;const auto receiver=frame->receiver;
 const auto key=content::saved_info_type_hash(std::string_view(id->bytes.data(),id->length));
 const auto previous=map->cells.find(key);const bool absent=previous==map->cells.end();
 const auto old_value=absent?std::shared_ptr<const Receiver>{}:previous->second;
 const auto before_count=map->cells.size();const auto before_revision=map->revision;
 bool inserted=false,stored=false;std::shared_ptr<const Registration> returned;
 const auto first=[&]{frame->first();eg.require_current_without_callbacks(operation,*loan);
  require(frame->operation==&operation&&frame->name588_registration==returned&&frame->id58==id&&frame->receiver==receiver&&
   actual->context_ordinary_state==parent&&parent->impl_->native_constructor_return==constructor&&
   constructor->impl_->alive.load()&&!constructor->impl_->failed.load()&&constructor->impl_->port&&
   constructor->impl_->port->map588&&&*constructor->impl_->port->map588==map&&
   Core::node(actual->nodes,frame->id).layout_baseline->base_.context_realm_3c==frame->realm&&
   map->cells.size()==before_count+(inserted?1u:0u)&&
   map->revision==before_revision+(inserted?1u:0u)+(stored?1u:0u),
   "ordinary515120 actual parsed3C/map preimage or earned stage changed");
  constructor->impl_->port->require_completed();const auto current=map->cells.find(key);
  require((absent&&!inserted)?current==map->cells.end():
   current!=map->cells.end()&&current->second==(stored?receiver:old_value),
   "ordinary515120 actual numeric cell changed across callback/allocation");
 };
 const auto full=[&]{first();frame->full();first();eg.require_source_owned(operation,*loan);first();};
 full();require(before_revision<=UINT64_MAX-(absent?2u:1u),"ordinary515120 journal exhausted");
 if(absent){
  context_constructor_suffix_detail::NamedReceiverMap588::Cells detached;
  detached.emplace(key,std::shared_ptr<const Receiver>{});full();auto cell=detached.extract(key);full();
  const auto inserted_cell=map->cells.insert(std::move(cell));
  require(inserted_cell.inserted&&inserted_cell.node.empty(),"ordinary515120 actual first NULL lookup insertion failed");
  inserted=true;++map->revision;full();
 }
 first();const bool native_null=map->cells.find(key)->second==nullptr;
 if(native_null){
  full();auto cell=map->cells.find(key);require(cell!=map->cells.end()&&!cell->second,
   "ordinary515120 actual second sameNULL lookup failed");cell->second=receiver;stored=true;++map->revision;full();
 }
 const auto selected=map->cells.find(key)->second;require(selected!=nullptr,"ordinary515120 returned nativeNULL unexpectedly");full();
 auto result=std::shared_ptr<const Registration>(new Registration(frame,constructor,map,id,selected,key,
  before_count,map->cells.size(),before_revision,map->revision,inserted,stored));full();
 returned=result;frame->name588_registration=returned;first();full();returned->first();
}
