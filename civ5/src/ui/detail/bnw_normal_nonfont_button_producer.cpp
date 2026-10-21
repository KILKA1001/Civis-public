#include "civ5/ui/detail/bnw_normal_nonfont_pipeline.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include <cstring>
#include <stdexcept>
#include <thread>
#include <xmmintrin.h>
#include <emmintrin.h>

namespace civ5::ui::detail {
namespace {
void nonfont_button_need(bool condition,const char* why){
 if(!condition)throw std::runtime_error(why);
}
std::uint32_t nonfont_button_word(const BnwNormalNonFontNativeRegion& region,std::size_t offset){
 nonfont_button_need(region.owner&&region.bytes&&offset<=region.extent&&4<=region.extent-offset,
  "Button original native backing word is unavailable");
 const auto* p=region.bytes+offset;
 return std::uint32_t(std::to_integer<unsigned char>(p[0]))|
  (std::uint32_t(std::to_integer<unsigned char>(p[1]))<<8)|
  (std::uint32_t(std::to_integer<unsigned char>(p[2]))<<16)|
  (std::uint32_t(std::to_integer<unsigned char>(p[3]))<<24);
}
std::uint32_t nonfont_button_subss(std::uint32_t value,std::uint32_t literal){
 // Native SUBSS, including the current MXCSR rounding/denormal controls.
 float a,b;std::memcpy(&a,&value,4);std::memcpy(&b,&literal,4);
 const auto result=_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b));
 const float scalar=_mm_cvtss_f32(result);std::uint32_t word;
 std::memcpy(&word,&scalar,4);return word;
}
std::uint32_t nonfont_button_cvtsi2ss(std::uint32_t word){
 std::int32_t signed_word;std::memcpy(&signed_word,&word,4);
 const auto result=_mm_cvtsi32_ss(_mm_setzero_ps(),signed_word);
 const float scalar=_mm_cvtss_f32(result);std::uint32_t bits;
 std::memcpy(&bits,&scalar,4);return bits;
}
// Exact finite operands expressed as signed magnitude in units of2^-149.
// 320bits cover every finiteF32 operand and every signed32 FILD integer.
// No floating host approximation or assumed literal value is used.
using NonFontMagnitude=std::array<std::uint32_t,10>;
NonFontMagnitude nonfont_magnitude(std::uint32_t mantissa,unsigned shift){
 NonFontMagnitude out{};const unsigned word=shift/32,bit=shift%32;
 const std::uint64_t wide=std::uint64_t(mantissa)<<bit;
 out[word]=static_cast<std::uint32_t>(wide);
 if(word+1<out.size())out[word+1]=static_cast<std::uint32_t>(wide>>32);
 return out;
}
int nonfont_high_bit(const NonFontMagnitude& value){
 for(unsigned word=10;word--!=0;)if(value[word]){
  unsigned bit=31;while(((value[word]>>bit)&1u)==0)--bit;
  return static_cast<int>(word*32+bit);
 }
 return -1;
}
bool nonfont_magnitude_bit(const NonFontMagnitude& value,unsigned bit){
 return ((value[bit/32]>>(bit%32))&1u)!=0;
}
bool nonfont_magnitude_low_bits(const NonFontMagnitude& value,unsigned end){
 const auto whole=end/32,partial=end%32;
 for(unsigned i=0;i<whole;++i)if(value[i])return true;
 return partial&&(value[whole]&((std::uint32_t{1}<<partial)-1u));
}
int nonfont_magnitude_compare(const NonFontMagnitude& a,const NonFontMagnitude& b){
 for(unsigned i=10;i--!=0;)if(a[i]!=b[i])return a[i]>b[i]?1:-1;
 return 0;
}
NonFontMagnitude nonfont_magnitude_add(const NonFontMagnitude& a,const NonFontMagnitude& b){
 NonFontMagnitude out{};std::uint64_t carry=0;
 for(unsigned i=0;i!=10;++i){const auto sum=std::uint64_t(a[i])+b[i]+carry;
  out[i]=static_cast<std::uint32_t>(sum);carry=sum>>32;}
 nonfont_button_need(carry==0,"Button finite native arithmetic exceeds exact magnitude");return out;
}
NonFontMagnitude nonfont_magnitude_subtract(const NonFontMagnitude& a,const NonFontMagnitude& b){
 NonFontMagnitude out{};std::uint64_t borrow=0;
 for(unsigned i=0;i!=10;++i){const auto subtrahend=std::uint64_t(b[i])+borrow;
  out[i]=static_cast<std::uint32_t>(std::uint64_t(a[i])-subtrahend);
  borrow=std::uint64_t(a[i])<subtrahend;}
 nonfont_button_need(borrow==0,"Button signed native arithmetic subtraction order invalid");return out;
}
NonFontMagnitude nonfont_magnitude_round(NonFontMagnitude value,unsigned precision,unsigned rc,bool negative){
 const auto high=nonfont_high_bit(value);
 if(high<0||static_cast<unsigned>(high)+1<=precision)return value;
 const unsigned drop=static_cast<unsigned>(high)+1-precision;
 const bool discarded=nonfont_magnitude_low_bits(value,drop);
 bool up=false;
 if(rc==0)up=nonfont_magnitude_bit(value,drop-1)&&
  (nonfont_magnitude_low_bits(value,drop-1)||nonfont_magnitude_bit(value,drop));
 else if(rc==1)up=negative&&discarded;
 else if(rc==2)up=!negative&&discarded;
 const auto whole=drop/32,partial=drop%32;
 for(unsigned i=0;i<whole;++i)value[i]=0;
 if(partial)value[whole]&=~((std::uint32_t{1}<<partial)-1u);
 if(up){std::uint64_t carry=std::uint64_t{1}<<partial;
  for(unsigned i=whole;i!=10&&carry;++i){const auto sum=std::uint64_t(value[i])+carry;
   value[i]=static_cast<std::uint32_t>(sum);carry=sum>>32;}
  nonfont_button_need(carry==0,"Button native rounding exceeds exact magnitude");}
 return value;
}
std::uint32_t nonfont_magnitude_fstp32(NonFontMagnitude value,unsigned rc,bool negative){
 value=nonfont_magnitude_round(value,24,rc,negative);
 const auto high=nonfont_high_bit(value);const auto sign=negative?0x80000000u:0u;
 if(high<0)return sign;
 if(high<23)return sign|value[0]; // exact subnormal in2^-149 units
 const unsigned exponent=static_cast<unsigned>(high)-22;
 if(exponent>=255){const bool infinity=rc==0||(rc==1&&negative)||(rc==2&&!negative);
  return sign|(infinity?0x7f800000u:0x7f7fffffu);}
 const unsigned shift=static_cast<unsigned>(high)-23,word=shift/32,bit=shift%32;
 std::uint64_t mantissa=value[word];
 if(word+1<value.size())mantissa|=std::uint64_t(value[word+1])<<32;
 return sign|(exponent<<23)|(static_cast<std::uint32_t>(mantissa>>bit)&0x7fffffu);
}
std::uint32_t nonfont_button_fild_fadd_fstp(std::uint32_t original,std::uint32_t fadd_f32,
 std::uint32_t x87_cw){
 const unsigned rc=(x87_cw>>10)&3u;
 const bool integer_negative=(original&0x80000000u)!=0;
 const auto integer_magnitude=integer_negative?std::uint32_t(0u-original):original;
 auto value=nonfont_magnitude(integer_magnitude,149);
 if(!integer_negative)return nonfont_magnitude_fstp32(value,rc,false); // original JGE bypasses FADD
 const auto operand_exponent=(fadd_f32>>23)&0xffu;
 nonfont_button_need(operand_exponent!=255,"Button nonfinite Original FADD operand unsupported");
 const bool operand_negative=(fadd_f32&0x80000000u)!=0;
 const auto operand_mantissa=(fadd_f32&0x7fffffu)|(operand_exponent?0x800000u:0u);
 const auto operand=nonfont_magnitude(operand_mantissa,operand_exponent?operand_exponent-1:0);
 bool negative=true;
 if(operand_negative)value=nonfont_magnitude_add(value,operand);
 else {const auto order=nonfont_magnitude_compare(value,operand);
  if(order>0)value=nonfont_magnitude_subtract(value,operand);
  else if(order<0){value=nonfont_magnitude_subtract(operand,value);negative=false;}
  else {value=NonFontMagnitude{};negative=rc==1;}}
 // Actual x87 FADD precision control applies before actual FSTP single.
 const unsigned pc=(x87_cw>>8)&3u;
 nonfont_button_need(pc!=1,"Button reserved Original x87 precision control unsupported");
 const unsigned precision=pc==0?24:(pc==2?53:64);
 value=nonfont_magnitude_round(value,precision,rc,negative);
 return nonfont_magnitude_fstp32(value,rc,negative);
}
}

