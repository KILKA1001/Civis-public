#include "civ5/graphics/bnw_warrior_code_material.hpp"
#include "civ5/graphics/bnw_warrior_sampler_records.hpp"
#include <iostream>
#include <limits>
using namespace civ5::graphics;
static void check(bool x){if(!x)throw std::runtime_error("CODE material test failed");}
int main(){try{
 unsigned negatives=0;
 auto reject=[&](auto f){try{f();}catch(const std::invalid_argument&){++negatives;return;}throw std::runtime_error("Invalid CODE input accepted");};
 BnwWarriorInitialAlphaOwner owner(16385,7);
 for(unsigned i=0;i<32;++i)check(owner.member(16385,7,i)==std::array<float,3>{1,0,0});
 reject([&]{owner.member(16385,7,32);});reject([&]{owner.member(1,7,0);});reject([&]{owner.member(16385,8,0);});
 reject([]{BnwWarriorInitialAlphaOwner(-1,7);});reject([]{BnwWarriorInitialAlphaOwner(16385,0);});
 for(unsigned i=0;i<12;++i){const auto r=resolve_bnw_native_sampler_record(i),s=r;check(r==s);const auto w=bnw_native_sampler_state_writes(r);check(w[0][0]==1&&w[7][0]==11&&w[6][1]==r.max_anisotropy);}
 for(std::uint16_t i:{12,13,65535})reject([&]{resolve_bnw_native_sampler_record(i);});
 std::array<std::uint8_t,100> packet{};packet[0]=0x5a;packet[0x60]=6;
 for(unsigned i=0;i<6;++i)packet[0x38+2*i]=i;
 const auto records=resolve_bnw_warrior_packet_sampler_records(packet);for(unsigned i=0;i<6;++i)check(records[i]==bnw_native_sampler_records[i]);
 auto bad=packet;bad[0]=0x54;reject([&]{resolve_bnw_warrior_packet_sampler_records(bad);});bad=packet;bad[0x60]=5;reject([&]{resolve_bnw_warrior_packet_sampler_records(bad);});bad=packet;bad[0x38]=12;reject([&]{resolve_bnw_warrior_packet_sampler_records(bad);});reject([&]{resolve_bnw_warrior_packet_sampler_records(std::span(packet).first(99));});
 BnwWarriorMaterialSamples rgb{{.5,.6,.7,.5},{.3,.4,.5,.6},{.7,.6,.5,1},{.2,.3,.4,1},{.8,.7,.6,1},{.2,.75,.4,1},{.12,.15,.19},0};
 unsigned cases=0;for(float z:{0.f,1.f,2.f})for(float a:{0.f,.5f,1.f})for(float b:{0.f,.5f,1.f})for(float t:{0.f,.5f,1.f}){
     BnwWarriorCodeMaterialSamples v{rgb,{z,1.f,{a,b,t}}};const auto out=evaluate_bnw_warrior_code_material_samples(v),base=evaluate_bnw_warrior_material_samples(rgb);for(unsigned i=0;i<3;++i)check(out[i]==base[i]);check(std::abs(out[3]-(a+t*(b-a))*(z>=1?1.f:std::bit_cast<float>(std::uint32_t{0x3ea8f5c3})))<1e-7);++cases;
 }
 for(float x:{-1.f,2.f,std::numeric_limits<float>::quiet_NaN()})reject([&]{evaluate_bnw_warrior_code_material_samples({rgb,{1,1,{x,1,0}}});});
 reject([&]{evaluate_bnw_warrior_code_material_samples({rgb,{1,std::numeric_limits<float>::quiet_NaN(),{1,0,0}}});});
 check(bnw_warrior_code_material_fragment.find("uniform float fWaterHeight;")!=std::string::npos&&bnw_warrior_code_material_fragment.find("uniform vec3 AlphaValuesS;")!=std::string::npos);
 check(negatives==16);std::cout<<"PASS CODE PS "<<cases<<" CPU cases; 32 source-initial alpha triples; 12 sampler records; "<<negatives<<" negatives\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
