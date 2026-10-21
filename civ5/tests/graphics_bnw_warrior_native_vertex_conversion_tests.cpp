#include "civ5/graphics/bnw_warrior_native_vertex_conversion.hpp"
#include "civ5/graphics/bnw_warrior_native_mesh_binding.hpp"
#include <fstream>
#include <iostream>
using namespace civ5::graphics;
static void check(bool b,const char*why){if(!b)throw std::runtime_error(why);}
static std::vector<std::uint8_t> read(const char*p){std::ifstream f(p,std::ios::binary);check(bool(f),"Native fixture absent");return {std::istreambuf_iterator<char>(f),{}};}
static std::uint32_t u32(std::span<const std::uint8_t>b,std::size_t at){check(at+4<=b.size(),"Fixture word bounds");std::uint32_t v;std::memcpy(&v,b.data()+at,4);return v;}
template<class F>static void rejects(F f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Unsupported SDK declaration accepted");}
int main(int argc,char**argv){try{
 check(argc==4,"Usage: native_vertex_conversion NATIVE_HALF_FIXTURE NATIVE_PACKED_FIXTURE NATIVE_BINDING_FIXTURE");
 const auto half=read(argv[1]),packed=read(argv[2]);
 check(half.size()>=12&&std::memcmp(half.data(),"BNWHALF1",8)==0,"Half schema");const auto cases=u32(half,8);check(cases==5120&&half.size()==12+cases*6,"Half extent");
 for(std::size_t i=0;i<cases;++i){std::uint16_t expected;std::memcpy(&expected,half.data()+16+i*6,2);check(bnw_warrior_native_real16_bits(u32(half,12+i*6))==expected,"Native half scalar mismatch");}
 check(bnw_warrior_native_real16_bits(0x3f801000)==0x3c01,"Native midpoint rounded to even instead of source up");
 check(bnw_warrior_native_real16_bits(0x80000000)==0x8000&&bnw_warrior_native_real16_bits(0x7f800001)==0x7c01,"Native signed zero/NaN payload changed");
 check(packed.size()>=12&&std::memcmp(packed.data(),"BNWPACK1",8)==0&&u32(packed,8)==3,"Packed schema");
 std::size_t at=12,total=0;std::vector<std::uint8_t> selected;
 for(unsigned model=0;model<3;++model){
  const auto count=u32(packed,at);at+=4;check(count==std::array<unsigned,3>{829,863,796}[model],"Actual source roster extent");check(at+std::size_t(count)*124<=packed.size(),"Packed stream extent");
  const std::span<const std::uint8_t> source(packed.data()+at,count*68);at+=count*68;
  const std::span<const std::uint8_t> converted(packed.data()+at,count*28);at+=count*28;
  const std::span<const std::uint8_t> native(packed.data()+at,count*28);at+=count*28;
  const auto result=convert_bnw_warrior_native_format4_vertices(source,count,68,bnw_warrior_sdk_vertex_declaration);
  check(result.bytes.size()==native.size()&&std::equal(result.bytes.begin(),result.bytes.end(),native.begin()),"Whole native packed stream mismatch");
  check(result.weights.changed==std::array<unsigned,3>{48,0,8}[model]&&result.weights.zero_divisors==0,"Native source weight changes mismatch");
  for(unsigned vertex=0;vertex<count;++vertex){
   const auto d=vertex*28;
   for(unsigned j=0;j<28;++j)if(j<8||j>=12)check(result.bytes[d+j]==converted[d+j],"Native weights changed nonweight channel");
   check(result.bytes[d+6]==0&&result.bytes[d+7]==0&&result.bytes[d+22]==0&&result.bytes[d+23]==0,"Source default missing array components not zero");
  }
  if(model==0)selected.assign(source.begin(),source.end());total+=count;
 }
 check(at==packed.size()&&total==2488,"Packed fixture trailing data");
 auto declaration=bnw_warrior_sdk_vertex_declaration;
 const auto convert=[&]{return convert_bnw_warrior_native_format4_vertices(selected,829,68,declaration);};
 declaration[0].name="";rejects(convert);declaration=bnw_warrior_sdk_vertex_declaration;
 declaration[0].array_count=4;rejects(convert);declaration=bnw_warrior_sdk_vertex_declaration;
 declaration[1].granny_type=12;rejects(convert);declaration=bnw_warrior_sdk_vertex_declaration;
 declaration[6].offset=60;rejects(convert);declaration=bnw_warrior_sdk_vertex_declaration;
 rejects([&]{convert_bnw_warrior_native_format4_vertices(selected,829,28,declaration);});
 rejects([&]{convert_bnw_warrior_native_format4_vertices(selected,828,68,declaration);});
 rejects([&]{convert_bnw_warrior_native_format4_vertices({},std::numeric_limits<std::size_t>::max(),68,declaration);});
 check(convert_bnw_warrior_native_format4_vertices({},0,68,declaration).bytes.empty(),"Empty native source rejected");
 const auto binding=read(argv[3]);check(binding.size()>=12&&std::memcmp(binding.data(),"BNWBIND1",8)==0&&u32(binding,8)==3,"Binding fixture schema");
 at=12;unsigned lookups=0;
 const auto string=[&](){const auto length=u32(binding,at);at+=4;check(length<128&&at+length<=binding.size(),"Named binding bounds");const std::string_view s(reinterpret_cast<const char*>(binding.data()+at),length);at+=length;return s;};
 for(unsigned model=0;model<3;++model){
  const auto bones=u32(binding,at);at+=4;check(bones==23,"Actual skeleton cardinality");std::array<std::string_view,23> names;for(auto&name:names)name=string();
  const auto count=u32(binding,at);at+=4;check(count==22,"Actual binding fixture cardinality");std::array<std::string_view,20> mesh;std::array<std::uint32_t,20> expected_palette;
  for(unsigned i=0;i<count;++i){const auto name=string();const bool found=u32(binding,at);const auto index=u32(binding,at+4);at+=8;const auto portable=lookup_bnw_warrior_native_bone(names,name);check(bool(portable)==found&&(!found||*portable==index),"Native mesh-binding lookup mismatch");if(i<20){mesh[i]=name;expected_palette[i]=index;}++lookups;}
  check(bind_bnw_warrior_native_mesh_palette(mesh,names)==expected_palette,"Actual native palette mismatch");
  auto bad=mesh;bad[0]="missing-native-bone";rejects([&]{bind_bnw_warrior_native_mesh_palette(bad,names);});
  rejects([&]{lookup_bnw_warrior_native_bone(names,"");});rejects([&]{bind_bnw_warrior_native_mesh_palette(std::span(mesh).first(19),names);});
 }
 check(at==binding.size()&&lookups==66,"Binding fixture extent");
 std::cout<<"PASS "<<cases<<" native half bit patterns,3 whole packed streams/"<<total<<" vertices,66 native bone lookups,16 rejected inputs; aggregate/override/finaldraw not accepted\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
