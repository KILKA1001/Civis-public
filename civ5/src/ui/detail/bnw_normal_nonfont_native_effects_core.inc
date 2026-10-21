// Private implementation helpers. Include only inside the Graph LIFE TU.
// A closed genuine owner/Source/Invocation guard is required by every caller.
// These functions do not issue a loan, canonical owner or completion.
namespace {
void nonfont_native_need(bool value,const char* why){
    if(!value)throw std::runtime_error(why);
}
void nonfont_native_range(std::size_t extent,std::size_t offset,std::size_t length){
    nonfont_native_need(offset<=extent&&length<=extent-offset,
        "NonFont actual native owner range is unavailable");
}
std::size_t nonfont_native_index_offset(std::uint32_t index,std::uint32_t stride,
    std::uint32_t prefix){
    const auto wide=std::uint64_t(prefix)+std::uint64_t(index)*stride;
    nonfont_native_need(wide<=std::numeric_limits<std::size_t>::max(),
        "NonFont actual backing index exceeds physical address domain");
    return static_cast<std::size_t>(wide);
}
std::uint32_t nonfont_native_u32(const std::byte* base,std::size_t extent,std::size_t offset){
    nonfont_native_need(base!=nullptr,"NonFont actual native owner storage is absent");
    nonfont_native_range(extent,offset,4);
    std::uint32_t value;std::memcpy(&value,base+offset,4);return value;
}
std::uint16_t nonfont_native_u16(const std::byte* base,std::size_t extent,std::size_t offset){
    nonfont_native_need(base!=nullptr,"NonFont actual native owner storage is absent");
    nonfont_native_range(extent,offset,2);
    std::uint16_t value;std::memcpy(&value,base+offset,2);return value;
}
void nonfont_native_put32(std::byte* base,std::size_t offset,std::uint32_t value)noexcept{
    std::memcpy(base+offset,&value,4);
}
void nonfont_native_put16(std::byte* base,std::size_t offset,std::uint16_t value)noexcept{
    std::memcpy(base+offset,&value,2);
}
void nonfont_native_gate_available(const std::byte* manager,std::size_t extent){
    nonfont_native_need(manager!=nullptr,"NonFont actual manager storage absent");
    nonfont_native_range(extent,0,4);
#ifdef _WIN32
    static_assert(sizeof(LONG)==4,"Original manager gate is DWORD");
    nonfont_native_need(reinterpret_cast<std::uintptr_t>(manager)%alignof(LONG)==0,
        "NonFont original manager gate storage is unaligned");
    nonfont_native_need(nonfont_native_u32(manager,extent,0)==0,
        "NonFont original manager gate unavailable before native effects");
#else
    throw std::runtime_error("NonFont actual Windows manager gate supplier unavailable");
#endif
}
class NonFontNativeManagerGate final {
    std::byte* const manager_;
    bool held_=false;
public:
    NonFontNativeManagerGate(std::byte* manager,std::size_t extent):manager_(manager){
        nonfont_native_need(manager_!=nullptr,"NonFont actual manager storage absent");
        nonfont_native_range(extent,0,4);
#ifdef _WIN32
        static_assert(sizeof(LONG)==4,"Original manager gate is DWORD");
        nonfont_native_need(reinterpret_cast<std::uintptr_t>(manager_)%alignof(LONG)==0,
            "NonFont original manager gate storage is unaligned");
        // OWN Windows implementation of the actual acquire-shaped operation.
        // Root must earn this actual live word/import/domain. No callbacks or
        // project allocation occur while the gate is held.
        LONG old;
        do{old=InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(manager_),1,0);}
        while(old==1);
        if(old!=0)throw std::runtime_error("NonFont actual gate domain differs from0/1");
        held_=true;
#else
        throw std::runtime_error("NonFont actual Windows manager gate supplier unavailable");
#endif
    }
    NonFontNativeManagerGate(const NonFontNativeManagerGate&)=delete;
    NonFontNativeManagerGate&operator=(const NonFontNativeManagerGate&)=delete;
    ~NonFontNativeManagerGate()noexcept{
#ifdef _WIN32
        if(held_)*reinterpret_cast<volatile LONG*>(manager_)=0; // Actual MOV0 exit.
#endif
    }
};
struct NonFontNativeCursorCut final {
    std::uint32_t old_cursor,new_cursor,pointer30;
    std::size_t record_offset;
};
NonFontNativeCursorCut nonfont_native_prepare_f0(
    const std::byte* owner,std::size_t owner_extent,std::uint32_t actual_payload_base,
    std::size_t payload_extent){
    // 8EF140 uses originalarg1 (global6F0), not incoming ECX.
    const auto old=nonfont_native_u32(owner,owner_extent,0x40);
    nonfont_native_need(nonfont_native_u32(owner,owner_extent,0x24)==actual_payload_base,
        "NonFont original source-pool backing24 differs");
    nonfont_native_range(owner_extent,0x30,4);
    nonfont_native_range(owner_extent,0x4c,2);
    nonfont_native_range(payload_extent,old,0xf0);
    const auto next=old+std::uint32_t{0xf0};
    const auto pointer=actual_payload_base+old;
    nonfont_native_need(next>=old&&pointer>=actual_payload_base,
        "NonFont genuine cursor/pointer domain overflow");
    return {old,next,pointer,old};
}
void nonfont_native_publish_f0(std::byte* owner,const NonFontNativeCursorCut& cut)noexcept{
    // Literal native order; no memset/zero of the borrowed F0 record.
    nonfont_native_put32(owner,0x30,cut.pointer30);
    nonfont_native_put32(owner,0x40,cut.new_cursor);
    nonfont_native_put16(owner,0x4c,0xf0);
}
struct NonFontNativeFreshSlotCut final {
    std::uint32_t index,next_counter,prior_count;
    std::size_t count_offset;
    bool limit_return_zero;
};
NonFontNativeFreshSlotCut nonfont_native_prepare_fresh_slot(
    const std::byte* manager,std::size_t manager_extent){
    nonfont_native_need(nonfont_native_u32(manager,manager_extent,0xd014b8)==0,
        "NonFont native8E5820 recycle-pool supplier is not implemented");
    const auto index=nonfont_native_u32(manager,manager_extent,0x40);
    const auto next=index+std::uint32_t{1}; // Preserve native32bit candidate.
    if(next>=0x20000u)return {index,next,0,0,true};
    // No index0 substitution or count=1 assignment. Genuine backing domain
    // is a prerequisite; malformed wrapped IDs cannot authorize a write.
    const auto offset=nonfont_native_index_offset(index,4,0x44);
    const auto count=nonfont_native_u32(manager,manager_extent,offset);
    nonfont_native_need(count!=0xffffffffu,"NonFont native slot count domain overflow");
    return {index,next,count,offset,false};
}
std::uint32_t nonfont_native_issue_fresh_slot(
    std::byte* manager,const NonFontNativeFreshSlotCut& cut)noexcept{
    if(cut.limit_return_zero)return 0;
    nonfont_native_put32(manager,0x40,cut.next_counter);
    nonfont_native_put32(manager,cut.count_offset,cut.prior_count+1);
    return cut.index;
}
void nonfont_native_initialize_descriptor100(std::byte* descriptor)noexcept{
    // 8E6D60 issues these writes even when allocator returns scalar index0.
    // 0x64 means100 decimal bytes. This is not the separately borrowed F0.
    std::memset(descriptor,0,0x64);
    nonfont_native_put32(descriptor,0,0x41);
}
struct NonFontNativeRegistrationCut final {
    std::uint32_t old_count,new_count,payload_pointer;
    std::uint16_t payload_length;
    std::size_t entry_offset;
};
std::size_t nonfont_native_prepare_queue_entry(const std::byte* owner,
    std::size_t owner_extent,std::uint32_t actual_queue_base,std::size_t queue_extent){
    const auto count=nonfont_native_u32(owner,owner_extent,0x0c);
    nonfont_native_need(nonfont_native_u32(owner,owner_extent,0x14)==actual_queue_base,
        "NonFont original32-entry backing14 differs");
    nonfont_native_need(count!=0xffffffffu&&count<=0x07ffffffu,
        "NonFont actual32-entry count/stride domain overflow");
    const auto offset=std::size_t(count)*0x20;
    nonfont_native_range(queue_extent,offset,0x20);
    return offset;
}
NonFontNativeRegistrationCut nonfont_native_prepare_registration(
    const std::byte* owner,std::size_t owner_extent,std::uint32_t actual_queue_base,
    std::size_t queue_extent,std::uint32_t expected_payload_pointer){
    const auto count=nonfont_native_u32(owner,owner_extent,0x0c);
    const auto offset=nonfont_native_prepare_queue_entry(owner,owner_extent,
        actual_queue_base,queue_extent);
    const auto pointer=nonfont_native_u32(owner,owner_extent,0x30);
    const auto length=nonfont_native_u16(owner,owner_extent,0x4c);
    nonfont_native_need(pointer==expected_payload_pointer&&length==0xf0,
        "NonFont registration does not borrow SAME original F0 occurrence");
    return {count,count+1,pointer,length,offset};
}
void nonfont_native_register32(std::byte* owner,std::byte* entry,
    const NonFontNativeRegistrationCut& cut,std::uint32_t index,
    std::uint32_t actual_material_word,std::uint16_t actual_receiver_word40)noexcept{
    // 8EC210 leaves all other entry bytes untouched, including upper WORD8.
    nonfont_native_put32(entry,4,0xffffffffu);
    nonfont_native_put32(entry,0,index);
    nonfont_native_put16(entry,8,actual_receiver_word40);
    nonfont_native_put32(entry,0x0c,0);
    nonfont_native_put32(entry,0x14,cut.payload_pointer);
    nonfont_native_put16(entry,0x18,cut.payload_length);
    nonfont_native_put32(entry,0x10,actual_material_word);
    nonfont_native_put32(owner,0x0c,cut.new_count); // Last actual publication.
}
struct NonFontNativeReleaseCut final {
    std::uint32_t index,prior_count,next_count,bucket_count;
    std::size_t count_offset,bucket_offset,bucket_element_offset;
    bool zero_argument,recycle;
};
NonFontNativeReleaseCut nonfont_native_prepare_release(
    const std::byte* manager,std::size_t manager_extent,std::uint32_t index,
    std::uint32_t actual_bucket_base,std::size_t bucket_extent){
    if(index==0)return {0,0,0,0,0,0,0,true,false}; // 8E4530 early RET4.
    const auto offset=nonfont_native_index_offset(index,4,0x44);
    const auto prior=nonfont_native_u32(manager,manager_extent,offset);
    nonfont_native_need(prior!=0,"NonFont release lacks genuine positive indexed count");
    const auto next=prior-1;
    if(next)return {index,prior,next,0,offset,0,0,false,false};
    const auto selector=nonfont_native_u32(manager,manager_extent,0x80044);
    const auto bucket=nonfont_native_index_offset(selector,0x410,0x80048);
    nonfont_native_range(manager_extent,bucket,0x410);
    const auto used=nonfont_native_u32(manager,manager_extent,bucket+4);
    const auto capacity=nonfont_native_u32(manager,manager_extent,bucket+8);
    nonfont_native_need(used!=capacity,
        "NonFont native8E3360 zero-recycle bucket growth supplier is not implemented");
    nonfont_native_need(used<capacity&&used!=0xffffffffu,
        "NonFont actual zero-recycle bucket count/capacity domain invalid");
    nonfont_native_need(nonfont_native_u32(manager,manager_extent,bucket)==actual_bucket_base,
        "NonFont actual selected zero-recycle backing differs");
    const auto element=nonfont_native_index_offset(used,4,0);
    nonfont_native_range(bucket_extent,element,4);
    nonfont_native_need(actual_bucket_base!=0&&element<=0xffffffffu-actual_bucket_base,
        "NonFont actual zero-recycle writable pointer domain unavailable");
    return {index,prior,next,used,offset,bucket,element,false,true};
}
std::uint32_t nonfont_native_release_slot(std::byte* manager,std::byte* bucket_backing,
    const NonFontNativeReleaseCut& cut)noexcept{
    if(cut.zero_argument)return 0;
    // All required native dependencies/ranges were checked before decrement.
    nonfont_native_put32(manager,cut.count_offset,cut.next_count);
    if(cut.recycle){
        manager[cut.bucket_offset+0x40c]=std::byte{0};
        nonfont_native_put32(bucket_backing,cut.bucket_element_offset,cut.index);
        nonfont_native_put32(manager,cut.bucket_offset+4,cut.bucket_count+1);
    }
    return cut.next_count;
}
} // namespace
