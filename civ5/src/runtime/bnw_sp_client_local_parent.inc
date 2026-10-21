// Private Source12. Pinned EXE origins are recorded in the packet. This is a
// local SP prefix: no Steam listener/transport ACK and no returned registration.
struct BnwSpClientPregameStartup::BootOwner final {
 struct Process final {
  // Source physical PE loader-initialized C88DE4=FFFFFFFF, BEFORE CRT.
  // SAME once process owns the writable cell; no frame/game-turn default.
  std::int32_t scheduler_last_turn=-1;
  // Future native629000->665CE0 and reset42A6C0/4E1130 ingress MUST update
  // this retained cell in original order before those messages can return.
  // Current accepted1208/155 path has no writer. Unknown types stay STOP.
  std::uint64_t scheduler_write_revision=0;
  // Different physical native loader cell C88DE8, not C88DE4. Native
  //608A30 reads it; actual614694 and reset4E11AA must write this SAME
  // storage before those future mandatory ingress paths may return.
  std::int32_t loadstate_last_turn=-1;
  std::uint64_t loadstate_write_revision=0;
  enum class FactoryKind {uninitialized,control1208,client155,heartbeat1004};
  // SAME once process original CRT namespace. The implemented native1208
  // supplier is registered BEFORE service/Read, never lazy on receive.
  std::map<std::int32_t,FactoryKind> control_factory;
  std::map<std::int32_t,std::int32_t> partial_assemblies;
  std::uint64_t factory_native_stores=0;
  bool control_factory_returned=false,assembly_constructor_returned=false,client155_factory_returned=false;
  bool heartbeat1004_factory_returned=false;
  // Supported selected Civis launcher version profile. Original CRT9CACE4
  // calls9B7D30 and constructs SAME global5CF7EA0 fromB727E8. The legacy
  //51BA90 installer is not invoked by this selected launcher. No original
  // launcher VersionInfo/GetVersionString ABI is claimed. Future installer
  // mutations require a real setter and revision/currentness join first.
  enum class VersionProfile {selected_civis_crt_constructor};
  const VersionProfile version_profile=VersionProfile::selected_civis_crt_constructor;
  std::string control_version;
  std::uint64_t control_version_revision=0;
  bool version_constructor_returned=false;
  Process(){
   // Earlier actual CRT9CAB58->9C5AB0->7F0530 registers155 supplier69CE10
   // in SAME5CF2B24 namespace, BEFORE9CAC58 assembly and9CAC7C1208.
   auto [client_entry,client_inserted]=control_factory.emplace(155,FactoryKind::uninitialized);
   need(client_inserted,"SP actual155 CRT supplier collided");
   client_entry->second=FactoryKind::client155;++factory_native_stores;
   client155_factory_returned=true;
   // Native CRT9CAC58 ->9B7AF0 ->7E5560 actual empty assembly constructor.
   // Civis uses its native C++ map backend for logical contents; empty dummy
   // hasher/bucket pointers are not transported as gameplay permission.
   need(partial_assemblies.empty(),"SP actual first assembly constructor differs");
   assembly_constructor_returned=true;
   // Native CRT9CAC7C ->9B7B90 ->7E7200, retained global factory constructor
   // and source operator[] default entry followed by actual1208 supplier store.
   need(control_factory.size()==1&&client155_factory_returned&&
    control_factory.at(155)==FactoryKind::client155,
    "SP actual1208 CRT required earlier155 supplier differs");
   auto [entry,inserted]=control_factory.emplace(1208,FactoryKind::uninitialized);
   need(inserted,"SP original Control1208 factory registration collided");
   entry->second=FactoryKind::control1208;++factory_native_stores;
   control_factory_returned=true;
   // Later native CRT9CACB0->9B7C60 JMP7E7610. SAME5CF2B24 namespace
   // operator[] key1004 default entry, then actual supplier7E6530 store.
   // This precedes selected version constructor9CACE4 and any live receive.
   auto [heartbeat,heartbeat_inserted]=control_factory.emplace(1004,FactoryKind::uninitialized);
   need(heartbeat_inserted,"SP actual1004 CRT supplier collided");
   heartbeat->second=FactoryKind::heartbeat1004;++factory_native_stores;
   heartbeat1004_factory_returned=true;
   //9B7D30 std::string(const char*) then original atexit registration. This
   // real once Process owns lifetime/destruction through its std::string.
   control_version.assign("UNKNOWN VERSION");version_constructor_returned=true;
  }
  // Native5CF2B2C lies in the PE loader-zero virtual tail. This object is
  // constructed ONCE for the process; never reset for another imported save.
  std::uint32_t next_id=0;
  // Actual5CF2B16 loader-zero; actual C9F444 PE initialized word1.
  // Both cells belong to the SAME once process owner.
  // Future selected setters cannot reset these already earned fields.
  bool native_debug_logging=false,native_debug_config_returned=false;
  std::uint32_t native_stream_byte_order=1;
  const BnwSpClientPregameStartup* lease=nullptr;
  bool claimed=false;
  std::atomic<bool> retired{false};
  std::mutex mutex;
  std::thread::id thread;
  std::map<std::uint32_t,BootOwner*> registry;
  std::shared_ptr<BootOwner> retained;
 };
 static std::shared_ptr<Process> process_lifetime(){
  static const auto retained=std::make_shared<Process>();return retained;
 }
 Process* const process=process_lifetime().get();
 const BnwSpClientPregameStartup* const caller;
 std::shared_ptr<BnwSpClientPregameStartup::CachedGlobals> retained_globals;
 std::shared_ptr<const BnwImportedPregameLocaleOwner> retained_locale;
 std::shared_ptr<BnwSpClientPregameStartup::PreGameContext> retained_pregame_context;
 bool service_returned=false,inner_returned=false,start_prefix_returned=false;
 bool retired=false,entered=false;
 std::uint64_t native_stores=0,os_calls=0;
 struct Windows final {
#ifdef _WIN32
  HMODULE sockets=nullptr,multimedia=nullptr;
  decltype(&WSAStartup) startup=nullptr;
  decltype(&WSACleanup) cleanup=nullptr;
  using Period=unsigned int (WINAPI*)(unsigned int);
  using Clock=DWORD (WINAPI*)();
  Period begin=nullptr,end=nullptr;
  Clock clock=nullptr;
  using Version=BOOL (WINAPI*)(LPOSVERSIONINFOA);
  Version version=nullptr;
  bool startup_called=false,period_called=false,service_returned=false;
  Windows(){
   sockets=LoadLibraryExW(L"ws2_32.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
   multimedia=LoadLibraryExW(L"winmm.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
   need(sockets&&multimedia,"SP client required Windows system libraries absent");
   startup=reinterpret_cast<decltype(startup)>(GetProcAddress(sockets,"WSAStartup"));
   cleanup=reinterpret_cast<decltype(cleanup)>(GetProcAddress(sockets,"WSACleanup"));
   begin=reinterpret_cast<Period>(GetProcAddress(multimedia,"timeBeginPeriod"));
   end=reinterpret_cast<Period>(GetProcAddress(multimedia,"timeEndPeriod"));
   clock=reinterpret_cast<Clock>(GetProcAddress(multimedia,"timeGetTime"));
   version=reinterpret_cast<Version>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"GetVersionExA"));
   need(startup&&cleanup&&begin&&end&&clock&&version,"SP client required Windows source entrypoint absent");
  }
  ~Windows(){
   // Service destructor7FA7AA invokes these independent calls in this order.
   // Library handles remain system-owned until their actual effects return.
   if(service_returned&&end)end(1);
   if(service_returned&&cleanup)cleanup();
   if(multimedia)FreeLibrary(multimedia);
   if(sockets)FreeLibrary(sockets);
  }
#else
  Windows(){throw std::runtime_error("SP local client requires real Windows timeGetTime/WSA source");}
#endif
 };
 std::unique_ptr<Windows> windows;
 // Relevant service/inner/proxy/context stores have actual retained ownership.
 // Unused vtable addresses are source descriptors, never executable pointers.
 std::array<std::map<std::int32_t,std::int32_t>,3> service_maps;
 std::map<std::int32_t,std::int32_t> optional_service_map;
 // Native service7FA55A allocates its separate service+88 userdata tree.
 std::map<std::int32_t,void*> nickname_userdata;
 std::map<std::int32_t,std::vector<std::int32_t>> peer_data;
 std::uint32_t observer_descriptor=0,registry_id=0;
 BootOwner* proxy_inner=nullptr;
 const BnwSpClientPregameStartup* proxy_wrapper=nullptr;
 BootOwner* wrapper_inner=nullptr;
 bool host_flag=false,metadata_enabled=false,inner_simultaneous=false;
 void* connector=nullptr;void* optional_component=nullptr;
 float metadata_rate=0,info_rate=0,inner_latency=0;
 bool inner45=false,inner46=false;
 std::uint32_t inner_update_depth=0;
 std::int32_t linked_a=-1,linked_b=-1;
 struct Descriptor {std::uint32_t kind=0;std::uint8_t flags=0;std::uint32_t id=0;
  BootOwner* bound=nullptr;std::uint32_t target=0;};
 // Actual8050F3 separate eight-byte adapter; the inner B74EC0 object
 // has no receiver method at +14. Keep real parent registry-ID ownership.
 struct ReceiverAdapter {std::uint32_t descriptor=0,parent_registry_id=0;bool constructed=false;};
 std::unique_ptr<ReceiverAdapter> inner_receiver_adapter;
 struct Context {
  struct Fragment {
   std::unique_ptr<std::uint8_t[]> payload;
   std::uint32_t flags=0,offset00=0,length=0,offset10=0;
   std::uint64_t copies=0;bool constructed=false,queued=false;
  };
  struct Queue {
   using Block=std::array<std::shared_ptr<Fragment>,4>;
   using Map=std::array<std::unique_ptr<Block>,8>;
   Queue* proxy=nullptr;
   std::unique_ptr<Map> map,partial_map;
   std::shared_ptr<Fragment> partial_fragment;
   std::uint32_t map_capacity=0,offset=0,size=0;
   bool initialized=false;
  } queue;
  std::array<Descriptor,4> descriptors;
  BootOwner* proxy=nullptr; // own sender affinity only, not native context+50
  ReceiverAdapter* receiver=nullptr; // actual constructor argument from inner+18
  void* udp=nullptr;void* steam=nullptr;
  std::uint32_t mode=0;bool server=false,accept=true;
 };
 std::unique_ptr<Context> context;
 std::uint32_t mtu=0,seed=0,clock_value=0;
 bool clock_returned=false,wrapper_sp_flag=false;
 template<class T> void store(T& field,const T& value){increment(native_stores);field=value;}
 explicit BootOwner(const BnwSpClientPregameStartup& owner):caller(&owner){
  // Single ordinary imported SP bootstrap is supported here. A prior fresh or
  // imported client lifetime cannot be manufactured from a new archive DTO.
  // No reset/reuse method exists: extending it needs its actual retire/restart.
  const std::lock_guard lock(process->mutex);
  need(!process->claimed&&!process->retired.load()&&process->registry.empty(),
   "SP client process lifetime already used; genuine retire/restart unjoined");
  process->claimed=true;process->thread=std::this_thread::get_id();process->lease=caller;
 }
 void current()const{
  caller->current();need(!retired&&!process->retired.load()&&process->claimed&&
   process->thread==std::this_thread::get_id()&&process->lease==caller,
   "SP local client process/parent lender retired");
 }
 bool is_host()const{
  current();const auto it=process->registry.find(registry_id);
  need(inner_returned&&it!=process->registry.end()&&it->second==this&&wrapper_inner==this,
   "SP client SAME actual registry/inner required before isHost");
  if(host_flag)return true;
  return caller->host_id_!=-1&&caller->host_id_==caller->local_id_;
 }
 bool connected()const{
  current();if(!context)return false; // original8073E0 missing context branch
  need(context->mode==0&&!context->server&&!context->steam,
   "SP client nonlocal context/Steam transport not in release scope");
  need(!context->udp,"SP client actual UDP source unjoined");
  return false; // native802490 reads SAME ctor-null UDP and returns zero
 }
 void service_constructor(){
  current();need(!service_returned&&!windows,"SP service constructor cannot replay");
  // 7FA520: three actual empty registry/map constructors, then observer=NULL.
  for(const auto& map:service_maps)need(map.empty(),"SP native service map constructor differs");
  store(observer_descriptor,std::uint32_t{0});
  need(nickname_userdata.empty(),"SP actual service+88 userdata tree constructor differs");
  // 7FF250 is called even for SP. Its bool result is IGNORED by7FA520;
  // syscall effects still occur in the original order, including failed cases.
  windows=std::make_unique<Windows>();
#ifdef _WIN32
  WSADATA data{};increment(os_calls);windows->startup_called=true;
  const auto result=windows->startup(MAKEWORD(2,2),&data);current();
  if(result==0){
   if(LOBYTE(data.wVersion)==2&&HIBYTE(data.wVersion)==2){
    increment(os_calls);windows->period_called=true;(void)windows->begin(1);current();
    OSVERSIONINFOA version{};version.dwOSVersionInfoSize=sizeof(version);
    increment(os_calls);const auto observed=windows->version(&version);current();
    need(observed!=FALSE,"SP source GetVersionExA observation failed; no guessed OS version");
    // Native global C9F5AF only disables a transport policy for OS major<6.
    // Retain the earned observation; it is not a Game/UI readiness flag.
    if(version.dwMajorVersion<6)store(legacy_socket_policy,false);
   }else{
    increment(os_calls);(void)windows->cleanup();current();
   }
  }
#endif
  // 59F670 actual optional service registry is created because loader-zero
  // global5CF2B24 has not been replaced by an earlier unsupported lifetime.
  need(optional_service_map.empty(),"SP actual optional service map differs");
  // Constructor default observer7FB080(NULL), then5C20E9 replacement. Steam
  // Init flag5C2091 is outside this SP release and never licenses any getter.
  store(observer_descriptor,std::uint32_t{0x9d9b04});
  store(observer_descriptor,std::uint32_t{0x9ee2e0});
  service_returned=true;
#ifdef _WIN32
  windows->service_returned=true;
#endif
  current();
 }
 bool legacy_socket_policy=true;
#include "bnw_sp_client_registration_control.inc"
#include "bnw_sp_client_registration_stream.inc"
#include "bnw_sp_client155_sender.inc"
#include "bnw_sp_inner_post_metadata_update.inc"
#include "bnw_sp_client_heartbeat_sender.inc"
#include "bnw_sp_client_mode0_frame_update.inc"
#include "bnw_sp_client_mode0_control_receiver.inc"
#include "bnw_sp_client_heartbeat_receiver.inc"
#include "bnw_sp_client155_receiver.inc"
 void inner_constructor(BnwSpClientPregameStartup& owner){
  current();need(service_returned&&!inner_returned&&caller==&owner&&process->registry.empty(),
   "SP inner requires actual service before registration");
  // Native640FA0 actual proxy, then7FD770/804F70/805090 actual inner.
  store(proxy_inner,static_cast<BootOwner*>(nullptr));store(proxy_wrapper,caller);
  store(registry_id,process->next_id);store(linked_a,std::int32_t{-1});store(linked_b,std::int32_t{-1});
  store(inner_latency,0.0f); // native80509A separate inner+40, not info+14
  store(host_flag,false);
  store(inner45,false);store(inner46,false); // original8050A4/8050A7
  store(seed,std::uint32_t{0});
  store(connector,static_cast<void*>(nullptr));store(optional_component,static_cast<void*>(nullptr));
  // Original8050F3..805105 allocates/initializes the distinct adapter BEFORE
  // metadata7FE470, then publishes the pointer at inner+18. Keep failed child
  // construction owned; no lazy receiver or impersonated inner vtable.
  need(!inner_receiver_adapter,"SP actual inner receiver adapter constructor replayed");
  auto actual_receiver=std::make_unique<ReceiverAdapter>();
  store(actual_receiver->descriptor,std::uint32_t{0xb75168});
  store(actual_receiver->parent_registry_id,registry_id);
  actual_receiver->constructed=true;inner_receiver_adapter=std::move(actual_receiver);increment(native_stores);
  metadata_constructor(); // actual retained7FE470 supplier, original field order
  need(owner.participants_.empty(),"SP actual info map must have native empty constructor");
  need(owner.auxiliary_participants_.empty(),"SP actual second info map must have native empty constructor");
  store(info_rate,0.0f);store(info_parent_id,registry_id);
  store(owner.local_id_,std::int32_t{-1});store(owner.host_id_,std::int32_t{-1});
  store(info_expected_count,std::uint32_t{1});store(info_offset10,std::uint32_t{0});
  store(inner_slot28,static_cast<void*>(nullptr)); // actual80524E
  store(inner_update_depth,std::uint32_t{0}); // original80525A before8090C0 peer tree
  need(peer_data.empty(),"SP actual peer-data map native constructor differs");
  store(inner88,-1.0f); //805262 reads source physical9E2940=-1.0f
  store(inner_simultaneous,false); // actual80526E constructor byte inner+84
  // Empty native a4/a8 peer-removal vector is retained by the owned backend.
  need(pending_peer_removals.empty(),"SP inner real constructor removal vector differs");
  store(proxy_inner,this); // actual7FD7DA parent link AFTER inner ctor returns
  // Native operator[] first inserts its zero mapped value; store follows.
  auto [it,inserted]=process->registry.emplace(registry_id,nullptr);
  need(inserted,"SP service native ID collision");store(it->second,this);
  store(process->next_id,process->next_id+std::uint32_t{1});
  const auto actual=process->registry.find(registry_id);
  need(actual!=process->registry.end()&&actual->second==this,"SP actual service lookup failed");
  store(wrapper_inner,actual->second); // native490DCA SAME wrapper binding
  store(metadata_rate,0.125f);store(info_rate,0.0f); // actual+9C ignores supplied10
  store(metadata_enabled,false);inner_returned=true;current();
 }
 void context_constructor(){
  current();need(inner_returned&&!context,"SP context constructor cannot replay");
  // 8053FD first reads isHost OR connected against SAME newborn native info.
  need(!is_host()&&!connected(),"SP original start rejected already-active client");
  auto created=std::make_unique<Context>();
  const std::array<std::uint32_t,4> targets{0x802840,0x802920,0x802910,0x802a40};
  for(std::size_t i=0;i<4;++i){auto& d=created->descriptors[i];
   store(d.flags,std::uint8_t{0});store(d.id,std::uint32_t{0});
   store(d.kind,i%2?std::uint32_t{0xb74ad8}:std::uint32_t{0xb74acc});
   store(d.bound,this);store(d.target,targets[i]);
   // SteamAPI_RegisterCallback only: excluded Steamworks registration is not
   // executed/ACKed. Native mode0 callback bodies return before all effects.
   if(i>=2)store(d.flags,std::uint8_t{2});
  }
  need(inner_receiver_adapter&&inner_receiver_adapter->constructed&&
   inner_receiver_adapter->descriptor==0xb75168&&inner_receiver_adapter->parent_registry_id==registry_id,
   "SP native context requires SAME actual inner+18 receiver adapter");
  // Actual805450 ->801010 receiver argument owns context+50. The sender
  // affinity is separate bookkeeping and must not count as a native store.
  created->proxy=this;store(created->receiver,inner_receiver_adapter.get());
  store(created->udp,static_cast<void*>(nullptr));
  store(created->steam,static_cast<void*>(nullptr));
  // Actual8010C1..8010E1 retained native queue proxy and empty fields.
  store(created->queue.proxy,&created->queue);
  need(!created->queue.map,"SP context native queue initial NULL map differs");increment(native_stores);
  store(created->queue.map_capacity,std::uint32_t{0});store(created->queue.offset,std::uint32_t{0});
  store(created->queue.size,std::uint32_t{0});created->queue.initialized=true;
  store(created->mode,std::uint32_t{0});
  store(created->server,false);store(created->accept,true);
  // Native805464 publishes actual context only after801010 returns.
  context=std::move(created);increment(native_stores);current();
  // Native693159 real SP caller supplies startMode0, local=false, ctor config
  // byte5=false. No actor, imported gameMode or active-player inference.
  store(context->mode,std::uint32_t{0});store(context->server,false);
  store(mtu,std::uint32_t{1450}); // C9F8D8 mode0, exact8012E4 clamp result
  need(!connected(),"SP context unexpectedly connected before local registration");
  // Original805499 config byte5==AL==0 skips808720, then calls connected again.
  need(!connected(),"SP context changed before real local seed");
 }
 void local_start_prefix(BnwSpClientPregameStartup& owner){
  current();need(!entered&&!start_prefix_returned&&caller==&owner,"SP client start prefix replay");
  entered=true;
  try{
   need(owner.early_locale_!=nullptr&&!process->native_debug_config_returned,
    "SP early selected logging compatibility cannot replay or borrow another client");
   owner.early_locale_->require_first_release_client_debug_configuration(owner);
   store(process->native_debug_logging,false); // actual7E4C50 equal setter, explicit own configuration
   process->native_debug_config_returned=true;current();
   service_constructor();inner_constructor(owner);context_constructor();
#ifdef _WIN32
   increment(os_calls);clock_value=windows->clock();clock_returned=true;current();
   const std::uint32_t mixed=clock_value*std::uint32_t{0x343fd}+std::uint32_t{0x269ec3};
   store(seed,mixed%std::uint32_t{0xffffcfef}); // original804ED0 inner+3C
#endif
   need(clock_returned,"SP local seed requires genuine Windows clock return");
   store(host_flag,false); // native80550A unconditional equal store
   start_prefix_returned=true;current();
  }catch(...){retired=true;process->retired=true;throw;}
 }
 void require_before_registration()const{
  current();need(start_prefix_returned&&clock_returned&&inner_returned&&service_returned&&
   context&&context->mode==0&&proxy_inner==this&&wrapper_inner==this,
   "SP original7F2FF0 cannot enter without returned genuine local parent prefix");
 }
 void registration_prefix(BnwSpClientPregameStartup& owner){
  require_before_registration();need(caller==&owner,"SP registration SAME info required");
  need(!connector&&!optional_component,"SP optional client component producer unjoined");
  // Original+190 returns NULL from SAME ctor-null connector. Original+150
  // also takes its genuine NULL connector branch and returns true, before
  // allocating a local slot through the actual Pregame interface.
  const auto id=owner.native_local_allocator_prefix();
  need(id!=-1&&owner.native_validate_local_slot(id),"SP native local allocation/validation rejected");
  store(owner.selected_registration_id_,id);
  store(owner.local_id_,id);store(owner.host_id_,id); // actual7F2E54/7F2E59
  // Actual7F3240 lookup returns NULL from SAME empty first info map; result is
  // ignored.7FDA20 uses SAME peer-data namespace and +8C local-ID getter.
  current();const auto prior=owner.participants_.find(id);
  need(prior==owner.participants_.end(),"SP local participant already exists");
  const auto peer=peer_data.find(owner.local_id_);
  need(peer==peer_data.end(),"SP original local peer-data variant source unjoined");current();
  // Native7F2F9B owns the actual0x20 participant before either info map insert.
  auto participant=std::make_shared<BnwSpClientPregameStartup::Participant>();
  store(participant->latency,0.0f);store(participant->counter,std::uint32_t{0});
  store(participant->userdata,static_cast<void*>(nullptr));store(participant->interval,0.0f);
  store(participant->kind,std::int32_t{0});store(participant->id,id);
  store(participant->flag18,false);store(participant->flag19,false);store(participant->flag1a,false);
  store(participant->state,std::int32_t{2});
  // Original7EAD40 userdataNULL returns without allocation/effects. Second
  // 7FDA20 still reads current local-ID and SAME empty peer-data map =>false.
  current();need(peer_data.find(owner.local_id_)==peer_data.end(),"SP peer-data changed during registration");
  auto [aux,aux_inserted]=owner.auxiliary_participants_.emplace(0,nullptr);
  need(aux_inserted,"SP original auxiliary map key already inserted");store(aux->second,participant);
  auto [first,first_inserted]=owner.participants_.emplace(id,nullptr);
  need(first_inserted,"SP original participant map key already inserted");store(first->second,participant);
  // Original inner+168 has the genuine ctor-null optional component branch.
  need(!optional_component,"SP native optional participant callback unjoined");
  // Only NOW does actual proxy+20 reach717F00. The required GC handicap/DLC
  // suffix can fail with both map aliases and IDs retained; count is not ACK.
  owner.phase_=BnwSpClientPregameStartup::Phase::registration_prefix;
  owner.native_registration_callback_prefix();
  need(owner.native_callback_returned_,"SP actual callback/native149/interface Destroy not returned");
  // Original7F2F4D compares actual first-map count against info+C. These
  // fields were earned by constructor and actual registration, never Read DTO.
  current();need(owner.participants_.size()<=64,"SP actual registration map size overflow");
  if(owner.participants_.size()==info_expected_count){
   construct_registration_control();submit_registration_control_prefix();
   need(registration_control&&registration_control->submit_returned&&control_stream&&control_stream->destroyed,
    "SP original Submit/stream destructor did not return");
   store(info_expected_count,std::uint32_t{0}); // native7F2F6D, AFTER returned Submit
   current(); // proxy+10/532F60 actual RET has no effects
   // Original CString destructor at7F2F87: real local string owner retires
   // before registration returns. Retained constructor/bytes/queue history
   // survives, never replayed or treated as physical message processing ACK.
   registration_control->text.clear();registration_control->retired=true;
  }
  // Native7F2D60 returns actual allocated ID.7F2FF0 stores info+8 even on
  // equality;805532 stores info+4 after successful start;6930B0's mode0
  // caller writes its actual SP wrapper flag only AFTER start returned.
  store(owner.local_id_,id);store(owner.host_id_,id);
  store(wrapper_sp_flag,false);owner.registration_returned_=true;current();
 }
 void quarantine()noexcept{retired=true;process->retired=true;}
};
