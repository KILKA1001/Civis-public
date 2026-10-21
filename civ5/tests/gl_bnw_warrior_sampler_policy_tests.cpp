#include "../validation/bnw_warrior_sampler_sources.hpp"
#include "civ5/graphics/gl_bnw_warrior_material_samplers.hpp"
#include <fstream>
#include <iostream>
using namespace civ5::graphics;
static void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
int main(int argc,char**argv){try{
 check(argc==6,"Usage ASSET_ROOT SDK_CACHE MATERIAL_METADATA SAMPLER_METADATA OUTPUT_DIR");
 std::filesystem::create_directories(argv[5]);const auto source=load_bnw_warrior_material_sources(argv[1],argv[2],argv[3]);
 unsigned negatives=0;auto reject=[&](auto f){try{f();}catch(const std::exception&){++negatives;return;}throw std::runtime_error("Invalid sampler source accepted");};
 const auto bad=std::filesystem::path(argv[5])/"foreign.json";std::ofstream(bad)<<"{}";
 reject([&]{load_bnw_warrior_sampler_policy(source,bad,0,16385,1);});
 reject([&]{load_bnw_warrior_sampler_policy(source,std::filesystem::path(argv[5])/"missing.json",0,16385,1);});
 auto foreign=source;foreign.metadata_sha256="foreign";reject([&]{load_bnw_warrior_sampler_policy(foreign,argv[4],0,16385,1);});
 reject([&]{load_bnw_warrior_sampler_policy(source,argv[4],3,16385,1);});
 reject([&]{load_bnw_warrior_sampler_policy(source,argv[4],0,-1,1);});
 reject([&]{load_bnw_warrior_sampler_policy(source,argv[4],0,16385,0);});
 auto context=std::make_shared<GlContext>(16,16,false);const auto policy=load_bnw_warrior_sampler_policy(source,argv[4],0,16385,1);
 reject([&]{GlBnwWarriorMaterialSamplers(nullptr,policy);});
 GlBnwWarriorMaterialSamplers owner(context,policy);
 const auto geti=reinterpret_cast<PFNGLGETSAMPLERPARAMETERIVPROC>(context->proc_address("glGetSamplerParameteriv"));
 const auto getf=reinterpret_cast<PFNGLGETSAMPLERPARAMETERFVPROC>(context->proc_address("glGetSamplerParameterfv"));
 for(unsigned slot=0;slot<6;++slot){const auto id=owner.sampler(*context,slot,0,16385,1);check(id!=0,"Missing owned sampler");
 for(auto [name,value]:{std::pair{GL_TEXTURE_WRAP_S,GL_REPEAT},std::pair{GL_TEXTURE_WRAP_T,GL_REPEAT},std::pair{GL_TEXTURE_WRAP_R,GL_REPEAT},std::pair{GL_TEXTURE_MAG_FILTER,GL_LINEAR},std::pair{GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR}}){GLint v=0;geti(id,name,&v);check(v==static_cast<GLint>(value),"Source sampler0 state drift");}
 GLfloat a=0;getf(id,0x84fe,&a);check(a==1,"Native maximum anisotropy1 drift");}
 reject([&]{owner.sampler(*context,6,0,16385,1);});reject([&]{owner.sampler(*context,0,1,16385,1);});reject([&]{owner.sampler(*context,0,0,1,1);});reject([&]{owner.sampler(*context,0,0,16385,2);});
 {GlContext other(16,16,false);reject([&]{owner.sampler(other,0,0,16385,1);});}
 unsigned mips=0;for(const auto&r:source.resources){GlTexture texture(*context,r.data,DdsSamplingColor::linear);for(unsigned f=0;f<r.data.face_count();++f)for(unsigned m=0;m<r.data.mip_count();++m){const auto b=texture.read_level_raw(f,m);const auto e=r.data.payload(f,m);check(b.size()==e.size()&&std::equal(b.begin(),b.end(),e.begin()),"Actual linear material texture drift");++mips;}}
 check(mips==156&&negatives==12,"Sampler verification cardinality");context->check("Source Warrior policy GL owner");
 std::ofstream(std::filesystem::path(argv[5])/"sampler-policy-gl-receipt.json")<<"{\"schema\":\"civis-warrior-native-sampler-policy-gl-v1\",\"sourceSamplerIndices\":[0,0,0,0,0,0],\"ownedSamplerObjects\":6,\"actualLinearDdsMipUploads\":156,\"negativeCases\":12,\"nativeSamplerPolicyProven\":true,\"nativeOracleExecuted\":false,\"waterHeightProducerProven\":false,\"modelDrawable\":false,\"fullGraphicsParity\":false}\n";
 std::cout<<"PASS source Warrior sampler0 six GL objects; actualDDS156; negatives12\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