struct BnwNormalNonFontPaintInvocation::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 const CapturedFields fields;
 struct Snapshot final {
  BnwNormalNonFontNativeRegion receiver;
  std::array<std::byte,0x60> words;
  std::array<std::optional<BnwNormalNonFontNativeRegion>,2> sources;
  std::array<std::array<std::uint32_t,3>,2> source_words;
  std::array<std::uint32_t,5> arguments;
  std::array<std::uint32_t,2> fixed_globals;
  std::array<std::uint32_t,2> literals;
  std::uint32_t outer_flags;
 };
 const Snapshot snapshot;
 static Snapshot capture_snapshot(const CapturedFields& captured){
  captured.receiver.require_current_without_callbacks();captured.frame.require_current_without_callbacks();
  const auto region=captured.receiver.receiver_region_without_callbacks();
  nonfont_button_need(region.owner&&region.bytes&&region.extent>=0x60,"Button own118 receiver unavailable");
  Snapshot result;result.receiver=region;std::memcpy(result.words.data(),region.bytes,0x60);
  for(std::size_t lane=0;lane!=2;++lane){
   result.sources[lane]=captured.receiver.source_region_without_callbacks(lane);
   if(result.sources[lane]){
    const auto& source=*result.sources[lane];
    result.source_words[lane]={nonfont_button_word(source,0x10c),nonfont_button_word(source,0x110),
     nonfont_button_word(source,0x11c)};
   }
  }
  result.arguments=captured.receiver.argument_words_without_callbacks();
  result.fixed_globals=captured.receiver.fixed_slot_global_words_without_callbacks();
  result.literals=captured.receiver.arithmetic_literal_words_without_callbacks();
  result.outer_flags=captured.receiver.flags_word40_without_callbacks();
  captured.receiver.require_current_without_callbacks();captured.frame.require_current_without_callbacks();
  return result;
 }
 explicit Impl(CapturedFields captured):fields(std::move(captured)),snapshot(capture_snapshot(fields)){}
 void same_snapshot()const{
  const auto current=fields.receiver.receiver_region_without_callbacks();
  nonfont_button_need(current.owner&&current.bytes==snapshot.receiver.bytes&&
   current.native_base==snapshot.receiver.native_base&&current.extent==snapshot.receiver.extent&&
   !current.owner.owner_before(snapshot.receiver.owner)&&!snapshot.receiver.owner.owner_before(current.owner),
   "Button own118 physical receiver backing changed");
  nonfont_button_need(std::memcmp(snapshot.words.data(),current.bytes,0x60)==0,
   "Button own118 receiver words changed across Source/callback");
  for(std::size_t lane=0;lane!=2;++lane){
   const auto current_source=fields.receiver.source_region_without_callbacks(lane);
   const auto& original=snapshot.sources[lane];
   nonfont_button_need(bool(current_source)==bool(original),"Button source44/48 branch changed");
   if(original){
    nonfont_button_need(current_source->owner&&current_source->bytes==original->bytes&&
     current_source->native_base==original->native_base&&current_source->extent==original->extent&&
     !current_source->owner.owner_before(original->owner)&&!original->owner.owner_before(current_source->owner),
     "Button source44/48 backing changed");
    nonfont_button_need(snapshot.source_words[lane]==std::array<std::uint32_t,3>{
     nonfont_button_word(*current_source,0x10c),nonfont_button_word(*current_source,0x110),
     nonfont_button_word(*current_source,0x11c)},"Button source IDs/scalar changed across callbacks");
   }
  }
  nonfont_button_need(snapshot.arguments==fields.receiver.argument_words_without_callbacks()&&
   snapshot.fixed_globals==fields.receiver.fixed_slot_global_words_without_callbacks()&&
   snapshot.literals==fields.receiver.arithmetic_literal_words_without_callbacks(),
   "Button own118 argument/global/literal capture changed");
  nonfont_button_need(snapshot.outer_flags==fields.receiver.flags_word40_without_callbacks(),
   "Button original OUTER flags40 changed across Source callbacks");
 }
 void first()const{
  nonfont_button_need(creator==std::this_thread::get_id(),"NonFont Invocation foreign creator");
  fields.receiver.require_current_without_callbacks();
  fields.frame.require_current_without_callbacks();
  fields.root.require_physical_current_without_callbacks();
  nonfont_button_need(fields.publication&&fields.matrix&&fields.pool&&fields.slots&&fields.resources,
   "NonFont Invocation lacks canonical publication/matrix/storage/resource provider");
  fields.frame.require_actual_publication_without_callbacks(*fields.publication);
  fields.receiver.require_actual_publication_without_callbacks(*fields.publication);
  fields.receiver.require_actual_frame_without_callbacks(fields.frame);
  fields.receiver.require_same_root_without_callbacks(fields.root);
  fields.root.require_actual_matrix_without_callbacks(*fields.matrix);
  fields.pool->require_physical_without_callbacks();
  fields.slots->require_physical_without_callbacks();
  fields.resources->require_physical_without_callbacks();
  fields.resources->require_same_root_without_callbacks(fields.root);
  fields.receiver.require_current_without_callbacks();fields.frame.require_current_without_callbacks();
  same_snapshot();
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
  first();fields.receiver.require_source_owned(operation);first();
  fields.frame.require_source_owned(operation);first();
  fields.root.require_source_owned(operation);first();
  fields.pool->require_source_owned(operation);first();
  fields.slots->require_source_owned(operation);first();
  fields.resources->require_source_owned(operation);first();
 }
};
BnwNormalNonFontPaintInvocation::BnwNormalNonFontPaintInvocation(std::shared_ptr<const Impl> data)
 :impl_(std::move(data)){}
