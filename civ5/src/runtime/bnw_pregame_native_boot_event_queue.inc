// Same once process native UI event namespace; source CRT9BA2C0→79EB60 and
//683EC0. This is distinct from CvNotifications and never claims delivery.
class BnwPregameNativeBootEventQueue final {
 static constexpr std::size_t buffer_bytes=0x400080;
 struct Buffer final {
  std::unique_ptr<std::uint8_t[]> bytes;
  std::atomic<std::uint32_t> count;
  // Zero virtual image tail precedes real ctor683EC0's explicit count0 store.
  Buffer():bytes(std::make_unique<std::uint8_t[]>(buffer_bytes)),count(0){}
 };
 struct Arena final {
  std::array<Buffer,2> buffers;
  std::uint32_t cycle=0,max_published=0;
 };
 struct Callback final {std::uint32_t thunk=0;const void* context=nullptr;};
 const std::thread::id thread_=std::this_thread::get_id();
 std::shared_ptr<assets::CivisResourceBankOwner> bank_;
 assets::CivisResourceBankSourceRecipe recipe_;
 // Original initializer stores0 index, calls two Buffer constructors per
 // Arena (4 counter stores), 4 Arena scalar stores, dispatch lock0 store,
 // and then one actual callback-registry memset. No ready/count inference.
 std::uint32_t arena_index_=0,dispatch_lock_=0;
 std::array<Arena,2> arenas_;
 std::array<std::array<Callback,192>,2> callbacks_{};
 std::uint64_t initializer_scalar_stores_=0,initializer_memsets_=0;
 mutable std::atomic<bool> retired_{false};
 bool entered_=false,event149_published_=false;
 struct Record final {
  std::uint32_t arena=0,parity=0,offset=0;
  std::int32_t actor=-1;
  bool counter_incremented=false,header_returned=false,actor_stored=false;
 } record_;
 struct ClientRecord {std::uint32_t arena=0,parity=0,type=0,offset=0;
  bool counter_incremented=false,header_returned=false;};
 std::vector<ClientRecord> client_records_;
 std::uint64_t producer_scalar_stores_=0;
 void current()const{
  if(thread_!=std::this_thread::get_id()||retired_.load()){
   retired_.store(true);throw std::runtime_error("Pregame native event queue foreign thread/retired before fields");
  }
  try{
   bank_->require_current();const auto canonical=bank_->retain_owner();
   locale_need(canonical.get()==bank_.get()&&!canonical.owner_before(bank_)&&!bank_.owner_before(canonical),"Pregame native queue Bank control changed");
   const auto now=bank_->source_recipe();locale_need(now.recipe_identity==recipe_.recipe_identity&&now.save_path==recipe_.save_path,"Pregame native queue selected lifetime changed");
   locale_need(initializer_scalar_stores_==10&&initializer_memsets_==1&&arena_index_<2,"Pregame native UI event initializer absent");
   bank_->require_current();locale_need(!retired_.load(),"Pregame native queue retired during checks");
  }catch(...){retired_.store(true);throw;}
 }
 static void put32(Buffer& buffer,std::size_t at,std::uint32_t value){
  locale_need(at+4<=buffer_bytes,"Pregame native UI event source offset outside buffer");
  for(std::size_t n=0;n<4;++n)buffer.bytes[at+n]=static_cast<std::uint8_t>(value>>(n*8));
 }
 BnwPregameNativeBootEventQueue(std::shared_ptr<assets::CivisResourceBankOwner> actual)
  :bank_(std::move(actual)),recipe_(bank_->source_recipe()){
  // Buffer byte allocations above stand for the pre-constructor loader-zero
  // virtual image tail. These real source ctor stores now occur in order.
  arena_index_=0;++initializer_scalar_stores_;
  for(auto& arena:arenas_){
   for(auto& buffer:arena.buffers){buffer.count.store(0,std::memory_order_seq_cst);++initializer_scalar_stores_;}
   arena.cycle=0;++initializer_scalar_stores_;
   arena.max_published=0;++initializer_scalar_stores_;
  }
  dispatch_lock_=0;++initializer_scalar_stores_;
  for(auto& registry:callbacks_)for(auto& callback:registry)callback={};
  ++initializer_memsets_;
  current();
 }
public:
 static std::shared_ptr<BnwPregameNativeBootEventQueue> capture_once(
  const std::shared_ptr<assets::CivisResourceBankOwner>& actual){
  // Actual selected bootstrap is the sole emitter of this process namespace.
  // Earlier use may never be reset or replaced with a copied zero counter.
  struct Process final {std::mutex mutex;bool claimed=false;std::shared_ptr<BnwPregameNativeBootEventQueue> owner;};
  static Process process;
  const std::lock_guard guard(process.mutex);
  locale_need(!process.claimed&&!process.owner,"Pregame native UI event process lifetime already used; real restart unjoined");
  process.claimed=true;
  process.owner=std::shared_ptr<BnwPregameNativeBootEventQueue>(new BnwPregameNativeBootEventQueue(actual));
  return process.owner;
 }
 void require_current()const{current();}
 void current_for_client(const BnwSpClientUpdateLoan& loan)const{
  loan.require_current_without_callbacks();
  if(thread_!=std::this_thread::get_id()||retired_.load()){
   retired_.store(true);throw std::runtime_error("SP client event arena foreign creator/retired");
  }
  try{
   bank_->require_current();loan.require_current_without_callbacks();
   const auto canonical=bank_->retain_owner();loan.require_current_without_callbacks();
   locale_need(canonical.get()==bank_.get()&&!canonical.owner_before(bank_)&&!bank_.owner_before(canonical),
    "SP client event SAME imported native149 arena Bank changed");
   const auto now=bank_->source_recipe();loan.require_current_without_callbacks();
   locale_need(now.recipe_identity==recipe_.recipe_identity&&now.save_path==recipe_.save_path&&
    initializer_scalar_stores_==10&&initializer_memsets_==1&&arena_index_<2&&event149_published_,
    "SP client event actual same initialized arena/Bank import origin absent");
   bank_->require_current();loan.require_current_without_callbacks();
   locale_need(!retired_.load(),"SP client native arena retired during producer checks");
  }catch(...){retired_.store(true);throw;}
 }
 void publish_native_client146_or_145(const BnwSpClientUpdateLoan& loan,std::uint32_t type){
  current_for_client(loan);
  if(entered_||(type!=146&&type!=145)){retired_.store(true);throw std::runtime_error("SP client actual146/145 producer reentered/type replaced");}
  entered_=true;
  try{
   auto& arena=arenas_.at(arena_index_);const auto parity=arena.cycle&1;
   auto& buffer=arena.buffers.at(parity);const auto before=buffer.count.load(std::memory_order_seq_cst);
   locale_need(before<=buffer_bytes-0x100,"SP client native event arena original overflow source required");
   ClientRecord record;record.arena=arena_index_;record.parity=parity;record.type=type;
   client_records_.push_back(record);auto& actual=client_records_.back();
   // Original636860/764C60 same128-byte19FE200 namespace. Native counter
   // increments BEFORE four headers, with no copied receipt or new arena.
   const auto old=buffer.count.fetch_add(0x80,std::memory_order_seq_cst);
   ++producer_scalar_stores_;actual.counter_incremented=true;actual.offset=old+0x80;
   put32(buffer,actual.offset,0x80);++producer_scalar_stores_;
   put32(buffer,actual.offset+4,type);++producer_scalar_stores_;
   put32(buffer,actual.offset+8,0);++producer_scalar_stores_;
   put32(buffer,actual.offset+12,0);++producer_scalar_stores_;actual.header_returned=true;
   // Neither actual native caller stores a payload/actor for146 or145.
   current_for_client(loan);entered_=false;
  }catch(...){retired_.store(true);throw;}
 }

