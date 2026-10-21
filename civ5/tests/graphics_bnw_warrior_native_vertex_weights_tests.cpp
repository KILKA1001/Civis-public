#include "civ5/graphics/bnw_warrior_native_vertex_weights.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
using namespace civ5::graphics;
static void check(bool b,const char*msg){if(!b)throw std::runtime_error(msg);}
template<class F>static void rejects(F f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Unknown vertex buffer accepted");}
int main(int argc,char**argv){try{
    check(argc==2,"Usage: native_vertex_weights NATIVE_DIFFERENTIAL_FIXTURE");
    std::ifstream input(argv[1],std::ios::binary);check(bool(input),"Fixture absent");
    const std::vector<std::uint8_t> raw{std::istreambuf_iterator<char>(input),{}};
    check(raw.size()>=12&&std::memcmp(raw.data(),"BNWWGT1\0",8)==0,"Fixture schema");
    std::uint32_t count=0;std::memcpy(&count,raw.data()+8,4);
    check(count==2497&&raw.size()==12+count*8,"Actual Warrior fixture extent");
    std::vector<std::uint8_t> stream(count*28);
    for(std::size_t i=0;i<count;++i){
        for(unsigned j=0;j<28;++j)stream[i*28+j]=std::uint8_t(i+j*17);
        std::array<std::uint8_t,4> a,b;
        std::memcpy(a.data(),raw.data()+12+i*8,4);std::memcpy(b.data(),raw.data()+16+i*8,4);
        check(reduce_bnw_warrior_native_weights(a).weights==b,"Native scalar differential mismatch");
        std::memcpy(stream.data()+i*28+8,a.data(),4);
    }
    const auto before=stream;const auto stats=postprocess_bnw_warrior_native_format4_weights(stream,count,4);
    check(stats.vertices==2497&&stats.zero_divisors==2,"Native zero-divisor behavior");
    for(std::size_t i=0;i<count;++i){
        check(std::memcmp(stream.data()+i*28+8,raw.data()+16+i*8,4)==0,"Native retained buffer differential mismatch");
        for(unsigned j=0;j<28;++j)if(j<8||j>=12)check(stream[i*28+j]==before[i*28+j],"Weights touched geometry/palette/UV");
    }
    const auto unchanged=stream;
    rejects([&]{postprocess_bnw_warrior_native_format4_weights(stream,count,3);});
    rejects([&]{postprocess_bnw_warrior_native_format4_weights(stream,count-1,4);});
    rejects([&]{postprocess_bnw_warrior_native_format4_weights(std::span(stream).first(stream.size()-1),count,4);});
    rejects([&]{postprocess_bnw_warrior_native_format4_weights({},std::numeric_limits<std::size_t>::max(),4);});
    std::array<std::uint8_t,68> sdk_stride{};rejects([&]{postprocess_bnw_warrior_native_format4_weights(sdk_stride,1,4);});
    check(stream==unchanged,"Rejected input mutated retained buffer");
    check(postprocess_bnw_warrior_native_format4_weights({},0,4).vertices==0,"Empty native buffer");
    const auto zero=reduce_bnw_warrior_native_weights({0,0,0,255});check(!zero.divisor_nonzero&&zero.weights[3]==255,"Zero divisor pretended native3 ready");
    check(reduce_bnw_warrior_native_weights({1,1,0,253}).weights==std::array<std::uint8_t,4>{128,128,255,0},"Native wrap was clamped");
    std::cout<<"PASS "<<count<<" exact native weight cases, retained packed buffer preservation, 5 rejected inputs; full optimized buffer unproven\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