BnwNormalNonFontPaintInvocation::~BnwNormalNonFontPaintInvocation()=default;
BnwNormalNonFontPaintInvocation BnwNormalNonFontPaintInvocation::capture_owned(
 const BnwNormalNonFontCaptureKey&,const runtime::BnwWorldViewSourceOperationLoan& operation,
 CapturedFields captured){
 // Strong fields and original guard values are captured before Source/allocator.
 Impl before(captured);before.full(operation);before.first();
 auto data=std::make_shared<const Impl>(std::move(captured));
 before.first();before.full(operation);data->full(operation);
 return BnwNormalNonFontPaintInvocation(std::move(data));
}
void BnwNormalNonFontPaintInvocation::require_current_without_callbacks()const{
 const auto data=impl_;nonfont_button_need(bool(data),"NonFont Invocation absent");data->first();
}
void BnwNormalNonFontPaintInvocation::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto data=impl_;nonfont_button_need(bool(data),"NonFont Invocation absent");data->full(operation);
}
void BnwNormalNonFontPaintInvocation::require_same_invocation_without_callbacks(
 const BnwNormalNonFontPaintInvocation& other)const{
 const auto a=impl_,b=other.impl_;a->first();b->first();
 nonfont_button_need(a.get()==b.get(),"NonFont Invocation control mismatch");
 a->fields.receiver.require_same_receiver_without_callbacks(b->fields.receiver);a->first();b->first();
}
void BnwNormalNonFontPaintInvocation::require_same_root_without_callbacks(
 const BnwNormalRootBatchHistory::Guard& root)const{
 const auto data=impl_;data->first();data->fields.root.require_same_root_without_callbacks(root);data->first();
}
void BnwNormalNonFontPaintInvocation::require_actual_publication_without_callbacks(
 const ::civ5::ui::BnwWorldViewControlRenderPublication& publication)const{
 const auto data=impl_;data->first();
 nonfont_button_need(data->fields.publication.get()==&publication,"NonFont original Publication mismatch");
 data->fields.receiver.require_actual_publication_without_callbacks(publication);data->first();
}
void BnwNormalNonFontPaintInvocation::require_actual_renderer_without_callbacks(
 const ::civ5::ui::UiScreenGlRenderer& renderer)const{
 const auto data=impl_;data->first();data->fields.receiver.require_actual_renderer_without_callbacks(renderer);
 data->fields.frame.require_actual_renderer_without_callbacks(renderer);data->first();
}
void BnwNormalNonFontPaintInvocation::require_actual_frame_without_callbacks(
 const BnwNormalRootPaintCallFrame::Guard& frame)const{
 const auto data=impl_;data->first();data->fields.receiver.require_actual_frame_without_callbacks(frame);data->first();
}
std::uint64_t BnwNormalNonFontPaintInvocation::actual_control_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.receiver.actual_control_without_callbacks();
}
std::size_t BnwNormalNonFontPaintInvocation::actual_ordered_ordinal_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.receiver.actual_ordered_ordinal_without_callbacks();
}
std::uint32_t BnwNormalNonFontPaintInvocation::flags_word40_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.receiver.flags_word40_without_callbacks();
}
BnwNormalRootBatchHistory::Guard BnwNormalNonFontPaintInvocation::captured_root_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.root;
}
std::shared_ptr<const BnwWindowedRendererMatrixDataLoan>
BnwNormalNonFontPaintInvocation::actual_matrix_owned_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.matrix;
}
std::shared_ptr<const BnwNormalNonFontNativePool>
BnwNormalNonFontPaintInvocation::native_pool_owned_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.pool;
}
std::shared_ptr<const BnwNormalNonFontNativeSlots>
BnwNormalNonFontPaintInvocation::native_slots_owned_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.slots;
}
std::shared_ptr<const BnwNormalNonFontResourceBank>
BnwNormalNonFontPaintInvocation::resource_bank_owned_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.resources;
}
BnwNormalNonFontCurrentReceiverGuard BnwNormalNonFontPaintInvocation::captured_receiver_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.receiver;
}
std::array<std::uint32_t,2> BnwNormalNonFontPaintInvocation::numeric_controls_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.frame.actual_numeric_profile_without_callbacks();
}
std::uint32_t BnwNormalNonFontPaintInvocation::material_selector_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.receiver.material_selector_without_callbacks();
}
std::uint16_t BnwNormalNonFontPaintInvocation::native_configuration_without_callbacks()const{
 const auto data=impl_;data->first();
 return static_cast<std::uint16_t>(nonfont_button_word(data->fields.receiver.receiver_region_without_callbacks(),0x40));
}
void BnwNormalNonFontPaintInvocation::require_button_suppliers_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto data=impl_;data->full(operation);const auto& current=data->fields.receiver;
 const auto region=current.receiver_region_without_callbacks();
 nonfont_button_need(region.owner&&region.bytes&&region.extent>=0x60,"Button actual118 backing absent");
 for(std::size_t lane=0;lane!=2;++lane){
  const auto source=current.source_region_without_callbacks(lane);
  const auto original=nonfont_button_word(region,lane==0?0x44:0x48);
  nonfont_button_need(bool(source)==(original!=0),"Button source44/48 null association mismatch");
  if(source)nonfont_button_need(source->owner&&source->bytes&&source->extent>=0x120&&
   source->native_base==original,"Button source object lost exact native44/48 backing");
  if(source)nonfont_button_need((nonfont_button_word(*source,0x11c)&0x7f800000u)!=0x7f800000u,
   "Button original source11C nonfinite value is not qualified");
 }
 const auto literals=current.arithmetic_literal_words_without_callbacks();
 // Root returns the actual sealed ONE F32 memory operand for each source.
 // Physical literal values stay external qualification; no expected-value test.
 nonfont_button_need((literals[1]&0x7f800000u)!=0x7f800000u,
  "Button nonfinite Original9DD390 FADD operand is not qualified");
 const auto numeric=data->fields.frame.actual_numeric_profile_without_callbacks();
 nonfont_button_need(((numeric[0]>>8)&3u)!=1&&(numeric[0]&0x3fu)==0x3fu,
  "Button Original reserved/unmasked x87 control requires another arithmetic supplier");
 (void)current.argument_words_without_callbacks();(void)current.fixed_slot_global_words_without_callbacks();
 const auto arguments=current.argument_words_without_callbacks();
 for(const auto value:arguments)nonfont_button_need((value&0x7f800000u)!=0x7f800000u,
  "Button nonfinite original argument arithmetic is not qualified");
 for(const std::size_t offset:{0x04,0x08,0x0c,0x10,0x24,0x28,0x50,0x54,0x58,0x5c})
  nonfont_button_need((nonfont_button_word(region,offset)&0x7f800000u)!=0x7f800000u,
   "Button nonfinite original float field is not qualified");
 nonfont_button_need((literals[0]&0x7f800000u)!=0x7f800000u,
  "Button original SUBSS literal is nonfinite");
 data->full(operation);data->first();
 data->fields.pool->require_record_available_without_callbacks();
 data->fields.slots->require_slot_and_release_available_without_callbacks();
}

