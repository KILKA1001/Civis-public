 // Genuine inner806950 continuation, called only after actual metadata/context.
 void* inner_slot28=nullptr;
 float inner88=0;
 std::vector<std::int32_t> pending_peer_removals;
 bool inner_post_metadata_entered=false,inner_post_metadata_returned=false;
 struct Heartbeat1004 {
  std::int32_t offset18=0,offset14=0,offset1c=0;
  std::uint32_t type=0,offset08=0,size=0,offset20=0,offset24=0,parts=0,descriptor=0;
  bool offset2c=false,offset04=false,offset05=false,constructed=false;
  bool submit_entered=false,submit_returned=false,retired=false;
 };
 std::unique_ptr<Heartbeat1004> partial_heartbeat;
 // Retains earned native constructor/message/queue history between actual
 // later heartbeat attempts; never reset a failed partial message.
 std::vector<std::unique_ptr<Heartbeat1004>> returned_heartbeat_history;
 std::int32_t heartbeat_target=-1;
 void complete_inner_post_metadata_owned(){
  current();need(caller->active_update_loan_&&actual_post_context_returned&&
   context&&context->mode==0&&inner_update_depth>0&&!inner_post_metadata_entered,
   "SP inner suffix lacks genuine current returned context/metadata or reentered");
  caller->active_update_loan_->require_current_without_callbacks();inner_post_metadata_entered=true;
  //8069CD obtains actual info20 twice before7F2BF0. Retained info's ID
  // is from8051BE, and both maps are real native constructors/registration.
  need(info_parent_id==registry_id,"SP inner current info20 supplier replaced");
  const auto retained=process->registry.find(info_parent_id);
  need(retained!=process->registry.end()&&retained->second==this,
   "SP inner native7F2BF0 parent lookup absent");
  for(std::int32_t actor=0;actor<64;++actor){
   const auto found=caller->participants_.find(actor);
   if(found==caller->participants_.end()||!found->second)continue;
   const auto& actual=*found->second;
   // Original info+14 greater-than-zero enters real timeout/isHost/remove
   // effects. The current real+9C setter wrote0; no missing-API inference.
   if(info_rate>0)throw std::runtime_error("SP info real timeout/remove effects mandatory");
   // Native+CC reads context7C through inner14 (not a separate service mode).
   // SAME mode0 constructor/start writes are retained. Mode2 skips heartbeat.
   if(context->mode!=2){
    const auto elapsed=static_cast<long double>(inner_latency)-static_cast<long double>(actual.latency);
    if(elapsed>5.0L){
     // Original7E65C0 exact specialised ctor order, after real time gate.
     // Own every partial store before required inner+44 targeted sender.
     need(!partial_heartbeat,"SP actual heartbeat constructor replay");
     partial_heartbeat=std::make_unique<Heartbeat1004>();auto& m=*partial_heartbeat;
     store(m.offset18,std::int32_t{-1});store(m.offset14,std::int32_t{-1});store(m.offset1c,std::int32_t{-1});
     store(m.type,std::uint32_t{1004});store(m.offset08,std::uint32_t{0});store(m.size,std::uint32_t{0});
     store(m.offset20,std::uint32_t{0});store(m.offset24,std::uint32_t{0});store(m.parts,std::uint32_t{1});
     store(m.offset2c,false);store(m.offset04,true);store(m.offset05,true);store(m.descriptor,std::uint32_t{0xb72da8});
     m.constructed=true;heartbeat_target=actor;current();
     submit_actual_heartbeat1004_owned();
     need(m.submit_returned&&!m.retired,"SP real1004 targeted submission failed to return");
     // Original temporary stack message has no post-send gameplay effect.
     // Retain paid history in owned storage, without a native clear/ACK.
     returned_heartbeat_history.push_back(std::move(partial_heartbeat));
    }
   }
   caller->active_update_loan_->require_current_without_callbacks();current();
  }
  //8069EE/6A10/6A40 read original80524B/24E/251 ctorNULL slots in order.
  if(inner_slot28)throw std::runtime_error("SP inner28 real7F9630 assembly Update required");
  if(connector)throw std::runtime_error("SP inner24 real7FCF10 connector state-machine required");
  if(optional_component)throw std::runtime_error("SP inner30 real virtual0C Update required");
  // Actual805262 retained float88=-1 gives native806A68 skip of disconnect
  // timer before all diagnostic/Game/client callbacks. Nonpositive includes
  // zero/negative; a changed positive value must execute its real suffix.
  if(inner88>0)throw std::runtime_error("SP original positive inner88 disconnect/client effects mandatory");
  //8086C0 final peer-removal loop reads actual ctor-empty vector. A nonempty
  // vector invokes real inner68 then erases; no fake successful omission.
  if(!pending_peer_removals.empty())throw std::runtime_error("SP inner peer-removal68/erase required");
  caller->active_update_loan_->require_current_without_callbacks();current();
  store(inner_update_depth,inner_update_depth-std::uint32_t{1}); //806D2B only now
  inner_post_metadata_returned=true;current();
 }
