#include "civ5/graphics/bnw_warrior_native_clip_time.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <span>
#include <vector>
using namespace civ5::graphics;
static void check(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
static std::uint32_t word(std::span<const std::uint8_t>b,std::size_t at){check(at+4<=b.size(),"Native clock fixture bounds");std::uint32_t n;std::memcpy(&n,b.data()+at,4);return n;}
template<class F>static void rejects(F f){bool bad=false;try{f();}catch(const std::invalid_argument&){bad=true;}check(bad,"Unsupported Warrior clock input accepted");}
int main(int argc,char**argv){try{
 check(argc==2,"Usage: native_clip_time NATIVE_CLIP_TIME_FIXTURE");std::ifstream f(argv[1],std::ios::binary);check(bool(f),"Native clip fixture missing");const std::vector<std::uint8_t> b{std::istreambuf_iterator<char>(f),{}};
 check(b.size()>12&&std::memcmp(b.data(),"BNWCLKT1",8)==0,"Native clock fixture schema");const auto count=word(b,8);check(count==5600&&b.size()==12+count*40,"Native clock fixture extent");
 bool negative_fix=false,late_clamp=false,signed_zero=false;
 for(unsigned i=0;i<count;++i){const auto at=12+i*40;
  const BnwWarriorPoseAnimTimeInput input{std::bit_cast<float>(word(b,at)),std::bit_cast<float>(word(b,at+4)),std::bit_cast<float>(word(b,at+8)),std::bit_cast<float>(word(b,at+12)),std::bit_cast<std::int32_t>(word(b,at+16)),std::bit_cast<std::int32_t>(word(b,at+20)),std::bit_cast<std::int32_t>(word(b,at+24))};
  const auto result=map_bnw_warrior_native_clip_time(input);
  check(std::bit_cast<std::uint32_t>(result.signed_local_clock)==word(b,at+28),"Source SSE/FPREM clock mismatch");check(std::bit_cast<std::uint32_t>(result.curve_local_clock)==word(b,at+32),"Source negative correction mismatch");check(unsigned(result.curve_boundary_wrap)==word(b,at+36),"Source ClampedLoop flags mismatch");
  negative_fix|=result.signed_local_clock<0&&result.curve_local_clock>=0;late_clamp|=input.loop_count>0&&result.curve_local_clock==input.duration;signed_zero|=result.curve_local_clock==0&&std::signbit(result.curve_local_clock);
 }
 check(negative_fix&&late_clamp&&signed_zero,"Native edge cases missing");
 BnwWarriorPoseAnimTimeInput good{1,0,1,std::bit_cast<float>(0x40c00001u),0,0,0};unsigned negative=0;
 for(unsigned field=0;field<4;++field){auto bad=good;switch(field){case 0:bad.graph_clock=std::numeric_limits<float>::quiet_NaN();break;case 1:bad.offset=std::numeric_limits<float>::infinity();break;case 2:bad.speed=std::numeric_limits<float>::quiet_NaN();break;case 3:bad.duration=std::numeric_limits<float>::infinity();break;}rejects([&]{map_bnw_warrior_native_clip_time(bad);});++negative;}
 for(float d:{0.f,-1.f,6.f}){auto bad=good;bad.duration=d;rejects([&]{map_bnw_warrior_native_clip_time(bad);});++negative;}
 auto bad=good;bad.graph_clock=std::numeric_limits<float>::max();bad.speed=2;rejects([&]{map_bnw_warrior_native_clip_time(bad);});++negative;
 bad=good;bad.loop_count=2147483647;bad.speed=std::numeric_limits<float>::max();rejects([&]{map_bnw_warrior_native_clip_time(bad);});++negative;
 std::cout<<"PASS"<<count<<" native SSE/FPREM/caller bit comparisons,"<<negative<<" missing/overflow refusals; standing graph/pose/animation owner not accepted\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
