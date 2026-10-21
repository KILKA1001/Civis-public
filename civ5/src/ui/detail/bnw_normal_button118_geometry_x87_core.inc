// Closed arithmetic backend for physically qualified6F6C2D/30 and5C7B50.
// Only the SAME Button-owned actor calls this leaf. This is no Source/CURRENT
// or native32-address issuer; hardware executes the admitted native x87 body.
namespace {
#if (defined(__GNUC__)||defined(__clang__)) && \
    (defined(__i386__)||defined(__x86_64__)) && !defined(_MSC_VER)
struct alignas(16) BnwButton118ActualX87Environment final {
    std::array<unsigned char,512> bytes;
    BnwButton118ActualX87Environment()noexcept{
        __asm__ volatile("fxsave %0":"=m"(bytes)::"memory");
    }
    std::uint16_t control()const noexcept{
        return std::uint16_t(bytes[0])|(std::uint16_t(bytes[1])<<8);
    }
    std::uint16_t status()const noexcept{
        return std::uint16_t(bytes[2])|(std::uint16_t(bytes[3])<<8);
    }
    bool empty()const noexcept{return bytes[4]==0;}
    void require_admitted_without_callbacks()const{
        require(empty()&&(control()&0x003fu)==0x003fu&&
            ((control()>>8)&3u)!=1u&&(status()&0x0080u)==0,
            "Button118 actual x87 requires empty bank, six masked exceptions, valid PC and no pending unmasked error");
    }
};
#endif

std::uint32_t bnw_button118_actual_word_data(const std::array<std::byte,0x60>& words,std::uint32_t offset){
    std::uint32_t result=0;
    for(std::uint32_t lane=0;lane!=4;++lane)
        result|=std::uint32_t(std::to_integer<unsigned char>(words[offset+lane]))<<(8u*lane);
    return result;
}

void bnw_button118_execute_actual_6f6c2d_increment_owned(
    const float& actual_source34,float& actual_destination114){
    require((std::bit_cast<std::uint32_t>(actual_source34)&0x7f800000u)!=0x7f800000u,
        "Button6F6C2D actual increment operand outside admitted finite F32 domain");
#if (defined(__GNUC__)||defined(__clang__)) && \
    (defined(__i386__)||defined(__x86_64__)) && !defined(_MSC_VER)
    const BnwButton118ActualX87Environment before;before.require_admitted_without_callbacks();
    const auto control=before.control();
    //Actual FLD source34 then FSTP destination114, preserving real PC/RC and
    //sticky status. No host floating division, default CW or state restore.
    __asm__ volatile("flds %[source]\n\tfstps %[destination]"
        :[destination]"=m"(actual_destination114):[source]"m"(actual_source34):"memory","st");
    const BnwButton118ActualX87Environment after;
    require(after.control()==control&&after.empty(),
        "Button6F6C2D actual x87 control/stack changed outside completed FLD/FSTP");
#else
    (void)actual_source34;(void)actual_destination114;
    throw runtime::ScriptError("Button6F6C2D requires genuine GNU x86/x64 x87 assembly backend");
#endif
}

void bnw_button118_execute_actual_5c7b50_nonnull_owned(
    std::array<std::byte,0x60>& actual_receiver,std::uint32_t actual_height114,std::uint32_t actual_width118){
    require(actual_height114&&actual_width118,
        "Button5C7B50 selected source114/118 zero dimension outside admitted domain");
    //The actor checked coverage for these four inputs before this private
    //call. No other unwritten byte is read, copied or serialized.
    for(const auto offset:{0x34u,0x38u,0x2cu,0x30u})
        require((bnw_button118_actual_word_data(actual_receiver,offset)&0x7f800000u)!=0x7f800000u,
            "Button5C7B50 actual physical numerator outside admitted finite F32 domain");
#if (defined(__GNUC__)||defined(__clang__)) && \
    (defined(__i386__)||defined(__x86_64__)) && !defined(_MSC_VER)
    const BnwButton118ActualX87Environment before;before.require_admitted_without_callbacks();
    const auto control=before.control();
    const auto signed_height=std::bit_cast<std::int32_t>(actual_height114);
    const auto signed_width=std::bit_cast<std::int32_t>(actual_width118);
    //Physical original9DD390 is qualified4F800000, not a float64 operand.
    const std::uint32_t correction_word=0x4f800000u;
    auto* const receiver=actual_receiver.data();
    //Both source dimensions belong to one immutable genuine keeper. The
    //second pair reuses that SAME object's words after the first two stores;
    //no callback can change source44 or114/118 during this actual body.
    //Native unsigned conversion is FILD signed32 + conditional FADD2^32.
    //Height quotient remains on ST(1) while width quotient uses ST(0).
    //FSTP writes the physical SAME receiver directly in original order:
    //58,5C,50,54. PC/RC act at FADD/FDIVR/FSTP exactly as actual x87 CW says.
    __asm__ volatile(
        "fildl %[height]\n\t"
        "testl %[height_word],%[height_word]\n\t"
        "jns .Lbnw_h1_%=\n\t"
        "fadds %[correction]\n\t"
        ".Lbnw_h1_%=:\n\t"
        "fdivrs 0x38(%[receiver])\n\t"
        "fildl %[width]\n\t"
        "testl %[width_word],%[width_word]\n\t"
        "jns .Lbnw_w1_%=\n\t"
        "fadds %[correction]\n\t"
        ".Lbnw_w1_%=:\n\t"
        "fdivrs 0x34(%[receiver])\n\t"
        "fstps 0x58(%[receiver])\n\t"
        "fstps 0x5c(%[receiver])\n\t"
        "fildl %[height]\n\t"
        "testl %[height_word],%[height_word]\n\t"
        "jns .Lbnw_h2_%=\n\t"
        "fadds %[correction]\n\t"
        ".Lbnw_h2_%=:\n\t"
        "fdivrs 0x30(%[receiver])\n\t"
        "fildl %[width]\n\t"
        "testl %[width_word],%[width_word]\n\t"
        "jns .Lbnw_w2_%=\n\t"
        "fadds %[correction]\n\t"
        ".Lbnw_w2_%=:\n\t"
        "fdivrs 0x2c(%[receiver])\n\t"
        "fstps 0x50(%[receiver])\n\t"
        "fstps 0x54(%[receiver])"
        :
        :[receiver]"r"(receiver),[height]"m"(signed_height),[width]"m"(signed_width),
         [height_word]"r"(actual_height114),[width_word]"r"(actual_width118),[correction]"m"(correction_word)
        :"memory","cc","st","st(1)");
    const BnwButton118ActualX87Environment after;
    require(after.control()==control&&after.empty(),
        "Button5C7B50 completed hardware body changed actual x87 control/stack");
    for(const auto offset:{0x58u,0x5cu,0x50u,0x54u})
        require((bnw_button118_actual_word_data(actual_receiver,offset)&0x7f800000u)!=0x7f800000u,
            "Button5C7B50 actual result outside admitted finite output domain; physical stores retained");
#else
    (void)actual_receiver;(void)actual_height114;(void)actual_width118;
    throw runtime::ScriptError("Button5C7B50 requires genuine GNU x86/x64 x87 assembly backend");
#endif
}
}
