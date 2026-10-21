#include "civ5/runtime/lua_script_host.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

//Arithmetic-only regression. It issues no Button actor, Source, native32
//address, Init-return, Invocation or CURRENT capability.
namespace civ5::ui {
static void require(bool condition,const char* message){
    if(!condition)throw runtime::ScriptError(message);
}
#include "../src/ui/detail/bnw_normal_button118_geometry_x87_core.inc"

namespace {
#if (defined(__GNUC__)||defined(__clang__)) && \
    (defined(__i386__)||defined(__x86_64__)) && !defined(_MSC_VER)
class ActualHardwareControl final {
    std::uint16_t original_;
public:
    ActualHardwareControl(unsigned precision,unsigned rounding){
        const BnwButton118ActualX87Environment actual;actual.require_admitted_without_callbacks();
        original_=actual.control();
        const auto selected=std::uint16_t((original_&~0x0f00u)|(precision<<8)|(rounding<<10));
        __asm__ volatile("fldcw %0"::"m"(selected):"memory");
    }
    ~ActualHardwareControl(){__asm__ volatile("fldcw %0"::"m"(original_):"memory");}
    ActualHardwareControl(const ActualHardwareControl&)=delete;
};

void word(std::array<std::byte,0x60>& bytes,std::uint32_t offset,std::uint32_t value){
    for(std::uint32_t lane=0;lane!=4;++lane)bytes[offset+lane]=std::byte((value>>(8u*lane))&0xffu);
}

void actual_unsigned_dimension_precision_case(unsigned precision,std::uint32_t expected){
    ActualHardwareControl control(precision,2); //Actual hardware RC=up.
    std::array<std::byte,0x60> bytes{};
    for(const auto offset:{0x34u,0x38u,0x2cu,0x30u})word(bytes,offset,0x3f800000u);
    const auto before=bytes;
    bnw_button118_execute_actual_5c7b50_nonnull_owned(bytes,0xffffffffu,0xffffffffu);
    for(const auto offset:{0x58u,0x5cu,0x50u,0x54u})
        require(bnw_button118_actual_word_data(bytes,offset)==expected,
            "Unsigned FILD/FADD+2^32/FDIVR/FSTP ignored actual PC/RC precision");
    for(std::size_t offset=0;offset!=0x50;++offset)
        require(bytes[offset]==before[offset],"Actual UV backend changed a non-output physical byte");
}

void actual_ordered_axes_and_domain_case(){
    ActualHardwareControl control(3,0); //Actual extended PC, nearest RC.
    std::array<std::byte,0x60> bytes{};
    word(bytes,0x34,0x41000000u); //originX8, source width8.
    word(bytes,0x38,0x41800000u); //originY16, source height4.
    word(bytes,0x2c,0x40000000u); //sizeX2.
    word(bytes,0x30,0x40800000u); //sizeY4.
    bnw_button118_execute_actual_5c7b50_nonnull_owned(bytes,4u,8u);
    require(bnw_button118_actual_word_data(bytes,0x58)==0x3f800000u&&
        bnw_button118_actual_word_data(bytes,0x5c)==0x40800000u&&
        bnw_button118_actual_word_data(bytes,0x50)==0x3e800000u&&
        bnw_button118_actual_word_data(bytes,0x54)==0x3f800000u,
        "Actual native114 height/118 width or origin/size UV destinations were transposed");
    const auto before=bytes;bool refused=false;
    try{bnw_button118_execute_actual_5c7b50_nonnull_owned(bytes,0u,8u);}
    catch(const runtime::ScriptError&){refused=true;}
    require(refused&&bytes==before,"Unsupported zero source dimension admitted or changed reached stores");
}
#endif
}

int run_actual_button118_x87_regression(){
#if (defined(__GNUC__)||defined(__clang__)) && \
    (defined(__i386__)||defined(__x86_64__)) && !defined(_MSC_VER)
    //For UINT32_MAX denominator with RCup: PC24 FADD rounds it to2^32,
    //so quotient1/2^32 is exact. PC64 retains2^32-1; positive quotient is
    //strictly above2^-32, whose upward F32 rounding is the next word.
    //These two independent exact arithmetic facts distinguish this body
    //from a final C++ binary64/long-double DTO copied into native bytes.
    actual_unsigned_dimension_precision_case(0,0x2f800000u);
    actual_unsigned_dimension_precision_case(3,0x2f800001u);
    actual_ordered_axes_and_domain_case();return 0;
#else
    throw runtime::ScriptError("Button118 x87 hardware regression backend unavailable");
#endif
}
}

int main(){
    try{return civ5::ui::run_actual_button118_x87_regression();}
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
