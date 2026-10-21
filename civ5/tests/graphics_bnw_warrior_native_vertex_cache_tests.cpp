#include "bnw_warrior_native_vertex_cache.hpp"
#include <iostream>
using namespace civ5::graphics;
static void check(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
template<class F>static void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Unsupported source cache accepted");}
int main(int argc,char**argv){try{
 check(argc==4,"Usage: native_vertex_cache ASSET_ROOT SDK_CACHE OWN_OUTPUT_DIR");check(std::filesystem::equivalent(argv[1],"C:/CrazyProject/civ5-assets"),"Canonical AssetRoot required");
 const std::filesystem::path cache=argv[2],out=argv[3];std::filesystem::create_directories(out);check(!std::filesystem::equivalent(cache,out),"Test output must differ from cache");
 unsigned vertices=0,negative=0;constexpr std::array<unsigned,3> counts{829,863,796},triangles{654,674,638};
 for(unsigned i=0;i<3;++i){const auto& spec=bnw_gandhi_warrior_art[i];const auto mesh=load_bnw_warrior_source_mesh(cache,spec,counts[i],triangles[i]);const auto skin=load_bnw_warrior_skin_diagnostic(cache,spec,counts[i]);
  const auto accepted=load_bnw_warrior_native_vertex_cache(cache,spec,mesh,skin);check(accepted.bytes.size()==counts[i]*28,"Actual native cache extent");vertices+=counts[i];
  const std::array<float,16> world{.14f,0,0,0,0,.14f,0,0,0,0,.14f,0,3,5,7,1};
  const auto rows=bnw_warrior_sdk_world_palette_rows(skin,-1,world);const auto matrices=skin.matrices(-1);
  // Direct independent point application fixes matrix orientation and ensures
  // the generated world scalar is applied once, not duplicated at GPU input.
  for(unsigned palette=0;palette<20;++palette)for(unsigned r=0;r<3;++r){double expected=world[12+r];for(unsigned c=0;c<3;++c)expected+=double(world[c*4+r])*(matrices[std::size_t(skin.palette[palette])][c*4]+2*matrices[std::size_t(skin.palette[palette])][c*4+1]+3*matrices[std::size_t(skin.palette[palette])][c*4+2]+matrices[std::size_t(skin.palette[palette])][c*4+3]);const auto at=palette*12+r*4;const float actual=rows[at]+2*rows[at+1]+3*rows[at+2]+rows[at+3];check(std::abs(actual-expected)<1e-5,"Palette/world composition or double scalar");}
  auto bad=skin;bad.palette[0]=0;rejects([&]{load_bnw_warrior_native_vertex_cache(cache,spec,mesh,bad);});++negative;
  bad=skin;bad.indices[0]=(bad.indices[0]+1)%20;rejects([&]{load_bnw_warrior_native_vertex_cache(cache,spec,mesh,bad);});++negative;
  bad=skin;bad.weights[0]^=1;rejects([&]{load_bnw_warrior_native_vertex_cache(cache,spec,mesh,bad);});++negative;
  auto foreign=mesh;foreign.gr2_sha256.assign(64,'0');rejects([&]{load_bnw_warrior_native_vertex_cache(cache,spec,foreign,skin);});++negative;
  const auto name=std::filesystem::path(spec.gr2_name).stem().string()+".mesh.json";auto text=warrior_read_bounded(cache/name,2'000'000);const auto at=text.find("\"rawVertexBytesBase64\": \"");check(at!=std::string::npos,"Actual payload key absent");text[at+25]=text[at+25]=='A'?'B':'A';
  {std::ofstream f(out/name,std::ios::binary);f<<text;check(bool(f),"Negative fixture write");}rejects([&]{load_bnw_warrior_native_vertex_cache(out,spec,mesh,skin);});++negative;
  auto nonfinite=world;nonfinite[0]=std::numeric_limits<float>::quiet_NaN();rejects([&]{bnw_warrior_sdk_world_palette_rows(skin,0,nonfinite);});++negative;
 }
 for(std::string_view bad:{"A","====","AAAA====","AB==","AAB=","AA?="}){rejects([&]{warrior_decode_source_base64(bad);});++negative;}
 std::cout<<"PASS actual3 pinned cache streams/"<<vertices<<" vertices,180 world/palette row comparisons,"<<negative<<" cache/base64/owner refusals; native animation remains diagnostic\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
