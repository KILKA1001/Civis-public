//Included inside civ5::ui after the genuine current ordinary receiver borrower.
//51E6A0 does NOT dispatch4C/94: its only native receiver effects are two40
//stores, separated by the real77AB00 parse/temporary-destruction return.
namespace {
std::uint32_t native_anchor_flags_77ab00_owned(const char* borrowed){
 //Fresh mapped x64 lexical strings own their storage/NUL/length/lifetime.
 //Internal Original256B inline headers/pool addresses are not runtime grants.
 //Declaration order preserves77AC12 input ->vertical ->horizontal destruction
 //before the POD mask actually returns to51E6B4.
 std::string horizontal,vertical,input;
 if(borrowed){const auto length=std::strlen(borrowed); //Actual NUL scan, not Lua object length.
  require(length<=static_cast<std::size_t>(INT32_MAX),"SetAnchor actual signed FStringA length unsupported");
  input.assign(borrowed,length); //81E890: own copy, embedded NUL terminates input.
 }
 {
  std::string working=input; //Actual45AAF0 has its OWN fourth temporary.
  //81DE00 then81DD40 classify unsigned bytes with actual CRT isspace.
  std::size_t left=0;while(left<working.size()&&std::isspace(static_cast<unsigned char>(working[left])))++left;
  if(left)working.erase(0,left);
  std::size_t right=working.size();while(right&&std::isspace(static_cast<unsigned char>(working[right-1])))--right;
  working.resize(right);
  if(!working.empty()){
   std::int32_t position=0;
   const auto length=static_cast<std::int32_t>(working.size());
   const auto token=[&](std::string&out){
    //Actual81D540 strpbrk starts AT position: repeated/leading delimiters
    //produce empty tokens. It never skips them or trims the output token.
    const std::int32_t begin=position;
    const auto found=begin>=0&&begin<length?
     working.find_first_of(" .,x*|",static_cast<std::size_t>(begin)):std::string::npos;
    position=found==std::string::npos?-1:static_cast<std::int32_t>(found);
    const auto count_word=static_cast<std::uint32_t>(position>=0?position:length)-static_cast<std::uint32_t>(begin);
    std::int32_t count=std::bit_cast<std::int32_t>(count_word); //native signed DWORD SUB.
    if(position>=0)++position; //81D586 actual next-cursor store before substring.
    const std::int32_t start=std::max(begin,0);
    count=std::max(count,0); //81D3B0 clamps start and count independently.
    if(start>length)count=0;
    else if(static_cast<std::int64_t>(start)+count>length)count=length-start;
    if(!count)out.clear();
    else out.assign(working.data()+start,static_cast<std::size_t>(count));
   };
   token(horizontal);token(vertical); //45AB75 before45AB90.
   //45AB9D tests source length, which81D540/81D3B0 do not modify.
   //These private distinct owners prove it stays positive; no retry/skip is added.
  }
 } //Actual45AAF0 temporary release BEFORE any77AB00 comparison.
 std::uint32_t result=0;
 //Exact four literals/one-byte comparison and conditional call order.
 if(::_strnicmp(horizontal.c_str(),"Right",1)==0)result=0x20u;
 else if(::_strnicmp(horizontal.c_str(),"Center",1)==0)result=0x10u;
 if(::_strnicmp(vertical.c_str(),"Bottom",1)==0)result|=0x100u;
 else if(::_strnicmp(vertical.c_str(),"Center",1)==0)result|=0x80u;
 return result; //NULL genuinely follows empty assignment/trim path and returns0.
}
}
void BnwWorldViewControlTree::set_native_ordinary_anchor_51e6a0_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,const char* borrowed,
 const std::function<void()>& rooted_without_callbacks){
 using Base=detail::NotificationLayoutBaseline;using Parent=detail::NotificationLayoutParentReceiver;
 const auto actual=core_;
 const auto pure=[&](bool busy){actual->layout_origin(busy);
  const auto&current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"SetAnchor offered retired/different Source operation");
  current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin(busy);
  require(bool(rooted_without_callbacks),"SetAnchor actual Lua argument2 root borrower absent");
  rooted_without_callbacks();actual->layout_origin(busy);};
 try{
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  const auto id=receiver->impl_->id;
  actual->busy=true;struct Exit{bool&value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  auto&state=*actual->nodes.at(id-1).layout_baseline;
  require(state.history_.max_size()-state.history_.size()>=2&&state.sequence_<=UINT64_MAX-2&&
   actual->effect_revision<=UINT64_MAX-2&&actual->layout_sequence<=UINT64_MAX-2,
   "SetAnchor physical two-store history exhausted");
  state.history_.reserve(state.history_.size()+2);pure(true);
  const auto commit=[&](std::uint32_t value){
   pure(true);actual->initial_lua_preflight_store_owned();pure(true);
   commit_native_ordinary_flag_word_owned(actual,receiver,concrete,NativeOrdinaryFlagWrite::anchor_51e6a0,value);pure(true);
  };
  commit(state.base_.flags_40&0xfffffe4fu); //51E6A7 FIRST, even unchanged/NULL input.
  const auto parsed=native_anchor_flags_77ab00_owned(borrowed); //Required genuine x64 parse return.
  pure(true);commit(state.base_.flags_40|parsed); //51E6B4 fresh read AFTER parser/local DTORs.
  //No native4C/94/parent or28/2C/20/24/bounds/cache writer exists in this
  //whole member. Real flags and writer history advance; geometric caches
  //remain as Original leaves them until an actual later layout operation.
  pure(true);operation.require_current();pure(true);
 }catch(...){actual->failed.store(true);throw;} //The first real AND survives a parse failure.
}
