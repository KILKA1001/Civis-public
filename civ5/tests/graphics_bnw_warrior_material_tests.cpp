#include "../validation/bnw_warrior_material_sources.hpp"
#include "../validation/bnw_warrior_material_pose.hpp"
#include <iostream>
#include <limits>
using namespace civ5::graphics;
static void check(bool b,const char *m){if(!b)throw std::runtime_error(m);}
int main(int argc,char **argv){try{
 check(argc==5,"Usage: material_CPU ASSET_ROOT SDK_CACHE METADATA OUTPUT_DIR");std::filesystem::create_directories(argv[4]);
 const auto sources=load_bnw_warrior_material_sources(argv[1],argv[2],argv[3]);check(sources.resources.size()==9,"Actual material resources");
 for(unsigned member=0;member<3;member++)for(unsigned slot=0;slot<5;slot++)check(sources.sampler(member,slot).data.face_count()==(slot<2?1u:6u),"Source sampler faces");
 const std::array<std::size_t,3> vertices{829,863,796},triangles{654,674,638},fourth{48,0,8};unsigned raw_changes=0;
 for(unsigned member=0;member<3;member++){
  const auto &spec=bnw_gandhi_warrior_art[member];const auto mesh=load_bnw_warrior_source_mesh(argv[2],spec,vertices[member],triangles[member]);const auto skin=load_bnw_warrior_skin_diagnostic(argv[2],spec,vertices[member]);
  const auto pose=prepare_bnw_warrior_material_pose(mesh,skin,1);check(pose.nonzero_fourth_influences==fourth[member]&&pose.native_three_influence_compatible==(member==1),"Actual fourth influence frontier");
  const auto old=skin.pose(mesh,1);for(std::size_t v=0;v<vertices[member];v++){double len=0;for(unsigned r=0;r<3;r++)len+=double(pose.vertex_pnuv[v*8+3+r])*pose.vertex_pnuv[v*8+3+r];len=std::sqrt(len);raw_changes+=std::abs(len-1)>1e-6;for(unsigned r=0;r<3;r++)check(std::abs(pose.vertex_pnuv[v*8+3+r]/len-old[v*8+3+r])<1e-5,"Source raw-normal direction");}
 }
 check(raw_changes==1186,"Actual raw-normal fixture changed");
 BnwWarriorMaterialSamples s{};s.diffuse={1,1,1,0};s.irradiance={1,1,1,1};s.team_color={.25f,.5f,.75f,1};
 check(evaluate_bnw_warrior_material_samples(s)==s.team_color,"Native alpha0 team-color masking");s.diffuse[3]=1;check(evaluate_bnw_warrior_material_samples(s)==WarriorColor{1,1,1,1},"Native alpha1 white masking");
 s={};s.sref={1,1,1,.5f};s.sharp_environment={1,1,1,1};check(evaluate_bnw_warrior_material_samples(s)==WarriorColor{.0625f,.0625f,.0625f,1},"Native SREF eighth-power reflection");s.fog_factor=1;s.fog_color={.25f,.5f,.75f};check(evaluate_bnw_warrior_material_samples(s)==WarriorColor{.25f,.5f,.75f,1},"Native full fog color");
 unsigned negatives=0;const auto reject=[&](auto f){try{f();}catch(const std::exception&){++negatives;}};
 const auto dir=std::filesystem::path(argv[4]);std::ofstream(dir/"foreign.json")<<"{}";std::filesystem::create_directories(dir/"empty-cache");
 reject([&]{load_bnw_warrior_material_sources(argv[1],argv[2],dir/"missing.json");});reject([&]{load_bnw_warrior_material_sources(argv[1],argv[2],dir/"foreign.json");});reject([&]{load_bnw_warrior_material_sources(dir,argv[2],argv[3]);});reject([&]{load_bnw_warrior_material_sources(argv[1],dir/"empty-cache",argv[3]);});reject([&]{sources.sampler(3,0);});reject([&]{sources.sampler(0,5);});
 for(float bad:{-1.f,2.f,std::numeric_limits<float>::quiet_NaN()})reject([&]{auto invalid=s;invalid.diffuse[0]=bad;evaluate_bnw_warrior_material_samples(invalid);});check(negatives==9,"Material missing/foreign inputs accepted");
 std::cout<<"PASS actual3 material/pose CPU:9 DDS, raw normals1186, fourth48/0/8, source missing/foreign/sampler/sample negatives9\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
