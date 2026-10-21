#include "../src/graphics/bnw_gsd_v7_descriptor.hpp"
#include <fstream>
#include <iostream>
using namespace civ5::graphics::gsd_detail;
static std::uint32_t word(std::ifstream&f){std::uint8_t b[4]{};f.read(reinterpret_cast<char*>(b),4);need(bool(f),"Truncated GSD fixture");return u32(b,0);}
static std::uint64_t wide(std::ifstream&f){auto low=word(f);return low|(std::uint64_t(word(f))<<32);}
static std::string text(std::ifstream&f){auto n=word(f);need(n<4096,"GSD fixture string budget");std::string out(n,'\0');f.read(out.data(),n);need(bool(f),"Truncated GSD fixture text");return out;}
static void set(civ5::assets::Bytes& b,std::size_t p,std::uint32_t v){need(p+4<=b.size(),"Mutation range");for(unsigned i=0;i<4;++i)b[p+i]=std::uint8_t(v>>(8*i));}
int main(int argc,char**argv){try{
 need(argc==3,"Expected source GSD fixture and canonical asset root");std::ifstream f(argv[1],std::ios::binary);char magic[8]{};f.read(magic,8);need(std::string(magic,8)=="BNWGSD01","GSD fixture schema");auto files=word(f);need(files==24,"GSD source corpus count");std::uint64_t names_total=0;unsigned refusals=0;StringPool retained;std::map<std::uint32_t,std::string> expected_pool;
 for(unsigned i=0;i<files;++i){auto rel=text(f);need(rel.starts_with("Resource/Common/")&&rel.find("..") == std::string::npos,"Diagnostic archive fixture path");auto offset=wide(f),size=wide(f);need(size<=16*1024*1024,"Diagnostic payload budget");std::ifstream archive(std::filesystem::path(argv[2])/rel,std::ios::binary);need(bool(archive),"Canonical archive absent");archive.seekg(std::streamoff(offset));civ5::assets::Bytes bytes(size);archive.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(size));need(bool(archive),"Canonical bounded GSD read failed");
  auto d=decode(bytes);auto count=word(f);need(d.names.size()==count,"Original GSD descriptor count");for(const auto&name:d.names)need(name==text(f),"Original relocated GSD name bytes differ");names_total+=count;StringPool local;local.retain(d);auto pooled=word(f);need(local.names().size()==pooled,"Original GSD first CRC pool count");for(const auto&[crc,name]:local.names()){need(crc==word(f)&&name==text(f),"Original CRC/pool copied names differ");expected_pool.try_emplace(crc,name);}
  retained.retain(d);std::fill(bytes.begin(),bytes.end(),0xdd);need(local.names().size()==pooled&&retained.names()==expected_pool,"Owned GSD pool changed after input invalidation");
  if(i==0){archive.clear();archive.seekg(std::streamoff(offset));archive.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(size));const auto valid=bytes;
   const auto refuse=[&](auto mutate){auto bad=valid;mutate(bad);try{(void)decode(bad);}catch(const std::runtime_error&){++refusals;return;}throw std::runtime_error("Unsupported GSD mutation accepted");};
   const auto section=std::size_t(0x20)+u32(valid,0x2c);const auto rootsection=u32(valid,0x3c);const auto roots=section+rootsection*44;const auto relocs=u32(valid,roots+28);const auto firstfield=u32(valid,relocs);
   refuse([](auto&b){b[0]^=1;});refuse([](auto&b){set(b,0x20,6);});refuse([](auto&b){set(b,0x44,0x80000030);});refuse([](auto&b){b.pop_back();});refuse([](auto&b){set(b,0x30,0xffffffff);});refuse([&](auto&b){set(b,section,1);});refuse([&](auto&b){set(b,section+40,1);});refuse([&](auto&b){set(b,section+4,0xffffffff);});refuse([&](auto&b){set(b,relocs+4,0xffffffff);});refuse([&](auto&b){set(b,relocs,u32(valid,roots+12));});refuse([&](auto&b){set(b,relocs+8,0xffffffff);});refuse([&](auto&b){set(b,relocs+12,firstfield);});refuse([&](auto&b){set(b,u32(valid,roots+4)+u32(valid,0x40),0xffffffff);});
  }
 }
 need(refusals==13,"GSD refusal count");need(f.peek()==std::ifstream::traits_type::eof(),"GSD fixture tail");std::cout<<"PASS 24 real canonical GSD descriptors, "<<names_total<<" original names, CRC/pool lifetime, 13 malformed/unsupported refusals; native namespace/manager unavailable\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
