#include "civ5/graphics/bnw_warrior_native_resource_mesh.hpp"
#include <fstream>
#include <iostream>
#include <limits>
using namespace civ5::graphics;
struct Reader {std::vector<std::uint8_t> bytes;std::size_t at=0;std::uint32_t word(){auto v=warrior_curve_word(bytes,at);at+=4;return v;}std::vector<std::uint8_t> take(std::size_t n){warrior_curve_require(at<=bytes.size()&&n<=bytes.size()-at,"Mesh fixture truncated");std::vector<std::uint8_t> v(bytes.begin()+at,bytes.begin()+at+n);at+=n;return v;}};
template<class F>void refuses(F f,unsigned& n){bool failed=false;try{f();}catch(const std::invalid_argument&){failed=true;}warrior_curve_require(failed,"Mesh unsupported input accepted");++n;}
int main(int argc,char**argv){try {
 warrior_curve_require(argc==2,"Usage: resource_mesh ACTUAL_FIXTURE");std::ifstream in(argv[1],std::ios::binary);warrior_curve_require(bool(in),"Actual native mesh fixture absent");Reader r{{std::istreambuf_iterator<char>(in),{}}};auto magic=r.take(8);warrior_curve_require(std::string(magic.begin(),magic.end())=="BNWRMSH1"&&r.word()==3&&r.word()==3,"Mesh fixture schema");
 constexpr std::array<std::string_view,3> identities{"d78b64da4eaadc285f2c8f8f27fbdb6b6f01e790d0262ab2bfc7e5f6a714c21d","56104434706e2921b8fe142de65b07e2aad67aa64008b95be80d532892eac9c0","e03a7afa5f2ff04e2103381aa7cb4475c92dcb87629b659cb3069f3ba513688d"};
 const BnwWarriorGr2ArtToolMetadata meta{std::bit_cast<float>(0x421d7af6u),{0,0,0},{1,0,0},{0,0,1},{0,-1,0}};
 unsigned vertices=0,negative=0;std::vector<std::uint8_t> rawLast;BnwWarriorNativeResourceBasis basisLast{};
 for(unsigned model=0;model<3;++model) {auto id=r.take(32);constexpr char digits[]="0123456789abcdef";std::string hex;for(auto b:id){hex+=digits[b>>4];hex+=digits[b&15];}warrior_curve_require(hex==identities[model],"Mesh fixture identity");auto count=r.word();warrior_curve_require(count>0&&count<2000,"Mesh fixture count");auto raw=r.take(std::size_t(count)*68);
  for(unsigned c=0;c<3;++c) {auto scale=std::bit_cast<float>(r.word());auto expected=r.take(raw.size());auto basis=compile_bnw_warrior_native_resource_basis(meta,"78ec5c7873c3cf59ec1d140dfca5990c3e0011076da434aeedb4a6bea56f49ac",scale);auto actual=convert_bnw_warrior_native_resource_mesh(raw,identities[model],basis);warrior_curve_require(actual.basis_key==basis.key&&actual.raw_stream_identity==identities[model],"Mesh same conversion/stream identity");
   if(actual.vertices_stride68!=expected){for(std::size_t at=0;at<expected.size();++at)if(actual.vertices_stride68[at]!=expected[at]){std::cerr<<"model="<<model<<" case="<<c<<" vertex="<<at/68<<" byte="<<at%68<<" expected="<<unsigned(expected[at])<<" actual="<<unsigned(actual.vertices_stride68[at])<<'\n';break;}throw std::invalid_argument("Whole source9B1930 mesh differential mismatch");}vertices+=count;basisLast=basis;
  }rawLast=raw;
 }
 warrior_curve_require(r.at==r.bytes.size(),"Mesh fixture trailing bytes");refuses([&]{convert_bnw_warrior_native_resource_mesh(rawLast,"foreign",basisLast);},negative);refuses([&]{convert_bnw_warrior_native_resource_mesh({},identities[2],basisLast);},negative);refuses([&]{convert_bnw_warrior_native_resource_mesh(std::span(rawLast).first(rawLast.size()-1),identities[2],basisLast);},negative);refuses([&]{convert_bnw_warrior_native_resource_mesh(std::span(rawLast).first(68),identities[2],basisLast);},negative);auto invalid=basisLast;invalid.inverse[1]=0;refuses([&]{convert_bnw_warrior_native_resource_mesh(rawLast,identities[2],invalid);},negative);invalid=basisLast;invalid.key.compiled_scale_bits=0x3e0f5c29;refuses([&]{convert_bnw_warrior_native_resource_mesh(rawLast,identities[2],invalid);},negative);auto malformed=rawLast;auto nonfinite=std::bit_cast<std::array<std::uint8_t,4>>(std::numeric_limits<float>::infinity());std::copy(nonfinite.begin(),nonfinite.end(),malformed.begin()+20);refuses([&]{convert_bnw_warrior_native_resource_mesh(malformed,identities[2],basisLast);},negative);
 auto overflow=rawLast;const auto huge=std::bit_cast<std::array<std::uint8_t,4>>(std::numeric_limits<float>::max());std::copy(huge.begin(),huge.end(),overflow.begin()+20);const auto small=compile_bnw_warrior_native_resource_basis(meta,"78ec5c7873c3cf59ec1d140dfca5990c3e0011076da434aeedb4a6bea56f49ac",.14f);refuses([&]{convert_bnw_warrior_native_resource_mesh(overflow,identities[2],small);},negative);
 std::cout<<"PASS "<<vertices<<" actual raw68 mesh vertices against whole source9B1930 bitwise and "<<negative<<" refusals; curves/production scale/adoption separate\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}


