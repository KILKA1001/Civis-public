#include "../src/graphics/bnw_small_object_allocator_source.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/platform/windows_path.hpp"
#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <iostream>
using namespace civ5::graphics::small_allocator_detail;
static void need(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
static std::uint32_t word(std::ifstream&f){unsigned char b[4]{};f.read(reinterpret_cast<char*>(b),4);need(bool(f),"Truncated allocator fixture");return std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);}
static std::string text(std::ifstream&f){auto n=word(f);need(n<128,"Fixture string budget");std::string s(n,'\0');f.read(s.data(),n);need(bool(f),"Truncated fixture SHA");return s;}
using Stat=std::array<std::uint32_t,8>;
static Stat identity(const std::filesystem::path&path){auto h=CreateFileW(civ5::platform::extended_windows_path(path).c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);need(h!=INVALID_HANDLE_VALUE,"Diagnostic EXE stat open");BY_HANDLE_FILE_INFORMATION i{};const auto ok=GetFileInformationByHandle(h,&i);CloseHandle(h);need(ok!=0,"Diagnostic EXE stat failed");return {i.dwFileAttributes,i.dwVolumeSerialNumber,i.nFileIndexHigh,i.nFileIndexLow,i.nFileSizeHigh,i.nFileSizeLow,i.ftLastWriteTime.dwHighDateTime,i.ftLastWriteTime.dwLowDateTime};}
int main(int argc,char**argv){try{
 need(argc==4,"Expected allocator fixture, canonical native EXE, original installed EXE");std::ifstream f(argv[1],std::ios::binary);char magic[8]{};f.read(magic,8);need(std::string(magic,8)=="BNWSOA01","Allocator fixture schema");const auto sha=text(f);need(sha=="7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686","Original source EXE pin");Stat original{},canonical{};for(auto&v:original)v=word(f);for(auto&v:canonical)v=word(f);const auto source=std::filesystem::path(argv[3]),copy=std::filesystem::path(argv[2]);need(identity(source)==original&&identity(copy)==canonical,"Distinct source/canonical EXE identity changed");need(civ5::runtime::identify_file(source).sha256==sha&&civ5::runtime::identify_file(copy).sha256==sha,"Pinned original/canonical source payload mismatch");
 auto count=word(f);need(count==45,"Bounded allocator fixture count");unsigned effects=0;
 for(unsigned i=0;i<count;++i){Words operands{};for(auto&v:operands)v=word(f);std::array<std::uint32_t,7>fields{};Params params{};for(auto&v:fields)v=word(f);for(auto&v:params)v=word(f);const auto n=word(f);need(n<=4,"Mandatory endpoint budget");std::vector<Effect>pending;for(unsigned j=0;j<n;++j)pending.push_back({word(f),word(f),word(f)});const auto actual=observe(operands);need(actual.fields==fields&&actual.params==params,"Original allocator routing/params differential");need(actual.pending==pending,"Mandatory allocator endpoint order mismatch");effects+=n;}
 need(f.peek()==std::ifstream::traits_type::eof(),"Allocator fixture tail");need(identity(source)==original&&identity(copy)==canonical,"EXE input changed during comparison");
 unsigned refused=0;try{(void)class_index(1,0,16);}catch(const std::invalid_argument&){++refused;}try{(void)fixture_pool_tag(7);}catch(const std::invalid_argument&){++refused;}try{require_live_allocator();}catch(const std::runtime_error&){++refused;}need(refused==3,"Unowned live allocator/pool/quantum must refuse");
 std::cout<<"PASS "<<count<<" original allocator routing cases, "<<effects<<" mandatory modeled endpoints, distinct original/canonical source pins, 3 refusals; live allocator/manager unavailable\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
