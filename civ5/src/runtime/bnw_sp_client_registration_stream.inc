 // Source18, only the original Control1208 registration sender. No arbitrary
 // message or UI receipt can issue this local queue effect.
 struct NativeStream final {
  std::uint32_t descriptor=0,byte_order=0,read_position=0,write_position=0,
   cursor=0,high_water=0,capacity=0,tag=0,allocator=0;
  bool prefix_flag=false,owns=false,growth_flag=false;
  std::unique_ptr<std::uint8_t[]> bytes;
  bool constructed=false,serialized=false,destroyed=false;
  std::uint64_t allocations=0,copies=0,frees=0;
 };
 std::shared_ptr<NativeStream> control_stream;
 std::uint64_t queue_bulk_zeroes=0;
 void require_control_sender()const{
  current();need(registration_control&&registration_control->constructed&&
   registration_control->submit_entered&&!registration_control->retired&&
   caller->native_callback_returned_&&!caller->registration_returned_&&
   caller->phase_==BnwSpClientPregameStartup::Phase::registration_prefix&&
   context&&context->proxy==this&&context->mode==0&&!context->udp&&!context->steam,
   "SP Control1208 requires actual original sender/callback/context, no replay or borrowed UI receipt");
 }
 void stream_constructor(){
  require_control_sender();need(!control_stream,"SP Control1208 temporary stream replay");
  control_stream=std::make_shared<NativeStream>();auto& s=*control_stream;
  // Native80613C..80617E actual stack constructor stores.
  store(s.prefix_flag,true);store(s.byte_order,std::uint32_t{1});
  store(s.read_position,std::uint32_t{0});store(s.write_position,std::uint32_t{0});
  store(s.descriptor,std::uint32_t{0xb76290});
  need(!s.bytes,"SP native stream initial NULL buffer differs");increment(native_stores);
  store(s.cursor,std::uint32_t{0});store(s.high_water,std::uint32_t{0});
  store(s.capacity,std::uint32_t{0});store(s.owns,true);store(s.growth_flag,false);
  store(s.tag,std::uint32_t{2});store(s.allocator,std::uint32_t{0});
  s.constructed=true;current();
 }
 void stream_reset(NativeStream& s){
  // Native8393B0: all three cursor stores occur even when already zero.
  store(s.read_position,std::uint32_t{0});store(s.write_position,std::uint32_t{0});
  store(s.cursor,std::uint32_t{0});
 }
 void stream_skip(NativeStream& s,std::uint32_t amount){
  // Native8391E0 reserve is cursor accounting, not a payload write.
  store(s.read_position,s.read_position+amount);store(s.write_position,s.write_position+amount);
  store(s.cursor,s.cursor+amount);
  need(s.cursor<=s.high_water,"SP actual native stream reserve exceeded serialized end");
 }
 void stream_word(NativeStream& s,std::uint32_t word){
  require_control_sender();need(s.constructed&&!s.destroyed,"SP actual stream unavailable");
  // Native834040 reads SAME process C9F444 (actual PE initialized word1).
  // Its original zero branch below publishes observed platform byte order;
  // ordinary first boot retains the earned initialized word without a write.
  if(process->native_stream_byte_order==0){
   const std::uint32_t one=1;
   const auto observed=std::bit_cast<std::array<std::uint8_t,4>>(one);
   const bool platform_little=observed[0]==1&&observed[1]==0&&observed[2]==0&&observed[3]==0;
   const bool platform_big=observed[0]==0&&observed[1]==0&&observed[2]==0&&observed[3]==1;
   need(platform_little||platform_big,"SP actual platform stream endian unsupported");
   store(process->native_stream_byte_order,platform_little?std::uint32_t{1}:std::uint32_t{2});
  }
  const auto target=s.cursor+std::uint32_t{4};
  if(target>s.capacity){
   need(s.owns,"SP actual serialized stream growth forbidden");
   const auto next=std::max(s.capacity*std::uint32_t{2},target);
   auto owned=std::make_unique<std::uint8_t[]>(next);increment(s.allocations);
   if(s.high_water){std::copy_n(s.bytes.get(),s.high_water,owned.get());increment(s.copies);}
   if(s.bytes){s.bytes.reset();increment(s.frees);}
   s.bytes=std::move(owned);increment(native_stores);store(s.capacity,next);
  }
  need(s.bytes&&target<=s.capacity,"SP actual stream owned buffer absent");
  // Stream byte order1 is wire little endian. A big-endian platform source
  // takes the original swap route before producing those same wire bytes.
  for(std::uint32_t i=0;i<4;++i)s.bytes[s.cursor+i]=static_cast<std::uint8_t>(word>>(i*8));
  increment(s.copies);store(s.cursor,target);store(s.read_position,s.read_position+std::uint32_t{4});
  store(s.write_position,s.write_position+std::uint32_t{4});
  if(s.cursor>s.high_water)store(s.high_water,s.cursor);current();
 }
 void stream_control_version_bytes(NativeStream& s,const std::string& text){
  require_control_sender();need(s.constructed&&!s.destroyed&&!text.empty()&&
   text.size()<=0xffffffffu-s.cursor,"SP actual CString payload source unavailable");
  const auto amount=static_cast<std::uint32_t>(text.size());const auto target=s.cursor+amount;
  //833D50 calls actual stream+8 once for the entire string, after its length.
  //839000 grows/copies old bytes/free/publishes new buffer BEFORE payload copy.
  if(target>s.capacity){
   need(s.owns,"SP actual version stream growth forbidden");
   const auto next=std::max(s.capacity*std::uint32_t{2},target);
   auto owned=std::make_unique<std::uint8_t[]>(next);increment(s.allocations);
   if(s.high_water){std::copy_n(s.bytes.get(),s.high_water,owned.get());increment(s.copies);}
   if(s.bytes){s.bytes.reset();increment(s.frees);}
   s.bytes=std::move(owned);increment(native_stores);store(s.capacity,next);
  }
  need(s.bytes&&target<=s.capacity,"SP actual version owned stream buffer absent");
  std::copy_n(text.data(),amount,s.bytes.get()+s.cursor);increment(s.copies);
  store(s.cursor,target);store(s.read_position,s.read_position+amount);
  store(s.write_position,s.write_position+amount);
  if(s.cursor>s.high_water)store(s.high_water,s.cursor);current();
 }
 void serialize_control1208(){
  stream_constructor();auto& s=*control_stream;auto& m=*registration_control;
  const auto before=s.read_position;
  // Actual7E8FF0→7E5180, seven source words, then833D50 CString length.
  stream_word(s,m.type);stream_word(s,1);stream_word(s,m.parts);
  stream_word(s,static_cast<std::uint32_t>(m.offset18));stream_word(s,static_cast<std::uint32_t>(m.offset14));
  stream_word(s,m.offset20);stream_word(s,m.offset24);
  need(process->version_constructor_returned&&process->control_version_revision==0&&
   m.text==process->control_version,"SP actual Control version copy/currentness changed");
  need(m.text.size()<=0x7fffffffu,"SP actual CString length outside original signed domain");
  stream_word(s,static_cast<std::uint32_t>(m.text.size()));
  if(!m.text.empty())stream_control_version_bytes(s,m.text);
  store(m.size,s.read_position-before);need(m.size>0&&mtu>0,"SP actual serialized size/fragment source absent");
  // Native7E50F0 now uses the earned nonzero size without reserialization.
  store(m.parts,(m.size-std::uint32_t{1})/mtu+std::uint32_t{1});
  // Actual7E52E0: preserve position, reset, reserve two words, patch parts
  // through the actual non-NULL EDI pointer; NULL stack pointer reserves its
  // word, then two more reserves, reset again, restore former position.
  const auto restore_position=s.write_position;stream_reset(s);
  stream_skip(s,4);stream_skip(s,4);stream_word(s,m.parts);
  stream_skip(s,4);stream_skip(s,4);stream_skip(s,4);
  stream_reset(s);stream_skip(s,restore_position);
  need(s.read_position==m.size&&s.high_water==m.size,"SP Control1208 header restoration differs");
  s.serialized=true;current();
 }
 void queue_control_fragment(std::uint32_t offset,std::uint32_t amount){
  require_control_sender();auto& q=context->queue;
  need(q.initialized&&q.proxy==&q&&q.size==0&&q.offset==0&&q.map_capacity==0&&!q.map,
   "SP first native registration fragment requires genuine ctor-empty retained queue");
  need(control_stream&&control_stream->serialized&&offset+amount<=control_stream->high_water,
   "SP real fragment serialized payload unavailable");
  // Native7FF1B0 force1: allocate the20-byte logical node, allocate payload,
  // store payload, copy bytes, then flags/zero/length/zero. No playback ACK.
  auto fragment=std::make_shared<Context::Fragment>();q.partial_fragment=fragment;
  fragment->payload=std::make_unique<std::uint8_t[]>(amount);increment(native_stores);
  std::copy_n(control_stream->bytes.get()+offset,amount,fragment->payload.get());increment(fragment->copies);
  store(fragment->flags,std::uint32_t{1});store(fragment->offset00,std::uint32_t{0});
  store(fragment->length,amount);store(fragment->offset10,std::uint32_t{0});
  fragment->constructed=true;current();
  // Actual802C20 first8035F0 growth: allocate eight pointer entries, fill
  // zero, update capacity BEFORE map pointer; allocate four-element block;
  // store element pointer BEFORE incrementing size. Retain partial blocks.
  auto owned=std::make_unique<Context::Queue::Map>();q.partial_map=std::move(owned);
  for(auto& block:*q.partial_map)block.reset();increment(queue_bulk_zeroes);
  store(q.map_capacity,std::uint32_t{8});q.map=std::move(q.partial_map);increment(native_stores);
  auto block=std::make_unique<Context::Queue::Block>();
  (*q.map)[0]=std::move(block);increment(native_stores);
  (*(*q.map)[0])[0]=fragment;increment(native_stores);store(q.size,q.size+std::uint32_t{1});
  fragment->queued=true;q.partial_fragment.reset();current();
 }
 void serialize_and_queue_registration_control(){
  require_control_sender();serialize_control1208();auto& message=*registration_control;
  // Original Control1208 offset04=true always takes direct send even if the
  // independent inner+84 state later changes; no false flag is inferred.
  need(message.offset04,"SP real Control1208 direct-send branch source changed");
  for(std::int32_t id=0;id<64;++id){
   const auto found=caller->participants_.find(id);current();
   if(found==caller->participants_.end()||!found->second)continue;
   const auto participant=found->second;
   // Actual7FDB40 repeats SAME service registry/peer-data lookup. Empty tree
   // returns actual byte count without transformation or future calculation.
   const auto registry=process->registry.find(registry_id);
   need(registry!=process->registry.end()&&registry->second==this,"SP fragment actual metadata service registry absent");
   need(peer_data.find(id)==peer_data.end(),"SP nonempty actual peer-data transformation unjoined");
   const auto second=caller->participants_.find(id);current();
   need(second!=caller->participants_.end()&&second->second==participant,
    "SP fragment original second participant lookup changed");
   const auto auxiliary=caller->auxiliary_participants_.find(participant->kind);current();
   if(auxiliary!=caller->auxiliary_participants_.end()&&auxiliary->second&&
    auxiliary->second->flag1a&&!message.offset04)continue;
   std::uint32_t offset=0,remaining=message.size;
   for(std::uint32_t part=0;part<message.parts;++part){
    const auto amount=std::min(remaining,mtu);queue_control_fragment(offset,amount);
    offset+=mtu;remaining-=mtu; // actual unsigned native fragment-loop stores
   }
   // Native inner+144 actual804EE0 reads SAME inner+40. This distinct
   // ctor-earned field is not info+14 or a copied latency DTO. Equal kept.
   store(participant->latency,inner_latency);current();
  }
  // Selected early canonical Bank configuration explicitly publishes first
  // release legacy diagnostic logging disabled. This optional compatibility
  // policy is independent of message bytes/RNG/queue/Game readiness.
  need(process->native_debug_config_returned&&!process->native_debug_logging,
   "SP optional legacy debug compatibility configuration unpublished");
  // Actual8065A3 inline temporary-stream destructor: descriptor store then
  // required owned heap buffer release; no invented field-zero native store.
  store(control_stream->descriptor,std::uint32_t{0xb76290});
  if(control_stream->owns&&control_stream->bytes){control_stream->bytes.reset();increment(control_stream->frees);}
  control_stream->destroyed=true;current();
 }