BnwNormalNonFontOwnReturn::BnwNormalNonFontOwnReturn(Outcome outcome,const BnwNormalNonFontPaintInvocation& invocation,
 std::shared_ptr<const BnwNormalNonFontItemOccurrence> item,
 std::shared_ptr<const BnwNormalNonFontMaterialLoan> material)
 :outcome_(outcome),invocation_control_(invocation.impl_),root_(invocation.captured_root_without_callbacks()),
 control_(invocation.actual_control_without_callbacks()),ordinal_(invocation.actual_ordered_ordinal_without_callbacks()),
 numeric_(invocation.numeric_controls_without_callbacks()),creator_(std::this_thread::get_id()),
 item_(std::move(item)),material_(std::move(material)){
 nonfont_button_need((outcome_==Outcome::registered)==bool(item_)&&bool(item_)==bool(material_),
  "NonFont own-return registered association is incomplete");
}
void BnwNormalNonFontOwnReturn::require_physical_current_without_callbacks()const{
 nonfont_button_need(creator_==std::this_thread::get_id()&&invocation_control_,"NonFont original own-return storage lost");
 root_.require_physical_current_without_callbacks();
 nonfont_button_need((outcome_==Outcome::registered)==bool(item_)&&bool(item_)==bool(material_),
  "NonFont own-return outcome association changed");
 if(item_){const auto item=item_->capture_guard_without_callbacks();
  item.require_physical_current_without_callbacks();item.require_same_root_without_callbacks(root_);
  material_->require_physical_current_without_callbacks();material_->require_same_item_without_callbacks(item);}
 // Original active Invocation/Record/Slot are deliberately not queried.
 root_.require_physical_current_without_callbacks();
}
void BnwNormalNonFontOwnReturn::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 require_physical_current_without_callbacks();root_.require_source_owned(operation);
 require_physical_current_without_callbacks();
 if(item_){const auto item=item_->capture_guard_without_callbacks();item.require_source_owned(operation);
  require_physical_current_without_callbacks();material_->require_source_owned(operation);
  require_physical_current_without_callbacks();}
}
void BnwNormalNonFontOwnReturn::require_same_invocation_control_without_callbacks(
 const BnwNormalNonFontPaintInvocation& invocation)const{
 require_physical_current_without_callbacks();
 nonfont_button_need(invocation_control_.get()==invocation.impl_.get()&&
  !invocation_control_.owner_before(invocation.impl_)&&!invocation.impl_.owner_before(invocation_control_),
  "NonFont own-return is from a different original Invocation control");
}
BnwNormalNonFontOwnReturn::Outcome BnwNormalNonFontOwnReturn::outcome_without_callbacks()const{
 require_physical_current_without_callbacks();return outcome_;
}
std::uint64_t BnwNormalNonFontOwnReturn::actual_control_without_callbacks()const{
 require_physical_current_without_callbacks();return control_;
}
std::size_t BnwNormalNonFontOwnReturn::actual_ordered_ordinal_without_callbacks()const{
 require_physical_current_without_callbacks();return ordinal_;
}
BnwNormalRootBatchHistory::Guard BnwNormalNonFontOwnReturn::captured_root_without_callbacks()const{
 require_physical_current_without_callbacks();return root_;
}
std::shared_ptr<const BnwNormalNonFontItemOccurrence> BnwNormalNonFontOwnReturn::item_owned_without_callbacks()const{
 require_physical_current_without_callbacks();return item_;
}
std::shared_ptr<const BnwNormalNonFontMaterialLoan> BnwNormalNonFontOwnReturn::material_owned_without_callbacks()const{
 require_physical_current_without_callbacks();return material_;
}

