#include "civ5/runtime/bnw_grand_selection.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/platform/windows_path.hpp"
#define NOMINMAX
#include <windows.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <type_traits>
using namespace civ5::runtime;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static std::uint32_t word(std::ifstream&f){unsigned char b[4]{};f.read(reinterpret_cast<char*>(b),4);need(bool(f),"Truncated original Grand fixture");return std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);}
using Stat=std::array<std::uint32_t,8>;
static Stat identity(const std::filesystem::path&p){const auto h=CreateFileW(civ5::platform::extended_windows_path(p).c_str(),FILE_READ_ATTRIBUTES,7,nullptr,OPEN_EXISTING,0,nullptr);need(h!=INVALID_HANDLE_VALUE,"Original/canonical stat open");BY_HANDLE_FILE_INFORMATION i{};const auto ok=GetFileInformationByHandle(h,&i);CloseHandle(h);need(ok!=0,"Source stat failed");return {i.dwFileAttributes,i.dwVolumeSerialNumber,i.nFileIndexHigh,i.nFileIndexLow,i.nFileSizeHigh,i.nFileSizeLow,i.ftLastWriteTime.dwHighDateTime,i.ftLastWriteTime.dwLowDateTime};}
template<class F>static void refuse(F&&f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}need(caught,"Expected source boundary refusal");}
int main(int argc,char**argv){try{
 static_assert(!std::is_default_constructible_v<BnwGrandSelectionPending>);
 static_assert(!std::is_default_constructible_v<BnwGrandMinimumAgeOwner>);
 static_assert(!std::is_copy_constructible_v<BnwGrandSelectionPending>);
 static_assert(!std::is_constructible_v<BnwGrandMinimumAgeOwner,std::int32_t>);
 need(argc==4,"Expected original fixture, original installed DLL and canonical DLL");std::ifstream f(argv[1],std::ios::binary);char magic[8]{};f.read(magic,8);need(std::string(magic,8)=="BNWGRS01","Original Grand fixture schema");Stat original{},canonical{};for(auto&v:original)v=word(f);for(auto&v:canonical)v=word(f);need(identity(argv[2])==original&&identity(argv[3])==canonical,"Distinct original-stat/canonical payload provenance");
 const std::string sha="c5483b5857a6fb7ef107af93c5d34282eb713472e2a84d0eb9d3fdc104e155a5";need(civ5::runtime::identify_file(argv[2]).sha256==sha&&civ5::runtime::identify_file(argv[3]).sha256==sha,"Pinned native source payload changed");
 const auto count=word(f);need(count==98,"Bounded original Grand case budget");unsigned city_logs=0,grand_logs=0,minus_one_noops=0,short_circuits=0;
 for(unsigned i=0;i<count;++i){std::array<std::int32_t,10>c{};std::array<std::uint32_t,13>native{};for(auto&v:c)v=std::bit_cast<std::int32_t>(word(f));for(auto&v:native)v=word(f);const std::array<std::int32_t,4>priorities{c[0],c[1],c[2],c[3]};need(c[5]>=0&&c[5]<INT32_MAX,"Source age increment budget");const auto q=evaluate_bnw_grand_selection(priorities,c[4],c[5]+1,c[6]);const bool dirty=q.specialization_called&&!c[7];const std::array<std::uint32_t,13>own{std::uint32_t(q.evaluated),std::bit_cast<std::uint32_t>(q.best_strategy),std::bit_cast<std::uint32_t>(q.best_priority),std::bit_cast<std::uint32_t>(q.active_after),std::bit_cast<std::uint32_t>(q.age_after),dirty?1U:std::uint32_t(c[8]),std::uint32_t(c[9]),dirty?0U:1U,std::uint32_t(q.minimum_age_read),std::uint32_t(q.set_active_called),std::uint32_t(q.active_written),std::uint32_t(q.specialization_called),q.slots_visited};need(own==native,"Original age/gate/strict best/SetActive/dirty/logger-stop differential");
 if(dirty)++city_logs;else ++grand_logs;if(q.set_active_called&&q.best_strategy==-1){++minus_one_noops;need(q.active_after==c[4]&&q.age_after==c[5]+1,"SetActive(-1) preserves original active and incremented age");}if(c[4]==-1){++short_circuits;const auto absent=evaluate_bnw_grand_selection(priorities,-1,c[5]+1);need(!absent.minimum_age_read&&absent.active_after==q.active_after,"NO strategy branch never requires cached MinAge owner");}
 }
 need(f.peek()==std::ifstream::traits_type::eof(),"Original fixture tail");need(city_logs>0&&grand_logs>0&&minus_one_noops>0&&short_circuits>0,"Required Grand branches covered");
 const std::array<std::int32_t,4>ties{7,7,7,7};need(evaluate_bnw_grand_selection(ties,-1,1).best_strategy==0,"Native strict tie keeps first slot");const auto held=evaluate_bnw_grand_selection({0,9,1,2},0,9,10);need(!held.evaluated&&held.active_after==0&&held.age_after==9,"Min-age must compare actual post-pressure age");
 refuse([&]{(void)evaluate_bnw_grand_selection(ties,0,1);});refuse([&]{(void)evaluate_bnw_grand_selection(ties,-2,1,10);});refuse([&]{(void)evaluate_bnw_grand_selection(ties,4,1,10);});refuse([&]{(void)evaluate_bnw_grand_selection(ties,-1,-1);});
 need(identity(argv[2])==original&&identity(argv[3])==canonical,"Native input changed during differential");
 std::cout<<"PASS "<<count<<" original Grand decision cases; "<<city_logs<<" mandatory city logs, "<<grand_logs<<" mandatory Grand logs, "<<minus_one_noops<<" SetActive(-1) no-ops, "<<short_circuits<<" MinAge short-circuits; diagnostic only, actual cached MinAge/logging/whole Grand unavailable\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
