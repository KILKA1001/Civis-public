#include "civ5/graphics/bnw_warrior_native_vertex_consumer.hpp"
#include <fstream>
#include <iostream>
using namespace civ5::graphics;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>static void rejects(F f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Invalid native vertex was accepted");}
static std::uint32_t word(std::span<const std::uint8_t> b,std::size_t at){check(at+4<=b.size(),"Fixture bounds");std::uint32_t n;std::memcpy(&n,b.data()+at,4);return n;}
int main(int argc,char**argv){try{
 check(argc==2,"Usage: native_vertex_consumer NATIVE_PACKED_FIXTURE");std::ifstream file(argv[1],std::ios::binary);check(bool(file),"Fixture absent");const std::vector<std::uint8_t> fixture{std::istreambuf_iterator<char>(file),{}};
 check(fixture.size()>12&&std::memcmp(fixture.data(),"BNWPACK1",8)==0&&word(fixture,8)==3,"Native packed fixture schema");
 BnwWarriorNativePaletteRows rows{};for(unsigned b=0;b<20;++b)for(unsigned r=0;r<3;++r){rows[b*12+r*4+r]=float(b+1);rows[b*12+r*4+3]=float(10+b*3+r);}
 std::size_t at=12,vertices=0;
 for(unsigned model=0;model<3;++model){const auto count=word(fixture,at);at+=4;check(at+count*124<=fixture.size(),"Packed fixture extent");const std::span<const std::uint8_t> raw(fixture.data()+at,count*68);at+=count*68;at+=count*28;const std::span<const std::uint8_t> native(fixture.data()+at,count*28);at+=count*28;
  const auto converted=convert_bnw_warrior_native_format4_vertices(raw,count,68,bnw_warrior_sdk_vertex_declaration);check(std::equal(converted.bytes.begin(),converted.bytes.end(),native.begin()),"Native wholepacked mismatch");validate_bnw_warrior_native_format4(native,count);
  for(unsigned v=0;v<count;++v){const auto result=consume_bnw_warrior_native_vertex(std::span<const std::uint8_t,28>(native.data()+v*28,28),rows);for(float f:result.world)check(std::isfinite(f),"Actual format4 world nonfinite");for(float f:result.raw_normal)check(std::isfinite(f),"Actual format4 normal nonfinite");}vertices+=count;
 }
 check(at==fixture.size()&&vertices==2488,"Actual native roster count");
 std::array<std::uint8_t,28> v{};const auto half=[&](unsigned offset,float f){const auto h=bnw_warrior_native_real16_bits(std::bit_cast<std::uint32_t>(f));std::memcpy(v.data()+offset,&h,2);};
 half(0,1);half(2,2);half(4,3);half(16,1);v[8]=255;
 auto result=consume_bnw_warrior_native_vertex(v,rows);check(result.world==std::array<float,3>{11,13,15},"Source padded w0 lost translation/xyz1");check(result.raw_normal==std::array<float,3>{1,0,0},"Normal included translation");
 v[8]=128;v[9]=0;v[10]=127;v[14]=19;result=consume_bnw_warrior_native_vertex(v,rows);check(std::abs(result.world[0]-11.f*128/255)<1e-6,"Native nested third guard lost when second zero");
 v[8]=128;v[9]=128;v[10]=255;v[13]=1;v[14]=2;result=consume_bnw_warrior_native_vertex(v,rows);const float expected=11.f*(128.f/255)+15.f*(128.f/255)+19;
 check(std::abs(result.world[0]-expected)<1e-5,"Native wrapped residual/excessive bytes clamped or normalized");check(result.raw_normal[0]>4,"Raw blended normal was normalized");
 unsigned negative=0;const auto refuse=[&]{rejects([&]{validate_bnw_warrior_native_format4(v,1);});++negative;};
 v[8]=v[9]=v[10]=0;v[11]=255;refuse();v[11]=0;refuse();v[8]=255;v[12]=20;refuse();v[12]=0;v[6]=1;refuse();v[6]=0;v[22]=1;refuse();v[22]=0;v[0]=0;v[1]=0x7c;refuse();half(0,1);
 rejects([&]{validate_bnw_warrior_native_format4(std::span(v).first(27),1);});++negative;rejects([&]{validate_bnw_warrior_native_format4({},0);});++negative;
 rows[0]=std::numeric_limits<float>::quiet_NaN();rejects([&]{consume_bnw_warrior_native_vertex(v,rows);});++negative;
 check(bnw_warrior_expand_real16(0x8000)==0&&std::signbit(bnw_warrior_expand_real16(0x8000)),"Half signed zero lost");
 std::cout<<"PASS2488 source-packed vertices, source w0->xyz1/DP3,nested y0/zpositive,unclamped128/128/255,"<<negative<<" refusals; GPU/native animation not executed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