struct BnwNormalNonFontMaterialLoan::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 const BnwNormalNonFontItemOccurrence::Guard item;
 const std::array<std::byte,0x64> descriptor;
 const std::array<std::byte,0x20> registration;
 const std::uint32_t index;
 explicit Impl(BnwNormalNonFontItemOccurrence::Guard original)
  :item(std::move(original)),descriptor(item.descriptor_without_callbacks()),
   registration(item.registration_without_callbacks()),index(item.index_without_callbacks()){}
 void first()const{
  nonfont_button_need(creator==std::this_thread::get_id(),"NonFont material foreign creator");
  item.require_physical_current_without_callbacks();
  nonfont_button_need(index==item.index_without_callbacks()&&descriptor==item.descriptor_without_callbacks()&&
   registration==item.registration_without_callbacks(),"NonFont material lost SAME immutable Item tuple");
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
  first();item.require_source_owned(operation);first();
 }
};
BnwNormalNonFontMaterialLoan::BnwNormalNonFontMaterialLoan(std::shared_ptr<const Impl> data)
 :impl_(std::move(data)){}
BnwNormalNonFontMaterialLoan::~BnwNormalNonFontMaterialLoan()=default;
std::shared_ptr<const BnwNormalNonFontMaterialLoan> BnwNormalNonFontMaterialLoan::capture_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontItemOccurrence::Guard& borrowed){
 const auto item=borrowed;item.require_physical_current_without_callbacks();item.require_source_owned(operation);
 item.require_physical_current_without_callbacks();
 const auto data=std::make_shared<const Impl>(item);item.require_physical_current_without_callbacks();data->full(operation);
 auto result=std::shared_ptr<const BnwNormalNonFontMaterialLoan>(new BnwNormalNonFontMaterialLoan(data));
 data->full(operation);return result;
}
void BnwNormalNonFontMaterialLoan::require_physical_current_without_callbacks()const{
 const auto data=impl_;nonfont_button_need(bool(data),"NonFont material absent");data->first();
}
void BnwNormalNonFontMaterialLoan::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto data=impl_;nonfont_button_need(bool(data),"NonFont material absent");data->full(operation);
}
void BnwNormalNonFontMaterialLoan::require_same_item_without_callbacks(const BnwNormalNonFontItemOccurrence::Guard& borrowed)const{
 const auto data=impl_;const auto other=borrowed;data->first();other.require_physical_current_without_callbacks();
 data->item.require_same_item_without_callbacks(other);data->first();other.require_physical_current_without_callbacks();
}
BnwNormalNonFontItemOccurrence::Guard BnwNormalNonFontMaterialLoan::captured_item_without_callbacks()const{
 const auto data=impl_;data->first();return data->item;
}

