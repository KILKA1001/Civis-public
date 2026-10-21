 // Native source-defined client155 is unrelated to GameCore EndTurn. Its
 // +30 actor and +34 boolean are constructed by original53FAF0(actor,true).
 struct Client155 final {
  std::int32_t offset18=0,offset14=0,offset1c=0,actor=0;
  std::uint32_t parts=0,type=0,offset08=0,size=0,offset20=0,offset24=0,descriptor=0;
  bool offset04=false,offset05=false,offset2c=false,value=false;
  bool constructed=false,submit_entered=false,submit_returned=false,retired=false;
 };
 std::shared_ptr<Client155> client155;
 std::shared_ptr<NativeStream> client155_stream;
 void require_client155_sender()const{
  need(caller->active_update_loan_,"SP155 requires actual canonical typed frame caller");
  caller->require_actual_control_callback(*caller->active_update_loan_);
  current();need(caller->control_stage_==BnwSpClientPregameStartup::ControlStage::sender&&
   caller->control_actor_==caller->local_id_&&context&&context->proxy==this&&context->mode==0&&
   !context->udp&&!context->steam,"SP155 sender genuine local branch/context absent");
 }
 void construct_client155(){
  require_client155_sender();need(!client155,"SP155 actual stack constructor replay");
  client155=std::make_shared<Client155>();auto& m=*client155;
  // Original7E4C60 source order, INCLUDING descriptor before base flags.
  store(m.type,std::uint32_t{155});store(m.descriptor,std::uint32_t{0xb72614});
  store(m.offset04,false);store(m.offset05,false);store(m.offset08,std::uint32_t{0});
  store(m.size,std::uint32_t{0});store(m.offset18,std::int32_t{-1});
  store(m.offset14,std::int32_t{-1});store(m.offset1c,std::int32_t{-1});
  store(m.offset20,std::uint32_t{0});store(m.offset24,std::uint32_t{0});
  store(m.parts,std::uint32_t{1});store(m.offset2c,false);
  //53FB05 actor;53FB08 direct flag;53FB0C derived descriptor;53FB12 bool.
  store(m.actor,caller->control_actor_);store(m.offset04,true);
  store(m.descriptor,std::uint32_t{0xa85314});store(m.value,true);
  m.constructed=true;require_client155_sender();
 }
 void client155_stream_constructor(){
  require_client155_sender();need(!client155_stream,"SP Control1208 temporary stream replay");
  client155_stream=std::make_shared<NativeStream>();auto& s=*client155_stream;
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
 void client155_stream_reset(NativeStream& s){
  // Native8393B0: all three cursor stores occur even when already zero.
  store(s.read_position,std::uint32_t{0});store(s.write_position,std::uint32_t{0});
  store(s.cursor,std::uint32_t{0});
 }
 void client155_stream_skip(NativeStream& s,std::uint32_t amount){
  // Native8391E0 reserve is cursor accounting, not a payload write.
  store(s.read_position,s.read_position+amount);store(s.write_position,s.write_position+amount);
  store(s.cursor,s.cursor+amount);
  need(s.cursor<=s.high_water,"SP actual native stream reserve exceeded serialized end");
 }
 void client155_stream_word(NativeStream& s,std::uint32_t word){
  require_client155_sender();need(s.constructed&&!s.destroyed,"SP actual stream unavailable");
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
 void client155_stream_bool(NativeStream& s,bool value){
  require_client155_sender();need(s.constructed&&!s.destroyed,"SP155 byte stream unavailable");
  //833CE0 calls actual839140 write with amount1, without endian conversion.
  const auto target=s.cursor+std::uint32_t{1};
  if(target>s.capacity){
   need(s.owns,"SP155 actual stream byte growth forbidden");
   const auto next=std::max(s.capacity*std::uint32_t{2},target);
   auto owned=std::make_unique<std::uint8_t[]>(next);increment(s.allocations);
   if(s.high_water){std::copy_n(s.bytes.get(),s.high_water,owned.get());increment(s.copies);}
   if(s.bytes){s.bytes.reset();increment(s.frees);}
   s.bytes=std::move(owned);increment(native_stores);store(s.capacity,next);
  }
  s.bytes[s.cursor]=value?std::uint8_t{1}:std::uint8_t{0};increment(s.copies);
  store(s.cursor,target);store(s.read_position,s.read_position+std::uint32_t{1});
  store(s.write_position,s.write_position+std::uint32_t{1});
  if(s.cursor>s.high_water)store(s.high_water,s.cursor);require_client155_sender();
 }
 void serialize_client155(){
  client155_stream_constructor();auto& s=*client155_stream;auto& m=*client155;
  const auto before=s.read_position;
  client155_stream_word(s,m.type);client155_stream_word(s,1);client155_stream_word(s,m.parts);
  client155_stream_word(s,static_cast<std::uint32_t>(m.offset18));
  client155_stream_word(s,static_cast<std::uint32_t>(m.offset14));
  client155_stream_word(s,m.offset20);client155_stream_word(s,m.offset24);
  //5FD360 base7E5180, actor834040, bool833CE0; no CString length word.
  client155_stream_word(s,static_cast<std::uint32_t>(m.actor));client155_stream_bool(s,m.value);
  store(m.size,s.read_position-before);need(m.size>0&&mtu>0,"SP155 size/MTU source unavailable");
  store(m.parts,(m.size-std::uint32_t{1})/mtu+std::uint32_t{1});
  const auto restore=s.write_position;client155_stream_reset(s);
  client155_stream_skip(s,4);client155_stream_skip(s,4);client155_stream_word(s,m.parts);
  client155_stream_skip(s,4);client155_stream_skip(s,4);client155_stream_skip(s,4);
  client155_stream_reset(s);client155_stream_skip(s,restore);
  need(s.read_position==m.size&&s.high_water==m.size,"SP155 header restoration differs");
  s.serialized=true;require_client155_sender();
 }
 void queue_client155_fragment(std::uint32_t offset,std::uint32_t amount){
  require_client155_sender();auto& q=context->queue;
  need(q.initialized&&q.proxy==&q&&q.map&&q.map_capacity==8&&
   client155_stream&&client155_stream->serialized&&offset+amount<=client155_stream->high_water,
   "SP155 requires SAME existing actual queue and serialized fragment");
  auto fragment=std::make_shared<Context::Fragment>();q.partial_fragment=fragment;
  fragment->payload=std::make_unique<std::uint8_t[]>(amount);increment(native_stores);
  std::copy_n(client155_stream->bytes.get()+offset,amount,fragment->payload.get());increment(fragment->copies);
  store(fragment->flags,std::uint32_t{1});store(fragment->offset00,std::uint32_t{0});
  store(fragment->length,amount);store(fragment->offset10,std::uint32_t{0});fragment->constructed=true;
  require_client155_sender();
  //802C20 reuses actual existing map. Original grow predicate executes before
  // block-index wrap. Unknown growth is an error after earned fragment bytes.
  const auto absolute=q.offset+q.size;
  if((absolute&3)==0&&q.map_capacity<=((q.size+4)>>2))
   throw std::runtime_error("SP155 mandatory existing deque map-growth source required");
  auto block_index=absolute>>2;if(block_index>=q.map_capacity)block_index-=q.map_capacity;
  need(block_index<q.map_capacity,"SP155 original deque index outside actual map");
  auto& block=(*q.map)[block_index];
  if(!block){auto created=std::make_unique<Context::Queue::Block>();block=std::move(created);increment(native_stores);}
  // Replace physical slot only. The currently popped Control fragment remains
  // in partial_frame_drain until its genuine receiver/destruction returns.
  (*block)[absolute&3]=fragment;increment(native_stores);store(q.size,q.size+std::uint32_t{1});
  fragment->queued=true;q.partial_fragment.reset();require_client155_sender();
 }
 void submit_client155(){
  construct_client155();auto& m=*client155;need(!m.submit_entered&&!m.retired,"SP155 submission replay");
  m.submit_entered=true;
  try{
   need(native_metadata&&native_metadata->returned&&native_metadata->observer,
    "SP155 genuine metadata getter absent");require_client155_sender();
   auto& metadata=*native_metadata;
   store(m.offset20,metadata.offset2c+metadata.offset24);
   const auto previous=metadata.offset30;store(metadata.offset30,previous+std::uint32_t{1});store(m.offset24,previous);
   need(context!=nullptr,"SP155 actual context lost after metadata partial writes");serialize_client155();
   need(m.offset04,"SP155 original direct flag changed");
   for(std::int32_t id=0;id<64;++id){
    const auto found=caller->participants_.find(id);require_client155_sender();
    if(found==caller->participants_.end()||!found->second)continue;
    const auto participant=found->second;
    const auto actual=process->registry.find(registry_id);
    need(actual!=process->registry.end()&&actual->second==this,"SP155 actual registry sender retired");
    need(peer_data.find(id)==peer_data.end(),"SP155 required nonempty peer-data transform unjoined");
    const auto second=caller->participants_.find(id);require_client155_sender();
    need(second!=caller->participants_.end()&&second->second==participant,"SP155 actual second participant alias changed");
    const auto auxiliary=caller->auxiliary_participants_.find(participant->kind);require_client155_sender();
    if(auxiliary!=caller->auxiliary_participants_.end()&&auxiliary->second&&auxiliary->second->flag1a&&!m.offset04)continue;
    std::uint32_t offset=0,remaining=m.size;
    for(std::uint32_t part=0;part<m.parts;++part){
     const auto amount=std::min(remaining,mtu);queue_client155_fragment(offset,amount);
     offset+=mtu;remaining-=mtu;
    }
    store(participant->latency,inner_latency);require_client155_sender();
   }
   need(process->native_debug_config_returned&&!process->native_debug_logging,
    "SP155 selected first-release logging compatibility unpublished");
   store(client155_stream->descriptor,std::uint32_t{0xb76290});
   if(client155_stream->owns&&client155_stream->bytes){client155_stream->bytes.reset();increment(client155_stream->frees);}
   client155_stream->destroyed=true;require_client155_sender();m.submit_returned=true;
  }catch(...){m.retired=true;quarantine();throw;}
 }