 void publish_native_client155_event(const BnwSpClientUpdateLoan& loan,std::uint32_t type){
  current_for_client(loan);
  if(entered_||(type!=146&&type!=132)){retired_.store(true);throw std::runtime_error("SP client actual155146/132 producer reentered/type replaced");}
  entered_=true;
  try{
   auto& arena=arenas_.at(arena_index_);const auto parity=arena.cycle&1;
   auto& buffer=arena.buffers.at(parity);const auto before=buffer.count.load(std::memory_order_seq_cst);
   locale_need(before<=buffer_bytes-0x100,"SP client native event arena original overflow source required");
   ClientRecord record;record.arena=arena_index_;record.parity=parity;record.type=type;
   client_records_.push_back(record);auto& actual=client_records_.back();
   // Original636860/6830F0 same128-byte19FE200 namespace. Native counter
   // increments BEFORE four headers, with no copied receipt or new arena.
   const auto old=buffer.count.fetch_add(0x80,std::memory_order_seq_cst);
   ++producer_scalar_stores_;actual.counter_incremented=true;actual.offset=old+0x80;
   put32(buffer,actual.offset,0x80);++producer_scalar_stores_;
   put32(buffer,actual.offset+4,type);++producer_scalar_stores_;
   put32(buffer,actual.offset+8,0);++producer_scalar_stores_;
   put32(buffer,actual.offset+12,0);++producer_scalar_stores_;actual.header_returned=true;
   // Neither actual native caller stores a payload/actor for146 or132.
   current_for_client(loan);entered_=false;
  }catch(...){retired_.store(true);throw;}
 }

 void publish_native149(std::int32_t actor){
  current();if(entered_||event149_published_||actor<0||actor>=64){retired_.store(true);throw std::runtime_error("Pregame native149 publication replay/reentry/range");}entered_=true;
  try{
   // Read actual retained arena index and parity from the initialized namespace.
   // Native8B7A40 performs InterlockedExchangeAdd BEFORE any record header.
   auto& arena=arenas_.at(arena_index_);const auto parity=arena.cycle&1;
   auto& buffer=arena.buffers.at(parity);
   const auto before=buffer.count.load(std::memory_order_seq_cst);
   locale_need(before<=buffer_bytes-0x100,"Pregame native event arena capacity needs original overflow owner");
   record_.arena=arena_index_;record_.parity=parity;
   const auto old=buffer.count.fetch_add(0x80,std::memory_order_seq_cst);
   ++producer_scalar_stores_;record_.counter_incremented=true;record_.offset=old+0x80;
   // Real source arena storage exists BEFORE increment, so no allocator/callback
   // separates this native count mutation from header's straight-line stores.
   put32(buffer,record_.offset,0x80);++producer_scalar_stores_;
   put32(buffer,record_.offset+4,0x95);++producer_scalar_stores_;
   put32(buffer,record_.offset+8,0);++producer_scalar_stores_;
   put32(buffer,record_.offset+12,0);++producer_scalar_stores_;
   record_.header_returned=true;
   // Original718266 stores actor into returned payload, not a global slot.
   put32(buffer,record_.offset+16,static_cast<std::uint32_t>(actor));++producer_scalar_stores_;
   record_.actor=actor;record_.actor_stored=true;event149_published_=true;
   current();entered_=false;
  }catch(...){retired_.store(true);throw;}
 }
 // No renderer/UI callback registration, cycle swap, dispatch or receipt is
 // fabricated here. The actual pending arena bytes remain strongly retained.
};
