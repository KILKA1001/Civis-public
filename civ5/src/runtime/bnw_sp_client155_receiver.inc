 // Source28 actual second local queued message. No EndTurn or readiness
 // receipt can construct a receiver; only SAME actual pop/CRT155 supplier.
 std::shared_ptr<Client155> received_client155;
 bool client155_decoder_entered=false,client155_decoder_returned=false;
 bool client155_value_read=false,client155_dispatch_entered=false;
 void receive_client155_prefix(std::int32_t participant_id){
  current();need(caller->active_update_loan_&&second_frame_pop_entered&&partial_frame_drain&&
   partial_frame_drain->popped&&partial_frame_drain->receiver_entered&&!partial_frame_drain->receiver_returned&&
   partial_frame_drain->fragment&&control_message_destroyed&&control_process_returned&&!received_control&&
   !client155_decoder_entered&&!received_client155,
   "SP155 Read requires genuine second pop and destroyed first Control, no replay");
  caller->active_update_loan_->require_current_without_callbacks();client155_decoder_entered=true;
  need(receiving_stream&&receiving_stream->destroyed,"SP prior actual ReadStream destructor absent");
  receiving_stream.reset(); // completed non-owning original stack owner, not a cursor/history reset
  received_stream_constructor(*partial_frame_drain->fragment);auto& s=*receiving_stream;
  need(process->client155_factory_returned&&process->assembly_constructor_returned&&
   process->partial_assemblies.empty(),"SP155 actual CRT factory/empty assembly source absent");
  const auto restore=s.write_position;received_reset();need(s.high_water>=28,"SP155 actual header incomplete");
  received_skip(4);received_skip(4);const auto parts=received_word();
  received_skip(4);received_skip(4);received_skip(4);received_skip(4);received_reset();received_skip(restore);
  need(parts==1,"SP155 original multipart source required");received_reset();
  const auto before=s.write_position;const auto type=received_word();
  const auto supplier=process->control_factory.find(static_cast<std::int32_t>(type));
  need(type==155&&supplier!=process->control_factory.end()&&supplier->second==Process::FactoryKind::client155,
   "SP second actual message requires unknown native supplier");
  received_client155=std::make_shared<Client155>();auto& m=*received_client155;
  //69CE10 heap supplier calls original7E4C60 then true/descriptor/actor-1.
  store(m.type,std::uint32_t{155});store(m.descriptor,std::uint32_t{0xb72614});
  store(m.offset04,false);store(m.offset05,false);store(m.offset08,std::uint32_t{0});
  store(m.size,std::uint32_t{0});store(m.offset18,std::int32_t{-1});store(m.offset14,std::int32_t{-1});
  store(m.offset1c,std::int32_t{-1});store(m.offset20,std::uint32_t{0});store(m.offset24,std::uint32_t{0});
  store(m.parts,std::uint32_t{1});store(m.offset2c,false);
  store(m.offset04,true);store(m.descriptor,std::uint32_t{0xa85314});store(m.actor,std::int32_t{-1});
  // Supplier does NOT initialize byte34. No read/license from C++ default:
  // actual6D2900/833FA0 writes it before client155_value_read can become true.
  m.constructed=true;(void)received_word();received_word_into(m.parts);
  received_word_into(m.offset18);received_word_into(m.offset14);received_word_into(m.offset20);
  received_word_into(m.offset24);received_word_into(m.actor);
  need(s.cursor<s.high_water,"SP155 original boolean read overflow formatter required");
  const auto actual_byte=s.bytes[s.cursor];
  need(actual_byte<=1,"SP155 noncanonical native bool storage unsupported");
  store(m.value,actual_byte!=0);increment(s.copies);
  store(s.cursor,s.cursor+std::uint32_t{1});store(s.read_position,s.read_position+std::uint32_t{1});
  store(s.write_position,s.write_position+std::uint32_t{1});client155_value_read=true;
  store(m.offset08,std::uint32_t{0});store(m.size,s.write_position-before);client155_decoder_returned=true;
  store(s.descriptor,std::uint32_t{0xb76290});s.destroyed=true;current();
  const auto participant=caller->auxiliary_participants_.find(0);
  need(participant!=caller->auxiliary_participants_.end()&&participant->second&&
   participant->second->id==participant_id,"SP155 actual auxiliary participant alias changed");
  need(m.offset18==-1&&m.offset14==-1,"SP155 original forwarded/routed source required");
  store(m.offset1c,participant->second->id);store(participant->second->interval,inner_latency);
  need(!optional_component,"SP155 actual optional component Update required");
  need(m.offset04&&m.type<1000&&!m.offset2c&&client155_value_read,
   "SP155 original direct/queued variant unsupported");
  //80806A proxy50->608F10->511FF0->7E4CA0->4764E0 must execute through
  // a genuine typed canonical client before m2c/destructor/context return.
  client155_dispatch_entered=true;
  caller->active_update_loan_->client155_registration_callback_owned();
  current();
  need(caller->client155_stage_==BnwSpClientPregameStartup::Client155Stage::client_returned,
   "SP155 original Process lacks genuine canonical Utility/UI normal return");
  //7E4CB9 occurs only AFTER actual4764E0 normal return. The same message
  // then follows original adapter's vtable0 scalar delete(1),780B90:
  // descriptorB72614 BEFORE heap free. No string destructor exists for155.
  store(m.offset2c,true);current();
  store(m.descriptor,std::uint32_t{0xb72614});m.retired=true;
  received_client155.reset();current();
  caller->active_update_loan_->require_current_without_callbacks();
  partial_frame_drain->receiver_returned=true;
  // Actual popped fragment teardown and metadata are owned by frame loop;
  // this returned receiver grants neither fullframe nor EndTurn completion.
 }