struct BnwNormalNonFontResourceLoan::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 const CapturedFields fields;
 explicit Impl(CapturedFields captured):fields(std::move(captured)){}
 void first()const{
  nonfont_button_need(creator==std::this_thread::get_id()&&fields.canonical_resource_owner&&
   fields.gpu_storage_owner&&fields.operation,"NonFont canonical resource/GPU keeper absent");
  fields.root.require_physical_current_without_callbacks();
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
  first();nonfont_button_need(&operation==fields.operation,"NonFont resource Source identity mismatch");
  fields.root.require_source_owned(operation);first();operation.require_operation(fields.serial);first();
  fields.root.require_source_owned(operation);first();
 }
};
BnwNormalNonFontResourceLoan::BnwNormalNonFontResourceLoan(std::shared_ptr<const Impl> data):impl_(std::move(data)){}
BnwNormalNonFontResourceLoan::~BnwNormalNonFontResourceLoan()=default;
std::shared_ptr<const BnwNormalNonFontResourceLoan> BnwNormalNonFontResourceLoan::capture_owned(
 const BnwNormalNonFontCaptureKey&,const runtime::BnwWorldViewSourceOperationLoan& operation,CapturedFields captured){
 // Only Root's genuine resource bank may bind SAME native resource/GPU owners
 // and qualified dimensions/helper result. No resource-ID-only constructor.
 Impl before(captured);before.full(operation);const auto data=std::make_shared<const Impl>(std::move(captured));
 before.first();before.full(operation);data->full(operation);
 auto result=std::shared_ptr<const BnwNormalNonFontResourceLoan>(new BnwNormalNonFontResourceLoan(data));
 data->full(operation);return result;
}
void BnwNormalNonFontResourceLoan::require_physical_without_callbacks()const{
 const auto data=impl_;nonfont_button_need(bool(data),"NonFont resource capsule absent");data->first();
}
void BnwNormalNonFontResourceLoan::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto data=impl_;nonfont_button_need(bool(data),"NonFont resource capsule absent");data->full(operation);
}
void BnwNormalNonFontResourceLoan::require_same_root_without_callbacks(
 const BnwNormalRootBatchHistory::Guard& borrowed)const{
 const auto data=impl_;const auto root=borrowed;nonfont_button_need(bool(data),"NonFont resource absent");
 data->first();root.require_physical_current_without_callbacks();data->fields.root.require_same_root_without_callbacks(root);
 data->first();root.require_physical_current_without_callbacks();
}
void BnwNormalNonFontResourceLoan::require_actual_resource_owner_without_callbacks(
 const std::shared_ptr<const void>& borrowed)const{
 const auto data=impl_;const auto owner=borrowed;nonfont_button_need(bool(data),"NonFont resource absent");data->first();
 const auto original=data->fields.canonical_resource_owner;
 nonfont_button_need(owner&&original.get()==owner.get()&&!original.owner_before(owner)&&!owner.owner_before(original),
  "NonFont resource canonical native owner differs");data->first();
}
std::uint32_t BnwNormalNonFontResourceLoan::resource_id_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.resource_id;
}
std::array<std::uint32_t,2> BnwNormalNonFontResourceLoan::dimension_words_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.dimensions;
}
std::uint32_t BnwNormalNonFontResourceLoan::mask_bit_count_without_callbacks()const{
 const auto data=impl_;data->first();return data->fields.mask_bit_count;
}

