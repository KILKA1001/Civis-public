 // Actual CRT1004 heap supplier7E6530, baseRead7E51F0, pure Process
 //80CA80, then2c and real scalar-delete7E9450 descriptor/free order.
 std::shared_ptr<Heartbeat1004> received_heartbeat1004;
 std::vector<Heartbeat1004> decoded_heartbeat1004_history; // values only, no live message alias
 std::uint64_t actual_heartbeat1004_message_frees=0;
 void receive_actual_heartbeat1004_after_factory_owned(std::int32_t participant_id,std::uint32_t before){
  current();need(caller->active_update_loan_&&decoder_entered&&!decoder_returned&&
   partial_frame_drain&&partial_frame_drain->receiver_entered&&!partial_frame_drain->receiver_returned&&
   receiving_stream&&receiving_stream->constructed&&!receiving_stream->destroyed&&!received_heartbeat1004,
   "SP1004 Read lacks genuine current type lookup/actual supplier loan");
  caller->active_update_loan_->require_current_without_callbacks();auto& s=*receiving_stream;
  // Selected managed allocator is the real30-byte logical factory instance;
  // no original pool/cache ABI receipt. Own source fields in actual order.
  received_heartbeat1004=std::make_shared<Heartbeat1004>();auto& m=*received_heartbeat1004;
  store(m.offset08,std::uint32_t{0});store(m.size,std::uint32_t{0});
  store(m.offset20,std::uint32_t{0});store(m.offset24,std::uint32_t{0});store(m.offset2c,false);
  store(m.type,std::uint32_t{1004});store(m.offset18,std::int32_t{-1});
  store(m.offset14,std::int32_t{-1});store(m.offset1c,std::int32_t{-1});
  store(m.parts,std::uint32_t{1});store(m.offset04,true);store(m.offset05,true);
  store(m.descriptor,std::uint32_t{0xb72da8});m.constructed=true;current();
  // B72DA8+C ->7E93B0 JMPbase7E51F0; current cursor follows type.
  (void)received_word();received_word_into(m.parts);received_word_into(m.offset18);
  received_word_into(m.offset14);received_word_into(m.offset20);received_word_into(m.offset24);
  store(m.offset08,std::uint32_t{0});store(m.size,s.write_position-before);decoder_returned=true;
  // ReadStream descriptor/destructor BEFORE any live adapter/participant effect.
  store(s.descriptor,std::uint32_t{0xb76290});s.destroyed=true;current();
  caller->active_update_loan_->require_current_without_callbacks();
  const auto participant=caller->auxiliary_participants_.find(0);
  need(participant!=caller->auxiliary_participants_.end()&&participant->second&&
   participant->second->id==participant_id,"SP1004 actual auxiliary participant alias changed");
  //807E71 +134 service getter precedes7FDBD0. Native registry lookup
  //reads actual message18; negative takes the pure early return beforeC4.
  const auto forwarded=process->registry.find(registry_id);
  need(forwarded!=process->registry.end()&&forwarded->second==this,
   "SP1004 original forwarded service registry changed");
  if(m.offset18>=0)throw std::runtime_error("SP1004 original7FDBD0 forwarded C4/targeted-send required");
  //807E8C repeats service getter before7FDC70; its real local-ID getter
  //is evaluated even for negative message18. Read the retained boot field.
  const auto routed=process->registry.find(registry_id);
  need(routed!=process->registry.end()&&routed->second==this,
   "SP1004 original routed service registry changed");
  const auto actual_local=caller->local_id_;current();
  need(actual_local>=0&&actual_local<64,"SP1004 actual registered local-ID absent");
  if(m.offset18==actual_local)throw std::runtime_error("SP1004 original7FDC70 routed participant effects required");
  store(m.offset1c,participant->second->id);store(participant->second->interval,inner_latency);
  need(!optional_component,"SP1004 actual optional component Update required");
  need(m.offset04&&m.type>=1000&&!m.offset2c,"SP1004 original queued variant mandatory");
  // Genuine direct specialised Process slot10 is80CA80 RET4, no Game/UI
  // mutation or callback. Original m2c writes only after that real return.
  current();caller->active_update_loan_->require_current_without_callbacks();
  store(m.offset2c,true);current();
  // Native80C804 pushes1; scalar-delete7E9450 FIRST stores base descriptor
  //B72614, THEN81CD70 actually frees. Value-only paid history holds no strong
  //message alias. Do not report free while a heap owner still remains.
  store(m.descriptor,std::uint32_t{0xb72614});m.retired=true;
  decoded_heartbeat1004_history.push_back(m);
  const std::weak_ptr<Heartbeat1004> retirement=received_heartbeat1004;
  received_heartbeat1004.reset();
  need(retirement.expired(),"SP1004 original scalar-delete still has strong message alias");
  increment(actual_heartbeat1004_message_frees);current();caller->active_update_loan_->require_current_without_callbacks();
  partial_frame_drain->receiver_returned=true;
 }
