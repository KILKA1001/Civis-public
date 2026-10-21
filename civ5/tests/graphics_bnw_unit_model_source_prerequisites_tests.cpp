#include "../src/graphics/bnw_unit_model_source_prerequisites.hpp"
#include <array>
#include <fstream>
#include <iostream>
using namespace civ5::graphics::model_prerequisite_detail;
static void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static std::uint32_t word(std::ifstream&f){unsigned char b[4]{};f.read(reinterpret_cast<char*>(b),4);need(bool(f),"Truncated source prerequisite fixture");return b[0]|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);}
static std::string text(std::ifstream&f){auto count=word(f);need(count<4096,"Bounded source fixture string");std::string s(count,'\0');f.read(s.data(),count);need(bool(f),"Truncated source string");return s;}
int main(int argc,char**argv){try{
 need(argc==2,"Expected prerequisite source fixture");std::ifstream f(argv[1],std::ios::binary);char magic[8]{};f.read(magic,8);need(std::string(magic,8)=="BNWMPQ01","Prerequisite fixture schema");const auto cb=callback_labels();for(auto label:std::array{cb.log,cb.log_user,cb.reader,cb.allocate,cb.release,std::uint32_t(cb.initialized)})need(word(f)==label,"Original callback publication labels");
 need(word(f)==4,"Allocation case count");for(unsigned i=0;i<4;++i){auto align=word(f),bytes=word(f);const auto route=allocation_route(align,bytes);need(word(f)==route.civ_aligned&&word(f)==route.bytes&&word(f)==route.alignment,"Original allocation callback argument forwarding");need(word(f)==0x4000abc,"Original free callback pointer forwarding");}
 need(word(f)==3,"Log case count");for(unsigned i=0;i<3;++i){auto type=word(f),code=word(f),count=word(f);auto level=log_level(type,code);need(count==unsigned(bool(level)),"Original log routing count");if(level)need(word(f)==*level,"Original log routing level");}
 need(word(f)==12,"Matcher case count");for(unsigned i=0;i<12;++i){auto name=text(f);need(word(f)==gsd_match(name),"Original fixed ASCII GSD matcher");}
 const std::map<std::uint32_t,std::string> names={{1,"z.gsd"},{2,"beta.dds"},{3,"a.GSD"},{4,"archive.gsdx"},{5,"earlier.gsd"},{6,"disabled.GSD"},{7,"later.GsD"},{8,"file.gsd.tmp"},{9,"FIRST.GSD"}};
 const std::vector<NamespaceFixture> stack={{true,{{5,9}}},{false,{{6}}},{true,{{7,8}}}};
 need(word(f)==2,"Namespace source case count");for(unsigned i=0;i<2;++i){bool enabled=word(f);NamespaceFixture current{enabled,{{1,2,9},{3,4}}};const auto list=enumerate_source_names(current,stack,names);need(word(f)==list.size(),"Original namespace count");for(const auto& name:list)need(text(f)==name,"Original reverse stack / bucket-link / first-CRC name order");}
 const std::vector<DescriptorFixture> descriptors={{true,true,{{2507411529U,"Bone"},{0x55aa,"CollisionA"}}},{true,true,{{0x55aa,"CollisionB"},{1096652136U,"Mesh"}}},{false,true,{}},{true,false,{}},{false,true,{}},{false,true,{}}};
 need(word(f)==2,"Descriptor lifetime case count");for(unsigned i=0;i<2;++i){auto result=register_descriptor_names(i?std::vector<DescriptorFixture>{}:descriptors,0x12345678);need(word(f)==result.comparison_label&&word(f)==result.files_freed&&word(f)==result.files_without_descriptor,"Original descriptor callback/cleanup behavior");need(word(f)==result.owned_names.size(),"Original retained copied string count");for(const auto& [crc,name]:result.owned_names)need(word(f)==crc&&text(f)==name,"Original separate owned pool bytes after payload free / first CRC retention");}
 unsigned refused=0;try{(void)gsd_match(std::string(1,char(0x80)));}catch(const std::invalid_argument&){++refused;}try{(void)gsd_match(std::string("x\0.gsd",6));}catch(const std::invalid_argument&){++refused;}need(refused==2,"Unsupported matcher encoding refuses");need(f.peek()==std::ifstream::traits_type::eof(),"Source prerequisite fixture tail");std::cout<<"PASS callback labels, 4 allocator routes, 3 log routes, 12 ASCII matcher cases, native namespace order and descriptor copied lifetime; 2 encoding refusals; pure fixture only\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
