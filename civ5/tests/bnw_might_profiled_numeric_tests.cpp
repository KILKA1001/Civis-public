#include "../src/runtime/bnw_might_profiled_numeric.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
using namespace civ5::runtime::diagnostic;
namespace {
constexpr auto p24 = MightPrecision::pc24;
constexpr auto p53 = MightPrecision::pc53;
constexpr auto p64 = MightPrecision::pc64;
static_assert(might_multiply_truncate(1000, 0x3f8147aeu, p24).value == 1010);
static_assert(might_multiply_truncate(1000, 0x3f8147aeu, p53).value == 1009);
static_assert(might_multiply_truncate(1000, 0x3f8147aeu, p64).value == 1009);
const char *name(MightNumericError e) {
    switch(e) {
    case MightNumericError::none: return "none";
    case MightNumericError::invalid_precision: return "invalid_precision";
    case MightNumericError::negative_total: return "negative_total";
    case MightNumericError::negative_multiplier: return "negative_multiplier";
    case MightNumericError::nonfinite_multiplier: return "nonfinite_multiplier";
    case MightNumericError::int32_overflow: return "int32_overflow";
    }
    throw std::runtime_error("unknown diagnostic numeric error");
}
void need(bool v, const char *s) { if (!v) throw std::runtime_error(s); }
void tests() {
    const auto eq=[](std::int32_t u,std::uint32_t b,MightPrecision p,std::int32_t expected) {
        const auto r=might_multiply_truncate(u,b,p);
        need(r.has_value() && r.value==expected,"profiled numeric independent golden differs");
    };
    const auto refuse=[](std::int32_t u,std::uint32_t b,MightPrecision p,MightNumericError expected) {
        const auto r=might_multiply_truncate(u,b,p);
        need(!r.has_value() && r.error==expected,"profiled numeric refusal differs");
    };
    for(auto p:{p24,p53,p64}) {
        eq(0,0x40000000,p,0);eq(24,0x3f800000,p,24);
        eq(22,0x40000000,p,44);eq(64,0x40000000,p,128);
        eq(1000,0x3f8147ae,p,p==p24?1010:1009);
        eq(2147483647,1,p,0); // smallest binary32 subnormal, exact float operand
        eq(2147483647,0x00800000,p,0); // smallest normal
        eq(2147483647,0,p,0);
        refuse(-1,0x3f800000,p,MightNumericError::negative_total);
        refuse(1,0xbf800000,p,MightNumericError::negative_multiplier);
        refuse(1,0x80000000,p,MightNumericError::negative_multiplier);
        for(auto b:{0x7f800000u,0xff800000u,0x7fc00001u,0x7f800001u})
            refuse(0,b,p,MightNumericError::nonfinite_multiplier);
        refuse(1,0x7f7fffff,p,MightNumericError::int32_overflow);
        refuse(1073741824,0x40000000,p,MightNumericError::int32_overflow);
        refuse(2147483647,0x40000000,p,MightNumericError::int32_overflow);
    }
    // PC24 FILD exact-int is followed by multiplication rounding, ties-to-even.
    eq(16777217,0x3f800000,p24,16777216);
    eq(16777219,0x3f800000,p24,16777220);
    // Premature int->float rounding would wrongly produce16777218 here.
    eq(16777217,0x3f800001,p24,16777220);
    eq(16777217,0x3f800001,p53,16777219);
    eq(16777217,0x3f800001,p64,16777219);
    eq(2147483583,0x3f800000,p24,2147483520);
    refuse(2147483584,0x3f800000,p24,MightNumericError::int32_overflow);
    eq(16777217,0x3f800000,p53,16777217);
    eq(16777219,0x3f800000,p64,16777219);
    eq(2147483647,0x3f800000,p53,2147483647);
    eq(2147483647,0x3f800000,p64,2147483647);
    refuse(2147483647,0x3f800000,p24,MightNumericError::int32_overflow);
    refuse(1,0x3f800000,static_cast<MightPrecision>(0),MightNumericError::invalid_precision);
}
}
int main(int argc,char **argv) try {
    if(argc==1) { tests();std::cout<<"PASS diagnostic original-product goldens, ties, subnormals and checked refusals\n";return 0; }
    need(argc==2 && std::string_view(argv[1])=="--stream","only --stream accepted");
    std::uint64_t id=0,bits=0;unsigned pc=0;std::int64_t units=0;
    while(std::cin>>id>>pc>>units>>bits) {
        need(units>=INT32_MIN && units<=INT32_MAX && bits<=UINT32_MAX,"numeric stream operand width differs");
        const auto r=might_multiply_truncate(static_cast<std::int32_t>(units),static_cast<std::uint32_t>(bits),static_cast<MightPrecision>(pc));
        std::cout<<id<<(r.has_value()?" ok ":" refused ");
        if(r.has_value())std::cout<<r.value;else std::cout<<name(r.error);
        std::cout<<'\n';
    }
    need(std::cin.eof(),"malformed diagnostic numeric stream");
    return 0;
} catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
