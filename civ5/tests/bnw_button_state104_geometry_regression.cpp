#include "civ5/runtime/lua_script_host.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>

//Arithmetic regression only. No constructor, Source, receiver, native32
//binding, state-setter return or CURRENT capability is issued by this test.
namespace civ5::ui {
static void require(bool condition,const char* message){
    if(!condition)throw runtime::ScriptError(message);
}
#include "../src/ui/detail/bnw_normal_button118_geometry_x87_core.inc"
#include "../src/ui/detail/bnw_normal_button_state104_x87_sse_core.inc"
namespace {
#if (defined(__GNUC__)||defined(__clang__)) && defined(__x86_64__) && !defined(_MSC_VER)
class ActualIndependentHardwareRounding final {
    const BnwButton118ActualX87Environment original_;
public:
    ActualIndependentHardwareRounding(unsigned x87_rounding,unsigned sse_rounding){
        original_.require_admitted_without_callbacks();
        require(x87_rounding<4&&sse_rounding<4,"Regression hardware rounding outside native domain");
        const auto cw=std::uint16_t((original_.control()&~0x0f00u)|0x0300u|(x87_rounding<<10));
        const auto mxcsr=(bnw_button_state104_actual_mxcsr_without_callbacks(original_)&~0x6000u)|
            0x1f80u|(sse_rounding<<13);
        __asm__ volatile("fldcw %0\n\tldmxcsr %1"::"m"(cw),"m"(mxcsr):"memory");
    }
    ~ActualIndependentHardwareRounding(){
        //Restore the real pre-test environment, including sticky exceptions.
        __asm__ volatile("fxrstor %0"::"m"(original_.bytes):"memory","st","st(1)","st(2)",
            "st(3)","st(4)","st(5)","st(6)","st(7)","xmm0","xmm1","xmm2","xmm3",
            "xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11",
            "xmm12","xmm13","xmm14","xmm15");
    }
    ActualIndependentHardwareRounding(const ActualIndependentHardwareRounding&)=delete;
};

void require_only_destination_words_changed(const std::array<std::byte,0x60>& before,
    const std::array<std::byte,0x60>& after){
    for(std::size_t offset=0;offset!=before.size();++offset)
        if(offset<0x34u||offset>=0x3cu)
            require(before[offset]==after[offset],"State geometry altered an unrelated physical word");
}

void original_unsigned_conversion_uses_x87_rounding_case(){
    //The actual conversion rounds up in x87 while subsequent SSE rounds
    //down. A host unsigned->float DTO conversion through SSE gives16777216,
    //whereas original FILD/FSTP produces exactly16777218 (4B800001).
    ActualIndependentHardwareRounding control(2,1);
    std::array<std::byte,0x60> bytes;bytes.fill(std::byte{0xcd});const auto before=bytes;
    const std::array<std::uint32_t,4> inputs{0u,0u,0x3f800000u,0x3f800000u};
    bnw_button118_execute_actual_6b363a_state_geometry_owned(bytes,0x01000001u,inputs);
    require(bnw_button118_actual_word_data(bytes,0x38)==0x4b800001u&&
        bnw_button118_actual_word_data(bytes,0x34)==0x4b800001u,
        "State104 conversion used SSE/default rounding instead of genuine x87 rounding");
    require_only_destination_words_changed(before,bytes);
}

void original_unsigned_high_bit_correction_case(){
    ActualIndependentHardwareRounding control(0,1); //x87 nearest; SSE down.
    std::array<std::byte,0x60> bytes;bytes.fill(std::byte{0xcd});const auto before=bytes;
    const std::array<std::uint32_t,4> inputs{0u,0u,0x3f800000u,0x3f800000u};
    bnw_button118_execute_actual_6b363a_state_geometry_owned(bytes,0xffffffffu,inputs);
    require(bnw_button118_actual_word_data(bytes,0x38)==0x4f800000u&&
        bnw_button118_actual_word_data(bytes,0x34)==0x4f800000u,
        "State104 high-bit conversion lost actual signedFILD/FADD2^32/FSTP sequence");
    require_only_destination_words_changed(before,bytes);
}

void original_axis_fields_remain_distinct_case(){
    ActualIndependentHardwareRounding control(0,0);
    std::array<std::byte,0x60> bytes;bytes.fill(std::byte{0xcd});const auto before=bytes;
    const std::array<std::uint32_t,4> inputs{0x41000000u,0x41800000u,0x40000000u,0x40800000u};
    bnw_button118_execute_actual_6b363a_state_geometry_owned(bytes,3u,inputs);
    require(bnw_button118_actual_word_data(bytes,0x38)==0x41e00000u&&
        bnw_button118_actual_word_data(bytes,0x34)==0x41600000u,
        "Original state geometry swapped outer108/10C/110/114 or physical150/14C");
    require_only_destination_words_changed(before,bytes);
}
#endif
}
int button_state104_geometry_regression(){
#if (defined(__GNUC__)||defined(__clang__)) && defined(__x86_64__) && !defined(_MSC_VER)
    original_unsigned_conversion_uses_x87_rounding_case();
    original_unsigned_high_bit_correction_case();
    original_axis_fields_remain_distinct_case();
    return 0;
#else
    throw runtime::ScriptError("State104 geometry regression needs the actual GNU x64 x87/SSE backend");
#endif
}
}
int main(){
    try{return civ5::ui::button_state104_geometry_regression();}
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
