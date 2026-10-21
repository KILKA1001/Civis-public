// SDK CvDllContext actual singleton initializer and GetPreGame ownership only.
// Its four service constructors are empty; their APIs remain unimplemented.
struct BnwSpClientPregameStartup::PreGameContext final {
 const BnwSpClientPregameStartup* const owner;
 bool entered=false,returned=false,failed=false;
 std::uint32_t rng_counter=0,net_init_counter=0,net_load_counter=0;
 std::uint64_t scalar_stores=0,os_calls=0;
 std::vector<std::pair<std::uint32_t,void*>> randoms,net_init,net_load;
 struct EmptyNativeService final {};
 std::array<std::unique_ptr<EmptyNativeService>,4> services;
#ifdef _WIN32
 HANDLE heap=INVALID_HANDLE_VALUE;
#endif
 struct Wrapper final {PreGameContext* context;const BnwSpClientPregameStartup* actual_namespace;};
 Wrapper* wrapper=nullptr;
 void* retained_wrapper_allocation=nullptr;
 bool wrapper_constructed=false,wrapper_queried=false,wrapper_destroy_called=false;
 explicit PreGameContext(const BnwSpClientPregameStartup& actual):owner(&actual){}
 ~PreGameContext(){
#ifdef _WIN32
  // Whole singleton lifetime retains its actual heap. A failed prefix cannot
  // be reset and reused. Process shutdown closes the actual owned heap only.
  if(heap!=INVALID_HANDLE_VALUE&&heap)HeapDestroy(heap);
#endif
 }
 void current()const{
  owner->current();need(!failed,"SP actual Pregame Context initializer/lease retired");
 }
 void initialize_once(){
  current();need(!entered&&!returned,"SP actual Pregame Context initializer replay");entered=true;
  try{
#ifdef _WIN32
   // OriginalInitializeSingleton calls both OS functions in this order and
   // ignores the LFH result. No original DLL entrypoint or allocator executes.
   ++os_calls;heap=HeapCreate(0,0,0);++scalar_stores;current();
   ULONG compatibility=2;++os_calls;
   (void)HeapSetInformation(heap,HeapCompatibilityInformation,&compatibility,sizeof(compatibility));current();
   need(heap!=nullptr,"SP actual Pregame Context heap allocation failed");
#else
   throw std::runtime_error("SP actual Pregame Context requires Windows owned HeapCreate/HeapSetInformation");
#endif
   // Ctor counters and default vectors then source-order four empty wrapper
   // constructors; no handlers are called or API capability invented.
   rng_counter=0;++scalar_stores;net_init_counter=0;++scalar_stores;net_load_counter=0;++scalar_stores;
   need(randoms.empty()&&net_init.empty()&&net_load.empty(),"SP actual Pregame Context constructor vectors differ");
   for(auto& service:services){service=std::make_unique<EmptyNativeService>();++scalar_stores;current();}
   returned=true;entered=false;current();
  }catch(...){failed=true;throw;}
 }
 #include "bnw_frame_pregame_interface.inc"
 void get_and_query_actual_pregame(){
  current();need(returned&&!wrapper&&!wrapper_constructed&&!wrapper_destroy_called,
   "SP actual GetPreGame cannot precede singleton constructor or replay");
  try{
#ifdef _WIN32
   ++os_calls;retained_wrapper_allocation=HeapAlloc(heap,0,sizeof(Wrapper));current();
   need(retained_wrapper_allocation!=nullptr,"SP actual GetPreGame allocation failed before QueryInterface");
   wrapper=new(retained_wrapper_allocation) Wrapper{this,owner};wrapper_constructed=true;
#else
   throw std::runtime_error("SP actual GetPreGame requires Windows owned heap");
#endif
   //5F7B80 singleton GetPreGame returns new CvDllPreGame then singleton
   // Destroy (native NOP).717F62 QueryPreGame2 returns SAME without AddRef.
   need(wrapper->context==this&&wrapper->actual_namespace==owner,"SP actual Pregame wrapper factory lineage changed");
   wrapper_queried=true;current();
  }catch(...){failed=true;throw;}
 }
 void destroy_after_native_event(){
  current();need(returned&&wrapper_constructed&&wrapper_queried&&wrapper&&!wrapper_destroy_called,
   "SP actual Pregame Destroy requires SAME queried allocation after event");
  try{
   // Native102EB2A0 invokes empty CvDllPreGame dtor then CvDllContext::Free.
   // Keep logical allocation retired after that call even if its BOOL is false;
   // originalFree ignores that result. No later pointer access is permitted.
   auto* actual=wrapper;actual->~Wrapper();wrapper=nullptr;
#ifdef _WIN32
   ++os_calls;(void)HeapFree(heap,0,actual);
#else
   throw std::runtime_error("SP actual Pregame Destroy requires Windows owned heap");
#endif
   retained_wrapper_allocation=nullptr;wrapper_destroy_called=true;current();
  }catch(...){failed=true;throw;}
 }
};
