 // Concrete first mode0 Control receiver. Only actual CRT1208 supplier is
 // implemented; unknown factories and multi-part assembly remain mandatory.
 struct ReadStream final {
  const std::uint8_t* bytes=nullptr;
  std::uint32_t descriptor=0,byte_order=0,read_position=0,write_position=0,
   cursor=0,high_water=0,capacity=0,tag=0,allocator=0;
  bool prefix_flag=false,owns=false,growth_flag=false,constructed=false,destroyed=false;
  std::uint64_t copies=0;
 };
 std::unique_ptr<ReadStream> receiving_stream;
 std::shared_ptr<Control1208> received_control;
 bool decoder_entered=false,decoder_returned=false,control_dispatch_entered=false;
 std::unique_ptr<std::string> control_diagnostic;
 bool control_process_returned=false,control_message_destroyed=false;
 void received_stream_constructor(const Context::Fragment& fragment){
  current();need(!receiving_stream&&fragment.payload&&fragment.constructed&&fragment.length,
   "SP receiver actual queued payload absent or ReadStream replayed");
  receiving_stream=std::make_unique<ReadStream>();auto& s=*receiving_stream;
  // Actual838F60 source order, before non-owning8392E0 binding.
  store(s.prefix_flag,true);store(s.byte_order,std::uint32_t{1});
  store(s.read_position,std::uint32_t{0});store(s.write_position,std::uint32_t{0});
  store(s.bytes,static_cast<const std::uint8_t*>(nullptr));store(s.cursor,std::uint32_t{0});
  store(s.high_water,std::uint32_t{0});store(s.capacity,std::uint32_t{0});
  store(s.owns,false);store(s.growth_flag,false);store(s.allocator,std::uint32_t{0});
  store(s.descriptor,std::uint32_t{0xb76290});store(s.tag,std::uint32_t{2});
  // Original839350 sees actual ctor owns=false and returns without free.
  store(s.owns,false);store(s.bytes,static_cast<const std::uint8_t*>(fragment.payload.get()));
  store(s.cursor,std::uint32_t{0});store(s.capacity,fragment.length);store(s.high_water,fragment.length);
  s.constructed=true;current();
 }
 std::uint32_t received_word(){
  current();need(receiving_stream&&receiving_stream->constructed&&!receiving_stream->destroyed,
   "SP original ReadStream absent before834600");auto& s=*receiving_stream;
  need(s.bytes&&s.cursor<=s.high_water&&s.high_water-s.cursor>=4,
   "SP mandatory original read overflow/error formatter not yet supported");
  // Actual8390C0 copy, cursor +=4, read +=4, write +=4, then834600 endian.
  const auto value=std::uint32_t{s.bytes[s.cursor]}|
   (std::uint32_t{s.bytes[s.cursor+1]}<<8)|(std::uint32_t{s.bytes[s.cursor+2]}<<16)|
   (std::uint32_t{s.bytes[s.cursor+3]}<<24);
  increment(s.copies);store(s.cursor,s.cursor+std::uint32_t{4});
  store(s.read_position,s.read_position+std::uint32_t{4});store(s.write_position,s.write_position+std::uint32_t{4});
  need(process->native_stream_byte_order==1&&s.byte_order==1,
   "SP receiver actual retained byte-order source unsupported");current();return value;
 }
 template<class T> void received_word_into(T& destination){
  current();need(receiving_stream&&receiving_stream->constructed&&!receiving_stream->destroyed,
   "SP original destination ReadStream absent");auto& s=*receiving_stream;
  need(s.bytes&&s.cursor<=s.high_water&&s.high_water-s.cursor>=4,
   "SP original destination read overflow/error formatter required");
  const auto value=std::uint32_t{s.bytes[s.cursor]}|
   (std::uint32_t{s.bytes[s.cursor+1]}<<8)|(std::uint32_t{s.bytes[s.cursor+2]}<<16)|
   (std::uint32_t{s.bytes[s.cursor+3]}<<24);
  //8390C0 copies directly into the message destination BEFORE its three
  // cursor stores. A return-value read followed by assignment reverses this.
  store(destination,std::bit_cast<T>(value));increment(s.copies);
  store(s.cursor,s.cursor+std::uint32_t{4});store(s.read_position,s.read_position+std::uint32_t{4});
  store(s.write_position,s.write_position+std::uint32_t{4});
  need(process->native_stream_byte_order==1&&s.byte_order==1,
   "SP receiver actual destination byte-order unsupported");current();
 }
 void received_reset(){
  auto& s=*receiving_stream;store(s.read_position,std::uint32_t{0});
  store(s.write_position,std::uint32_t{0});store(s.cursor,std::uint32_t{0});
 }
 void received_skip(std::uint32_t amount){
  auto& s=*receiving_stream;need(s.cursor<=s.high_water&&amount<=s.high_water-s.cursor,
   "SP receiver original reserve exceeds actual queued bytes");
  store(s.read_position,s.read_position+amount);store(s.write_position,s.write_position+amount);store(s.cursor,s.cursor+amount);
 }
 void construct_received_control(){
  need(!received_control,"SP actual factory-created Control1208 cannot replay");
  received_control=std::make_shared<Control1208>();auto& m=*received_control;
  // Exact same original7E8DF0 supplier constructor, independent receiver object.
  store(m.offset18,std::int32_t{-1});store(m.offset14,std::int32_t{-1});store(m.offset1c,std::int32_t{-1});
  store(m.parts,std::uint32_t{1});store(m.offset04,true);store(m.type,std::uint32_t{1208});
  store(m.offset05,false);store(m.offset08,std::uint32_t{0});store(m.size,std::uint32_t{0});
  store(m.offset20,std::uint32_t{0});store(m.offset24,std::uint32_t{0});store(m.offset2c,false);
  store(m.descriptor,std::uint32_t{0xb72ce4});
  need(process->version_constructor_returned&&process->control_version_revision==0&&
   process->version_profile==Process::VersionProfile::selected_civis_crt_constructor,
   "SP receiver actual retained version constructor/profile absent or changed");
  m.text.assign(process->control_version);m.constructed=true;current();
 }
 void receive_control1208_prefix(std::int32_t participant_id){
  current();need(partial_frame_drain&&partial_frame_drain->popped&&partial_frame_drain->receiver_entered&&
    !partial_frame_drain->receiver_returned&&!decoder_entered&&partial_frame_drain->fragment,
    "SP Control receiver requires genuine current pop/adapter, no history replay");
  decoder_entered=true;const auto& fragment=*partial_frame_drain->fragment;
  if(receiving_stream){
   need(receiving_stream->destroyed,"SP current fragment cannot retire unfinished original ReadStream");
   receiving_stream.reset(); // only returned previous non-owning stack storage
  }
  received_stream_constructor(fragment);
  // Actual7E4D20 first consults CRT-owned5CF7E50 assembly namespace.
  need(process->control_factory_returned&&process->assembly_constructor_returned,
   "SP receiver required actual CRT factory/assembly initializer missing");
  need(process->partial_assemblies.empty(),"SP receiver existing multi-part assembly mandatory source unjoined");
  auto& s=*receiving_stream;
  // Original7E5240 preserves position, resets, checks>=28, skips two words,
  // reads parts, reserves four remaining words, resets and restores position.
  const auto restore=s.write_position;received_reset();
  need(s.high_water>=28,"SP receiver native header incomplete");
  received_skip(4);received_skip(4);const auto parts=received_word();
  received_skip(4);received_skip(4);received_skip(4);received_skip(4);
  received_reset();received_skip(restore);
  need(parts==1,"SP receiver real multi-part assembly source required");
  received_reset();const auto before=s.write_position;const auto type=received_word();
  const auto supplier=process->control_factory.find(static_cast<std::int32_t>(type));
  if(type==1004){
   need(process->heartbeat1004_factory_returned&&supplier!=process->control_factory.end()&&
    supplier->second==Process::FactoryKind::heartbeat1004,"SP1004 genuine CRT supplier absent");
   receive_actual_heartbeat1004_after_factory_owned(participant_id,before);return;
  }
  need(!control_process_returned,"SP repeated actual1208 callback needs its real separate transition");
  need(supplier!=process->control_factory.end()&&supplier->second==Process::FactoryKind::control1208,
   "SP receiver required original message factory supplier not implemented");
  construct_received_control();auto& m=*received_control;
  // Actual7E9010 invokes base7E51F0 at current position after type.
  (void)received_word(); // original version is read into temporary and discarded
  received_word_into(m.parts);received_word_into(m.offset18);received_word_into(m.offset14);
  received_word_into(m.offset20);received_word_into(m.offset24);
  //833E60 reads signed length BEFORE clear, then allocates an independent
  // temporary, Read copies bytes there BEFORE three cursor stores, assigns
  // message text by exact length and frees the temporary BEFORE size stores.
  const auto text_size=std::bit_cast<std::int32_t>(received_word());m.text.clear();
  need(text_size>=0&&s.cursor<=s.high_water&&static_cast<std::uint32_t>(text_size)<=s.high_water-s.cursor,
   "SP actual Control version length/overflow formatter source unsupported");
  if(text_size>0){
   auto temporary=std::make_unique<std::uint8_t[]>(static_cast<std::size_t>(text_size));
   const auto amount=static_cast<std::uint32_t>(text_size);
   std::copy_n(s.bytes+s.cursor,amount,temporary.get());increment(s.copies);
   store(s.cursor,s.cursor+amount);store(s.read_position,s.read_position+amount);
   store(s.write_position,s.write_position+amount);
   m.text.assign(reinterpret_cast<const char*>(temporary.get()),amount);
   temporary.reset();
  }
  need(process->version_constructor_returned&&process->control_version_revision==0,
   "SP receiver actual version-profile mutation requires source setter");
  // A decoded mismatching string is retained here. Original != belongs
  // AFTER the actual proxy24 callback, never to the Read/constructor gate.
  store(m.offset08,std::uint32_t{0});store(m.size,s.write_position-before);
  // No existing assembly iterator means7E5051 skips original cleanup branch.
  decoder_returned=true;
  // Original7E4CD0 temporary destructor before adapter dispatch. Owns=false
  // means no payload free; the actual context retains the popped fragment.
  store(s.descriptor,std::uint32_t{0xb76290});s.destroyed=true;current();
  // Actual adapter80C6D0 invokes SAME inner807E10 for ordinary type1208.
  const auto participant=caller->auxiliary_participants_.find(0);
  need(participant!=caller->auxiliary_participants_.end()&&participant->second&&
    participant->second->id==participant_id,"SP received control actual participant alias changed");
  need(m.offset18==-1&&m.offset14==-1,"SP receiver routed/forwarded message variant source required");
  store(m.offset1c,participant->second->id);
  store(participant->second->interval,inner_latency); // original equal store included
  need(!optional_component,"SP receiver real optional component Update required");
  need(m.offset04&&m.type>=1000&&!m.offset2c,"SP receiver required queued/processed message variant source unjoined");
  control_dispatch_entered=true;
  // Original7E8EA0 looks up first-map actor and sets flag18 BEFORE +24 proxy.
  const auto first=caller->participants_.find(m.offset1c);
  need(first!=caller->participants_.end()&&first->second,"SP received Control first-map actor absent");
  store(first->second->flag18,true);
  need(caller->active_update_loan_,"SP original proxy24 requires actual canonical frame Loan");
  caller->active_update_loan_->control_registration_callback_owned(m.offset1c);
  current();
  // Actual494940 still requires name/UI list8B7E30 before Control process returns.
  caller->active_update_loan_->control_proxy_optional_ui_owned();
  current();
  // Original7E8F05 constructs the diagnostic temporary in source order.
  // It is not emitted to any logger/UI. Keep the genuine selected process
  // version and actual participant userdata operand, not an invented string.
  caller->require_actual_control_callback(*caller->active_update_loan_);
  need(caller->control_stage_==BnwSpClientPregameStartup::ControlStage::client_returned&&
   !control_process_returned&&!control_message_destroyed&&!control_diagnostic,
   "SP Control process tail requires actual returned local proxy and no replay");
  need(process->version_constructor_returned&&process->control_version_revision==0,
   "SP original current version profile changed before comparison");
  control_diagnostic=std::make_unique<std::string>("Version mismatch. My Version = ");
  control_diagnostic->append(process->control_version);
  control_diagnostic->append(" Remote (");
  if(first->second->userdata)
   throw std::runtime_error("SP original nonnull participant userdata/name owner required");
  // Actual native userdataNULL branch7E8F3E chooses static char9FFFB9="".
  control_diagnostic->append("");control_diagnostic->append(") Version = ");
  control_diagnostic->append(m.text);
  // IAT9C95B4 is std::operator!=, NOT equality. Matching SAME owned version
  // genuinely takes7E8F7C->7E8FD3: no C4/isHost read or event98 call at all.
  if(m.text!=process->control_version)
   throw std::runtime_error("SP actual version mismatch requires original C4/event98 source consumer");
  control_diagnostic.reset(); //7E8FD7 actual string destructor BEFORE Process return
  current();caller->active_update_loan_->require_current_without_callbacks();
  control_process_returned=true;
  // Actual808062 sets the CURRENT decoded Control2c ONLY after Process return.
  // The queued33-byte155 remains unprocessed and supplies no completion here.
  store(m.offset2c,true);
  // Actual80C800 calls message virtual scalar-delete(1).7E8E40 destroys
  // message text FIRST, writes base descriptorB72614, then frees the object.
  {std::string original_text;original_text.swap(m.text);}
  store(m.descriptor,std::uint32_t{0xb72614});m.retired=true;
  received_control.reset();control_message_destroyed=true;
  current();caller->active_update_loan_->require_current_without_callbacks();
  // Actual80C810 adapter return is now earned. Fragment payload/free/repeat,
  //155 factory/consumer and7FE550 metadata are still mandatory subsequent work.
  partial_frame_drain->receiver_returned=true;
 }