BnwNormalNonFontOwnReturn BnwNormalNonFontButtonProducer::prepare_button_own118_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontPaintInvocation& borrowed){
 // Capture physical controls before the first Source/callback boundary.
 const auto invocation=borrowed;
 invocation.require_current_without_callbacks();
 const auto receiver=invocation.captured_receiver_without_callbacks();
 const auto pool=invocation.native_pool_owned_without_callbacks();
 const auto bank=invocation.resource_bank_owned_without_callbacks();
 const auto backing=receiver.receiver_region_without_callbacks();
 const auto source44=receiver.source_region_without_callbacks(0);
 const auto source48=receiver.source_region_without_callbacks(1);
 const auto args=receiver.argument_words_without_callbacks();
 const auto fixed_globals=receiver.fixed_slot_global_words_without_callbacks(); // [global24,global20]
 const auto literals=receiver.arithmetic_literal_words_without_callbacks();
 const auto numeric=invocation.numeric_controls_without_callbacks();
 invocation.require_button_suppliers_owned(operation);invocation.require_current_without_callbacks();
 nonfont_button_need((invocation.flags_word40_without_callbacks()&0x10000000u)==0,
  "Button own118 is not selected by actual flags40");
 // Original F0 cursor side effects occur before returned INDEX, even INDEX0.
 auto record=BnwNormalNonFontRecordLoan::capture_owned(operation,invocation);
 record->require_current_owned(operation,invocation);
 const auto put=[&](std::uint32_t target,std::uint32_t source){
  record->write_word_without_callbacks(target,nonfont_button_word(backing,source));};
 put(0x00,0x24);put(0x04,0x28);
 record->write_word_without_callbacks(0x08,nonfont_button_subss(args[0],literals[0]));
 record->write_word_without_callbacks(0x0c,nonfont_button_subss(args[1],literals[0]));
 put(0x10,0x50);put(0x14,0x54);put(0x18,0x58);put(0x1c,0x5c);
 put(0x30,0x14);put(0x34,0x18);put(0x38,0x1c);put(0x3c,0x20);
 put(0x20,0x04);put(0x24,0x08);put(0x28,0x0c);put(0x2c,0x10);
 record->write_word_without_callbacks(0x4c,args[2]); // dereferenced arg3
 record->write_word_without_callbacks(0x50,args[4]); // dereferenced arg5
 record->write_word_without_callbacks(0x54,args[3]); // dereferenced arg4
 invocation.require_current_without_callbacks();
 auto slot=BnwNormalNonFontSlotLoan::capture_owned(operation,invocation);
 record->require_current_owned(operation,invocation);slot->require_current_owned(operation,invocation);
 if(slot->index_without_callbacks()==0){
  // Native8E6D60 has still cleared100bytes and stored41 in descriptor0.
  invocation.require_source_owned(operation);record->require_current_owned(operation,invocation);
  return BnwNormalNonFontOwnReturn(BnwNormalNonFontOwnReturn::Outcome::zero_index,invocation,nullptr,nullptr);
 }
 slot->write_word_without_callbacks(0x04,fixed_globals[0]);
 slot->write_word_without_callbacks(0x08,fixed_globals[1]);
 slot->write_word_without_callbacks(0x0c,2);slot->write_byte_without_callbacks(0x60,2);
 const auto configuration=static_cast<std::uint16_t>(nonfont_button_word(backing,0x3c));
 if(source44){
  slot->write_word16_without_callbacks(0x18,static_cast<std::uint16_t>(nonfont_button_word(*source44,0x10c)));
  slot->write_word16_without_callbacks(0x1a,static_cast<std::uint16_t>(nonfont_button_word(*source44,0x110)));
  slot->write_word16_without_callbacks(0x38,configuration);slot->write_word16_without_callbacks(0x3a,configuration);
 }else if(nonfont_button_word(backing,0x4c)!=0){
  slot->write_word16_without_callbacks(0x1a,static_cast<std::uint16_t>(nonfont_button_word(backing,0x4c)));
  slot->write_word16_without_callbacks(0x18,0);
  slot->write_word16_without_callbacks(0x38,configuration);slot->write_word16_without_callbacks(0x3a,configuration);
 }
 if(source48){
  slot->write_byte_without_callbacks(0x60,4);
  slot->write_word16_without_callbacks(0x1c,static_cast<std::uint16_t>(nonfont_button_word(*source48,0x10c)));
  slot->write_word16_without_callbacks(0x3c,configuration);
  slot->write_word16_without_callbacks(0x1e,static_cast<std::uint16_t>(nonfont_button_word(*source48,0x110)));
  slot->write_word16_without_callbacks(0x3e,configuration);
 }
 std::array<std::shared_ptr<const BnwNormalNonFontResourceLoan>,4> resources{};
 const auto cuts=[&]{invocation.require_source_owned(operation);
  record->require_current_owned(operation,invocation);slot->require_current_owned(operation,invocation);};
 const auto source_lane=[&](const BnwNormalNonFontNativeRegion& source,std::size_t lane,
  std::uint32_t scalar_offset,std::uint32_t mask_offset,std::uint32_t first_dimensions,
  std::uint32_t second_dimensions)->bool{
  cuts();const auto id1=nonfont_button_word(source,0x10c),id2=nonfont_button_word(source,0x110);
  resources[lane]=bank->resolve_owned(operation,id1);cuts();
  resources[lane+1]=bank->resolve_owned(operation,id2);cuts();
  if(!resources[lane]||!resources[lane+1])return false;
  const auto a=resources[lane],b=resources[lane+1];
  a->require_source_owned(operation);cuts();b->require_source_owned(operation);cuts();
  nonfont_button_need(a->resource_id_without_callbacks()==id1&&b->resource_id_without_callbacks()==id2,
   "Button ResourceBank returned wrong actual resource IDs");
  const auto mask_count=a->mask_bit_count_without_callbacks();
  const auto dims1=a->dimension_words_without_callbacks(),dims2=b->dimension_words_without_callbacks();
  record->write_word_without_callbacks(scalar_offset,nonfont_button_word(source,0x11c));
  const auto native_mask=(std::uint32_t{1}<<(mask_count&31u))-1u;
  record->write_word_without_callbacks(mask_offset,nonfont_button_cvtsi2ss(native_mask));
  record->write_word_without_callbacks(first_dimensions,nonfont_button_fild_fadd_fstp(dims1[0],literals[1],numeric[0]));
  record->write_word_without_callbacks(first_dimensions+4,nonfont_button_fild_fadd_fstp(dims1[1],literals[1],numeric[0]));
  record->write_word_without_callbacks(second_dimensions,nonfont_button_fild_fadd_fstp(dims2[0],literals[1],numeric[0]));
  record->write_word_without_callbacks(second_dimensions+4,nonfont_button_fild_fadd_fstp(dims2[1],literals[1],numeric[0]));
  cuts();return true;
 };
 if(source44){
  if(!source_lane(*source44,0,0x70,0x80,0x90,0xa0)){
   cuts();pool->cleanup_native_8eefe0_without_callbacks();invocation.require_current_without_callbacks();
   return BnwNormalNonFontOwnReturn(BnwNormalNonFontOwnReturn::Outcome::missing_resource,invocation,nullptr,nullptr);
  }
 }else record->write_word_without_callbacks(0x70,0); // only this explicit native clear
 if(source48&& !source_lane(*source48,2,0xb0,0xc0,0xd0,0xe0)){
  cuts();pool->cleanup_native_8eefe0_without_callbacks();invocation.require_current_without_callbacks();
  return BnwNormalNonFontOwnReturn(BnwNormalNonFontOwnReturn::Outcome::missing_resource,invocation,nullptr,nullptr);
 }
 // Absent source48 leaves B0..E4 ORIGINAL backing untouched.
 cuts();pool->cleanup_native_8eefe0_without_callbacks();invocation.require_current_without_callbacks();
 record->seal_complete_without_callbacks(invocation);
 const auto item=BnwNormalNonFontItemOccurrence::register_owned(operation,invocation,*record,*slot,resources);
 // LIFE register_owned performs the contiguous actual32-byte registration
 // THEN8E4D80/4530 release and seals Item only after both. Historical Item no
 // longer depends on the ACTIVE slot/current occupant. Do not release twice.
 const auto item_guard=item->capture_guard_without_callbacks();
 auto material=BnwNormalNonFontMaterialLoan::capture_owned(operation,item_guard);
 item_guard.require_source_owned(operation);invocation.require_current_without_callbacks();
 return BnwNormalNonFontOwnReturn(BnwNormalNonFontOwnReturn::Outcome::registered,invocation,item,std::move(material));
}
}
