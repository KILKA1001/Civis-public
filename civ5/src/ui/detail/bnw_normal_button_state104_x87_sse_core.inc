// Included after the earned Button118 actual x87 environment definition.
// Only the SAME Button-owned actor calls this arithmetic body. No Source,
// CURRENT, native32 address or tail-return capability is constructed here.
namespace {
#if (defined(__GNUC__)||defined(__clang__)) && \
    (defined(__i386__)||defined(__x86_64__)) && !defined(_MSC_VER)
std::uint32_t bnw_button_state104_actual_mxcsr_without_callbacks(
    const BnwButton118ActualX87Environment& environment)noexcept{
    const auto& bytes=environment.bytes;
    return std::uint32_t(bytes[24])|(std::uint32_t(bytes[25])<<8)|
        (std::uint32_t(bytes[26])<<16)|(std::uint32_t(bytes[27])<<24);
}
#endif

void bnw_button118_execute_actual_6b363a_state_geometry_owned(
    std::array<std::byte,0x60>& actual_receiver,std::uint32_t actual_state104,
    const std::array<std::uint32_t,4>& actual_outer108_10c_110_114){
    //Each source word is copied only after the supplier's SAME physical
    //receiver checks. No callback or XML/DTO parser runs in this body.
    for(const auto word:actual_outer108_10c_110_114)
        require((word&0x7f800000u)!=0x7f800000u,
            "Button6B363A actual outer geometry input outside admitted finite F32 domain");
#if (defined(__GNUC__)||defined(__clang__)) && \
    (defined(__i386__)||defined(__x86_64__)) && !defined(_MSC_VER)
    const BnwButton118ActualX87Environment before;before.require_admitted_without_callbacks();
    const auto control=before.control();
    const auto mxcsr=bnw_button_state104_actual_mxcsr_without_callbacks(before);
    require((mxcsr&0x1f80u)==0x1f80u,
        "Button6B363A actual SSE requires six masked exceptions");
    const auto signed_state=std::bit_cast<std::int32_t>(actual_state104);
    //Actual9DD390 was qualified as physical4F800000. FADD and FSTP use
    //the current real x87 PC/RC; subsequent MULSS/ADDSS use current MXCSR.
    const std::uint32_t correction_word=0x4f800000u;
    std::uint32_t native_stack14,native_stack0c,native_stack08;
    auto*const receiver=actual_receiver.data();
    __asm__ volatile(
        "movss %[origin_x],%%xmm0\n\t"
        "movss %[origin_y],%%xmm3\n\t"
        "fildl %[signed_state]\n\t"
        "testl %[state_word],%[state_word]\n\t"
        "jge .Lbnw_state104_%=\n\t"
        "fadds %[correction]\n\t"
        ".Lbnw_state104_%=:\n\t"
        "movss %[increment_y],%%xmm2\n\t"
        "fstps %[native_stack14]\n\t"
        "movss %[native_stack14],%%xmm4\n\t"
        "movss %[increment_x],%%xmm1\n\t"
        "mulss %%xmm4,%%xmm2\n\t"
        "addss %%xmm3,%%xmm2\n\t"
        "mulss %%xmm4,%%xmm1\n\t"
        "movss %%xmm2,%[native_stack0c]\n\t"
        "addss %%xmm1,%%xmm0\n\t"
        "movss %%xmm0,%[native_stack08]\n\t"
        //Original6B368A writes150 before6B3696 writes14C. The SAME
        //embedded118 receiver stores actual words38 then34, directly.
        "movl %[native_stack0c],%%eax\n\t"
        "movl %%eax,0x38(%[receiver])\n\t"
        "movl %[native_stack08],%%eax\n\t"
        "movl %%eax,0x34(%[receiver])"
        :[native_stack14]"=m"(native_stack14),[native_stack0c]"=m"(native_stack0c),
         [native_stack08]"=m"(native_stack08)
        :[receiver]"r"(receiver),[signed_state]"m"(signed_state),[state_word]"r"(actual_state104),
         [correction]"m"(correction_word),[origin_x]"m"(actual_outer108_10c_110_114[0]),
         [origin_y]"m"(actual_outer108_10c_110_114[1]),
         [increment_x]"m"(actual_outer108_10c_110_114[2]),
         [increment_y]"m"(actual_outer108_10c_110_114[3])
        :"memory","cc","eax","xmm0","xmm1","xmm2","xmm3","xmm4","st");
    const BnwButton118ActualX87Environment after;
    require(after.control()==control&&after.empty()&&
        (bnw_button_state104_actual_mxcsr_without_callbacks(after)&~0x3fu)==(mxcsr&~0x3fu),
        "Button6B363A completed actual body changed x87/SSE control or x87 stack");
    //Failures after actual stores retain the physical partial state. No
    //replacement result or successful prefix is returned on overflow.
    for(const auto offset:{0x38u,0x34u})
        require((bnw_button118_actual_word_data(actual_receiver,offset)&0x7f800000u)!=0x7f800000u,
            "Button6B3696 actual output outside admitted finite F32 domain; physical stores retained");
#else
    (void)actual_receiver;(void)actual_state104;(void)actual_outer108_10c_110_114;
    throw runtime::ScriptError("Button6B363A requires genuine GNU x86/x64 x87/SSE assembly backend");
#endif
}
}
