 // Source22 frame consumer is reached only through canonical LocalClient and
 // the real active renderer/time loan. It cannot be invoked by a save receipt.
 struct FrameDrain final {
  std::shared_ptr<Context::Fragment> fragment;
  bool popped=false,receiver_entered=false,receiver_returned=false;
 };
 std::unique_ptr<FrameDrain> partial_frame_drain;
 std::uint64_t actual_frame_payload_frees=0,actual_frame_fragment_frees=0;
 bool second_frame_pop_entered=false;
 bool actual_post_context_entered=false,actual_post_context_returned=false;
 void update_post_context_metadata_owned(){
  current();need(caller->active_update_loan_&&context&&context->queue.size==0&&!partial_frame_drain&&
   native_metadata&&native_metadata->returned&&!actual_post_context_entered,
   "SP metadata suffix requires actual empty context normal return, no replay");
  caller->active_update_loan_->require_current_without_callbacks();actual_post_context_entered=true;
  auto& m=*native_metadata;
  //7FE59E/7FE5B6 repeats actual service registry lookup AFTER context. Then
  //7FE650 obtains SAME inner once more before touching the real observer.
  const auto again=process->registry.find(m.parent_id);
  need(again!=process->registry.end()&&again->second==this,
   "SP post-context native inner disappeared");current();
  const auto supplier=process->registry.find(m.parent_id);
  need(supplier!=process->registry.end()&&supplier->second==this&&m.observer&&
   m.observer->descriptor==0xb7506c,"SP metadata genuine Observer/inner supplier absent");
  //809490 reads the ctor-retained actual+4 head FIRST. Zero returnsNULL;
  // do not call a guessed pending-message pop or manufacture a message ACK.
  if(m.observer->value!=0)throw std::runtime_error("SP metadata real observer node destruction/dispatch mandatory");
  if(m.offset34)throw std::runtime_error("SP metadata native7FE700 discrete division/sequence branch mandatory");
  need(std::fegetround()==FE_TONEAREST,"SP post-context floating environment changed");
  //7FE73B performs extended subtraction and compares directly with float
  // rate. >= excludes unordered just as native FCOMP carry/7FE746 does.
  const bool gate=(static_cast<long double>(m.offset10)-static_cast<long double>(m.offset0c))>=
   static_cast<long double>(metadata_rate);
  if(gate){
   require_before_registration();current();
   const auto sequence=m.offset24;
   for(const auto& [id,actual]:caller->participants_){
    (void)id;
    if(actual&&actual->flag19&&sequence>static_cast<std::uint32_t>(actual->id)){
     //7FE6CF returnsNULL before second809490 and offset28 store.
     actual_post_context_returned=true;current();return;
    }
   }
   caller->active_update_loan_->require_current_without_callbacks();
   //7FE6DD re-reads actual Observer, even though no message was produced.
   need(m.observer&&m.observer->descriptor==0xb7506c,"SP metadata Observer retired before second read");
   if(m.observer->value!=0)throw std::runtime_error("SP metadata newly real queued message mandatory");
   store(m.offset28,m.offset24); //7FE6E9 equal store also when NULL message
  }
  caller->active_update_loan_->require_current_without_callbacks();current();actual_post_context_returned=true;
 }
 void update_local_mode0_prefix(float seconds){
  current();require_before_registration();
  need(caller->registration_returned_&&inner_returned&&service_returned&&
       native_metadata&&native_metadata->returned&&context,
       "SP frame requires SAME completed boot/metadata/context");
  need(!partial_frame_drain&&inner_update_depth==0,"SP frame cannot replay a failed original pop/inner Update");
  if(actual_post_context_entered){
   need(actual_post_context_returned&&inner_post_metadata_returned,
    "SP next typed frame cannot clear failed original metadata/inner history");
   // New genuine renderer Loan after prior normal inner return. These flags
   // are private borrow bookkeeping only; no native field or paid history reset.
   actual_post_context_entered=false;actual_post_context_returned=false;
   inner_post_metadata_entered=false;inner_post_metadata_returned=false;
   // New typed renderer Loan after normal prior inner return opens a new
   // actual context borrow. Only private per-pop/decoder flags retire; no
   // native stores, queue contents, processed history or failed state reset.
   second_frame_pop_entered=false;decoder_entered=false;decoder_returned=false;
  }
  // Native80696A increments inner+60 BEFORE its +14C getter. These fields
  // have explicit original8050A4/8050A7/80525A constructor stores below.
  store(inner_update_depth,inner_update_depth+std::uint32_t{1});
  if(inner45){
   throw std::runtime_error("SP frame mandatory STOP at original inner+6C conditional callback");
  }
  // Two float operands have an exact sum in binary64; the float store gives
  // the source x87 FSTP32 result under the selected nearest rounding mode.
  need(std::fegetround()==FE_TONEAREST,"SP frame required floating environment changed");
  store(inner_latency,static_cast<float>(static_cast<double>(inner_latency)+seconds));
  auto& metadata=*native_metadata;
  store(metadata.offset10,static_cast<float>(static_cast<double>(metadata.offset10)+seconds));
  const auto actual=process->registry.find(metadata.parent_id);
  need(actual!=process->registry.end()&&actual->second==this,
       "SP metadata Update registry no longer supplies actual inner");current();
  // Native inner+FC/808330 calls8019E0 with the actual retained context.
  need(context->mode==0&&!context->udp&&!context->steam&&context->proxy==this,
       "SP frame mode0 context required; transport cannot be guessed absent");
  auto& queue=context->queue;
  need(queue.initialized&&queue.proxy==&queue&&queue.map&&queue.map_capacity==8,
       "SP frame actual local fragment queue absent");
  if(queue.size==0){
   update_post_context_metadata_owned();complete_inner_post_metadata_owned();return;
  }
  need(queue.offset<queue.map_capacity*4,"SP frame local queue offset invalid");
  const auto& block=(*queue.map)[queue.offset/4];
  need(block!=nullptr,"SP frame original deque block unavailable");
  auto drain=std::make_unique<FrameDrain>();
  // Approved managed-storage port: transfer the live node's strong owner
  // from the inactive physical slot into the actual popped FrameDrain.
  // This is C++ ownership bookkeeping, no extra original native store. The
  // original stale pointer bytes are not observable logical queue state.
  drain->fragment=std::move((*block)[queue.offset%4]);partial_frame_drain=std::move(drain);
  // Original8023? retains the element then mutates offset/size BEFORE calling
  // receiver. The physical block slot is left unchanged by the original pop.
  store(queue.offset,queue.offset+std::uint32_t{1});
  if(queue.offset>=queue.map_capacity*4)store(queue.offset,std::uint32_t{0});
  store(queue.size,queue.size-std::uint32_t{1});
  if(queue.size==0)store(queue.offset,std::uint32_t{0});
  partial_frame_drain->popped=true;current();
  auto fragment=partial_frame_drain->fragment;
  if(!fragment||!context->receiver){
   // Optional native receiver branch is honored; fragment teardown and the
   // next context/metadata step still need their actual producers.
   throw std::runtime_error("SP frame mandatory STOP at original fragment teardown without receiver");
  }
  auto& receiver=*context->receiver;
  need(inner_receiver_adapter&&context->receiver==inner_receiver_adapter.get()&&
       receiver.constructed&&receiver.descriptor==0xb75168&&
       receiver.parent_registry_id==registry_id&&fragment->constructed&&fragment->queued,
       "SP frame actual receiver adapter/fragment lineage differs");
  partial_frame_drain->receiver_entered=true;
  const auto parent=process->registry.find(receiver.parent_registry_id);
  need(parent!=process->registry.end()&&parent->second==this,
       "SP adapter receiver registry cannot lend original inner");
  const auto participant=caller->auxiliary_participants_.find(0);
  const auto participant_id=participant!=caller->auxiliary_participants_.end()&&participant->second?
   participant->second->id:std::int32_t{-1};
  // Required actual adapter ReadStream/factory dispatch, not a queue ACK.
  receive_control1208_prefix(participant_id);
  current();
  need(partial_frame_drain->receiver_returned&&decoder_returned&&!received_control,
   "SP fragment teardown requires CURRENT genuine typed factory/Read/adapter return");
  //802431 optional destruction callback is read FIRST. Original7FF20E
  // stores0 here; only its genuine NULL branch is supported by this profile.
  need(fragment->offset10==0,"SP original nonnull fragment callback required");
  if((fragment->flags&4)==0&&fragment->payload){
   fragment->payload.reset();++actual_frame_payload_frees; //80244C
  }
  const std::weak_ptr<Context::Fragment> first_retirement=fragment;
  partial_frame_drain->fragment.reset();fragment.reset(); //802456 actual last strong owner release
  need(first_retirement.expired(),"SP original first fragment still strongly owned after destructor");
  ++actual_frame_fragment_frees;
  partial_frame_drain.reset();current(); // original loop reads size only NOW
  need(!second_frame_pop_entered,"SP second actual context pop cannot replay");
  if(queue.size==0){
   // Genuine Control-only batch ended after its actual first node free.
   update_post_context_metadata_owned();complete_inner_post_metadata_owned();return;
  }
  second_frame_pop_entered=true;
  need(queue.offset<queue.map_capacity*4,"SP next actual queue offset invalid");
  const auto& next_block=(*queue.map)[queue.offset/4];
  need(next_block!=nullptr,"SP actual next deque block unavailable");
  auto next_drain=std::make_unique<FrameDrain>();
  next_drain->fragment=std::move((*next_block)[queue.offset%4]);partial_frame_drain=std::move(next_drain);
  //8023F7/408/410: all native pop stores precede actual second receiver call.
  store(queue.offset,queue.offset+std::uint32_t{1});
  if(queue.offset>=queue.map_capacity*4)store(queue.offset,std::uint32_t{0});
  store(queue.size,queue.size-std::uint32_t{1});
  if(queue.size==0)store(queue.offset,std::uint32_t{0});
  partial_frame_drain->popped=true;current();fragment=partial_frame_drain->fragment;
  need(fragment&&context->receiver==inner_receiver_adapter.get()&&
   receiver.constructed&&receiver.parent_registry_id==registry_id&&fragment->constructed&&fragment->queued,
   "SP second actual fragment/receiver lineage absent");
  partial_frame_drain->receiver_entered=true;
  const auto next_parent=process->registry.find(receiver.parent_registry_id);
  need(next_parent!=process->registry.end()&&next_parent->second==this,
   "SP next adapter SAME parent replaced");
  const auto next_participant=caller->auxiliary_participants_.find(0);
  need(next_participant!=caller->auxiliary_participants_.end()&&next_participant->second,
   "SP next actual auxiliary participant absent");
  receive_client155_prefix(next_participant->second->id);current();
  need(partial_frame_drain->receiver_returned&&fragment&&!received_client155,
   "SP second fragment teardown before actual155 receiver/destructor return");
  need(fragment->offset10==0,"SP second fragment actual nonnull destruction callback mandatory");
  if((fragment->flags&4)==0&&fragment->payload){
   fragment->payload.reset();++actual_frame_payload_frees;
  }
  const std::weak_ptr<Context::Fragment> second_retirement=fragment;
  partial_frame_drain->fragment.reset();fragment.reset();
  need(second_retirement.expired(),"SP original second fragment still strongly owned after destructor");
  ++actual_frame_fragment_frees;partial_frame_drain.reset();current();
  // Source-specific initial1208+155 batch. Future queued message types must
  // execute their real native receiver, not get silently drained or ACKed.
  if(queue.size!=0)throw std::runtime_error("SP subsequent actual native queued message receiver mandatory");
  // Complete the actual second-message batch before inner continuation.
  update_post_context_metadata_owned();complete_inner_post_metadata_owned();
 }
