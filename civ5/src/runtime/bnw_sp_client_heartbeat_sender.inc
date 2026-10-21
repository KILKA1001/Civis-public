 // True1004 targeted native805F80->806060->806600 submission. This
 // is a real context enqueue; the next receiver is still independently
 // mandatory. Reuses SAME existing Boot/Context/NativeStream storage type.
 std::shared_ptr<NativeStream> heartbeat_stream;
 void require_heartbeat_sender_owned()const{
  current();need(caller->active_update_loan_&&inner_post_metadata_entered&&!inner_post_metadata_returned&&
   actual_post_context_returned&&inner_update_depth>0&&partial_heartbeat&&partial_heartbeat->constructed&&
   !partial_heartbeat->retired&&context&&context->proxy==this&&context->mode==0&&!context->udp&&!context->steam,
   "SP1004 sender lacks genuine current inner suffix/stack message/context");
  caller->active_update_loan_->require_current_without_callbacks();current();
 }
 void heartbeat_stream_constructor(){
  require_heartbeat_sender_owned();need(!heartbeat_stream,"SP1004 temporary stream replay");
  heartbeat_stream=std::make_shared<NativeStream>();auto& s=*heartbeat_stream;
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
 void heartbeat_stream_reset(NativeStream& s){
  // Native8393B0: all three cursor stores occur even when already zero.
  store(s.read_position,std::uint32_t{0});store(s.write_position,std::uint32_t{0});
  store(s.cursor,std::uint32_t{0});
 }
 void heartbeat_stream_skip(NativeStream& s,std::uint32_t amount){
  // Native8391E0 reserve is cursor accounting, not a payload write.
  store(s.read_position,s.read_position+amount);store(s.write_position,s.write_position+amount);
  store(s.cursor,s.cursor+amount);
  need(s.cursor<=s.high_water,"SP actual native stream reserve exceeded serialized end");
 }
 void heartbeat_stream_word(NativeStream& s,std::uint32_t word){
  require_heartbeat_sender_owned();need(s.constructed&&!s.destroyed,"SP actual stream unavailable");
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

 void serialize_actual_heartbeat1004_owned(){
  heartbeat_stream_constructor();auto& s=*heartbeat_stream;auto& m=*partial_heartbeat;
  const auto before=s.read_position;
  // Specialised B72DA8+8 ->7EA1B0 ->base7E5180: seven words ONLY.
  heartbeat_stream_word(s,m.type);heartbeat_stream_word(s,1);heartbeat_stream_word(s,m.parts);
  heartbeat_stream_word(s,static_cast<std::uint32_t>(m.offset18));
  heartbeat_stream_word(s,static_cast<std::uint32_t>(m.offset14));
  heartbeat_stream_word(s,m.offset20);heartbeat_stream_word(s,m.offset24);
  store(m.size,s.read_position-before);need(m.size==28&&mtu>0,"SP actual1004 base wire/MTU mismatch");
  store(m.parts,(m.size-std::uint32_t{1})/mtu+std::uint32_t{1});
  const auto restore=s.write_position;heartbeat_stream_reset(s);
  heartbeat_stream_skip(s,4);heartbeat_stream_skip(s,4);heartbeat_stream_word(s,m.parts);
  heartbeat_stream_skip(s,4);heartbeat_stream_skip(s,4);heartbeat_stream_skip(s,4);
  heartbeat_stream_reset(s);heartbeat_stream_skip(s,restore);
  need(s.read_position==m.size&&s.high_water==m.size,"SP actual1004 header restoration differs");
  s.serialized=true;require_heartbeat_sender_owned();
 }
 void queue_heartbeat_fragment(std::uint32_t offset,std::uint32_t amount){
  require_heartbeat_sender_owned();auto& q=context->queue;
  need(q.initialized&&q.proxy==&q&&q.map&&q.map_capacity==8&&
   heartbeat_stream&&heartbeat_stream->serialized&&offset+amount<=heartbeat_stream->high_water,
   "SP1004 requires SAME existing actual queue and serialized fragment");
  auto fragment=std::make_shared<Context::Fragment>();q.partial_fragment=fragment;
  fragment->payload=std::make_unique<std::uint8_t[]>(amount);increment(native_stores);
  std::copy_n(heartbeat_stream->bytes.get()+offset,amount,fragment->payload.get());increment(fragment->copies);
  store(fragment->flags,std::uint32_t{1});store(fragment->offset00,std::uint32_t{0});
  store(fragment->length,amount);store(fragment->offset10,std::uint32_t{0});fragment->constructed=true;
  require_heartbeat_sender_owned();
  //802C20 reuses actual existing map. Original grow predicate executes before
  // block-index wrap. Unknown growth is an error after earned fragment bytes.
  const auto absolute=q.offset+q.size;
  if((absolute&3)==0&&q.map_capacity<=((q.size+4)>>2))
   throw std::runtime_error("SP1004 mandatory existing deque map-growth source required");
  auto block_index=absolute>>2;if(block_index>=q.map_capacity)block_index-=q.map_capacity;
  need(block_index<q.map_capacity,"SP1004 original deque index outside actual map");
  auto& block=(*q.map)[block_index];
  if(!block){auto created=std::make_unique<Context::Queue::Block>();block=std::move(created);increment(native_stores);}
  // Replace physical slot only. The currently popped Control fragment remains
  // in partial_frame_drain until its genuine receiver/destruction returns.
  (*block)[absolute&3]=fragment;increment(native_stores);store(q.size,q.size+std::uint32_t{1});
  fragment->queued=true;q.partial_fragment.reset();require_heartbeat_sender_owned();
 }

 void submit_actual_heartbeat1004_owned(){
  require_heartbeat_sender_owned();auto& m=*partial_heartbeat;
  need(!m.submit_entered&&!m.submit_returned,"SP1004 targeted sender replay");m.submit_entered=true;
  try{
   // Actual808340 reads SAME info+24 participant map FIRST. Missing target
   // takes original service/peer fallback; current constructor/real map
   // provides our target, so no fabricated connected/default result.
   const auto first=caller->participants_.find(heartbeat_target);
   need(first!=caller->participants_.end()&&first->second,
    "SP1004 absent target original808340/peer fallback mandatory");
   const auto participant=first->second;require_heartbeat_sender_owned();
   //805FAA +134 service getter THEN7FDAD0 reads registry and peer tree.
   const auto actual=process->registry.find(registry_id);
   need(actual!=process->registry.end()&&actual->second==this,"SP1004 actual service registry retired");
   need(peer_data.find(heartbeat_target)==peer_data.end(),"SP1004 nonempty7FDAD0 peer transform mandatory");
   const auto transformed=heartbeat_target; // genuine empty tree unchanged return
   const auto second=caller->participants_.find(transformed);
   need(second!=caller->participants_.end()&&second->second==participant,
    "SP1004 original first post-transform participant read changed");
   const auto third=caller->participants_.find(transformed);
   need(third!=caller->participants_.end()&&third->second==participant,
    "SP1004 original repeated participant read changed");
   const auto target_kind=third->second->kind;require_heartbeat_sender_owned();
   // Native806060 obtains actual metadata, writes message20, then sequence
   //30 and message24 BEFORE generic targeted serializer/send.
   need(native_metadata&&native_metadata->returned&&native_metadata->observer,
    "SP1004 native metadata supplier absent");auto& metadata=*native_metadata;
   store(m.offset20,metadata.offset2c+metadata.offset24);
   const auto previous=metadata.offset30;store(metadata.offset30,previous+std::uint32_t{1});store(m.offset24,previous);
   require_heartbeat_sender_owned();
   if(heartbeat_stream){need(heartbeat_stream->destroyed,"SP1004 prior actual temporary stream not retired");heartbeat_stream.reset();}
   serialize_actual_heartbeat1004_owned();
   need(m.offset04&&m.parts==1,"SP1004 direct force1 multipart source mandatory");
   // Original806860 mode0/force1 and constructor NULL transports reach the
   // actual existing queue. No broadcast or fake network completion.
   std::uint32_t offset=0,remaining=m.size;
   for(std::uint32_t part=0;part<m.parts;++part){
    const auto amount=std::min(remaining,mtu);queue_heartbeat_fragment(offset,amount);
    offset+=mtu;remaining-=mtu;
   }
   //8066A8 uses auxiliary info+44 map with target KIND, unlike the first
   //participant map above. SAME shared participant alias earns latency4.
   const auto aux=caller->auxiliary_participants_.find(target_kind);
   if(aux!=caller->auxiliary_participants_.end()&&aux->second){
    store(aux->second->latency,inner_latency);require_heartbeat_sender_owned();
   }
   need(process->native_debug_config_returned&&!process->native_debug_logging,
    "SP1004 selected first-release logging compatibility unpublished");
   // Original806836 descriptor BEFORE required owned heap release.
   store(heartbeat_stream->descriptor,std::uint32_t{0xb76290});
   if(heartbeat_stream->owns&&heartbeat_stream->bytes){heartbeat_stream->bytes.reset();increment(heartbeat_stream->frees);}
   heartbeat_stream->destroyed=true;require_heartbeat_sender_owned();m.submit_returned=true;
  }catch(...){m.retired=true;quarantine();throw;}
 }
