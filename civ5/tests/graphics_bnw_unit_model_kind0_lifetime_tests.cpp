#include "../src/graphics/bnw_unit_model_kind0_source_lifetime.hpp"
#include "../src/graphics/bnw_gsd_v7_descriptor.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/platform/windows_path.hpp"
#define NOMINMAX
#include <windows.h>
#include <array>
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace civ5::graphics::kind0_lifetime_detail;
static void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static std::uint32_t word(std::ifstream& f){std::array<unsigned char,4>b{};f.read(reinterpret_cast<char*>(b.data()),4);need(bool(f),"Truncated lifetime fixture");return std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);}
static std::uint64_t wide(std::ifstream&f){const auto low=word(f);return low|(std::uint64_t(word(f))<<32);}
static std::string text(std::ifstream&f){auto n=word(f);need(n<1024,"Fixture text budget");std::string s(n,'\0');f.read(s.data(),n);need(bool(f),"Truncated fixture text");return s;}
using Stat=std::array<std::uint32_t,8>;
static Stat identity(const std::filesystem::path&path){auto h=CreateFileW(civ5::platform::extended_windows_path(path).c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);need(h!=INVALID_HANDLE_VALUE,"Read-only diagnostic stat open");BY_HANDLE_FILE_INFORMATION i{};const auto ok=GetFileInformationByHandle(h,&i);CloseHandle(h);need(ok!=0,"Read-only diagnostic stat failed");return {i.dwFileAttributes,i.dwVolumeSerialNumber,i.nFileIndexHigh,i.nFileIndexLow,i.nFileSizeHigh,i.nFileSizeLow,i.ftLastWriteTime.dwHighDateTime,i.ftLastWriteTime.dwLowDateTime};}
static std::vector<std::uint8_t> payload(const std::filesystem::path&path,std::uint64_t offset,std::uint64_t length){std::ifstream f(civ5::platform::extended_windows_path(path),std::ios::binary);need(bool(f),"Bounded canonical/original archive absent");f.seekg(std::streamoff(offset));std::vector<std::uint8_t>b(length);f.read(reinterpret_cast<char*>(b.data()),std::streamsize(length));need(bool(f),"Bounded GSD payload failed");return b;}
int main(int argc,char**argv){try{
 need(argc==4,"Expected fixture, canonical assets, original install");std::ifstream f(argv[1],std::ios::binary);char magic[8]{};f.read(magic,8);need(std::string(magic,8)=="BNWK0L01","Lifetime fixture schema");
 const auto relative=text(f),sha=text(f);need(relative=="Resource/Common/UnitModels.fpk","Pinned archive path");need(sha=="f063955e71728fc76fdc097b9759e291f203c895d32f5f6eeaf5e4f78f73a959","Pinned actual GSD SHA");const auto ordinal=word(f);const auto offset=wide(f),length=wide(f);need(ordinal==4245&&offset==62570904&&length==160004,"Pinned FPK entry coordinates");
 Stat original_expected{},canonical_expected{};for(auto&v:original_expected)v=word(f);for(auto&v:canonical_expected)v=word(f);
 const auto original=std::filesystem::path(argv[3])/relative,canonical=std::filesystem::path(argv[2])/relative;
 need(identity(original)==original_expected&&identity(canonical)==canonical_expected,"Original stat and canonical payload identities changed");const auto a=payload(original,offset,length),b=payload(canonical,offset,length);need(a==b&&civ5::runtime::identify_bytes(b).sha256==sha,"Pinned original/canonical GSD bytes changed");
 const auto descriptor=civ5::graphics::gsd_detail::decode(b);need(descriptor.names.size()==3629,"Actual UnitModels descriptor count");const auto contains=[&](const char*name){return std::find(descriptor.names.begin(),descriptor.names.end(),name)!=descriptor.names.end();};need(contains("Models")&&contains("Skeletons")&&contains("ArtToolInfo")&&contains("FromArtToolName")&&!contains("UnitModel"),"GSD schema is not source kind0 registry registration");
 const auto cases=word(f);need(cases==25,"Bounded native lifetime case count");unsigned effects=0;
 for(unsigned i=0;i<cases;++i){std::array<std::uint32_t,8>in{},expected{};for(auto&v:in)v=word(f);for(auto&v:expected)v=word(f);auto n=word(f);need(n<=8,"Effect budget");std::vector<RequiredEffect>pending;for(unsigned j=0;j<n;++j)pending.push_back({word(f),word(f),word(f)});
  FixtureState s;s.objects.emplace(1,FixtureObject{in[1],false,0,0,0xb71548});s.objects.emplace(2,FixtureObject{in[2],false,0,0,0xb71548});s.objects.emplace(3,FixtureObject{in[3],true,in[4],0x44556677,0xb70f24});std::uint32_t result=0,out=0;
  switch(in[0]){
   case 0:result=diagnostic_load_result(s,3,0xa1,in[5]);break;
   case 1:{std::optional<Id>receiver;if(in[6]!=3)receiver=in[6];result=diagnostic_copy_out(s,3,receiver);out=receiver?*receiver:3;break;}
   case 2:result=diagnostic_unload(s,3);break;
   case 3:result=diagnostic_still_referenced(s,3);break;
   case 4:out=diagnostic_adapter_result(s,in[3]?3:0);break;
   case 5:{std::vector<Id>slots{3,0};diagnostic_remove_pending(s,slots,3);out=slots.front();break;}
   case 6:wrapper_destructor(s,3,in[7]);break;
   default:throw std::runtime_error("Unknown fixture operation");
  }
  const auto&w=object(s,3);const std::array<std::uint32_t,8>actual{result,out,object(s,1).refs,object(s,2).refs,w.refs,w.underlying,w.name,w.vtable};need(actual==expected,"Original kind0 lifetime field differential");need(s.pending==pending,"Original mandatory destructor/free ordering differs");effects+=n;
 }
 need(f.peek()==std::ifstream::traits_type::eof(),"Unexpected fixture tail");need(identity(original)==original_expected&&identity(canonical)==canonical_expected,"Pinned source input changed during differential");
 FixtureState invalid;invalid.objects.emplace(1,FixtureObject{1,false});invalid.objects.emplace(3,FixtureObject{1,true,1});unsigned type_refusals=0;
 const auto must_refuse=[&](auto operation){try{operation();}catch(const std::invalid_argument&){++type_refusals;return;}throw std::runtime_error("Unknown/wrong diagnostic reference accepted");};
 must_refuse([&]{(void)object(invalid,7);});must_refuse([&]{(void)diagnostic_unload(invalid,1);});must_refuse([&]{(void)diagnostic_load_result(invalid,3,0xa1,3);});need(type_refusals==3,"Strict diagnostic reference type guards");
 bool refused=false;try{require_native_load();}catch(const std::runtime_error&){refused=true;}need(refused,"Unowned native loader must refuse");
 std::cout<<"PASS "<<cases<<" original kind0 lifetime cases, "<<effects<<" pending mandatory effects, pinned original-stat/canonical GSD, native load refusal; namespace/manager/scale unavailable\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
