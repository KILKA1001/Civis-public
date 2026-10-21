#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include "civ5/ui/bnw_native_pinned_installation_epoch.hpp"
#include <iostream>
#include <thread>
#include <type_traits>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
namespace {
template<class T>concept CallerInstallationEpochIssuer=requires(const T& owner){owner.capture_pinned_installation_epoch_after_parent_return();};
static_assert(!CallerInstallationEpochIssuer<ui::BnwNativeInstallationInputOwner>);
static_assert(!std::is_default_constructible_v<ui::BnwNativePinnedInstallationEpoch>);
static_assert(!std::is_copy_constructible_v<ui::BnwNativePinnedInstallationEpoch>);
static_assert(!std::is_move_constructible_v<ui::BnwNativePinnedInstallationEpoch>);
void need(bool v,const char* m){if(!v)throw std::runtime_error(m);}template<class F>void refused(F&& f){
 bool v=false;try{f();}catch(const std::exception&){v=true;}need(v,"Invalid installation/client source was accepted");}
struct Window{HWND h=CreateWindowExW(0,L"STATIC",L"Installation without save",WS_OVERLAPPEDWINDOW,0,0,841,621,
 nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(h)DestroyWindow(h);}};}
int main(int argc,char** argv){try{
 using Owner=ui::BnwNativeInstallationInputOwner;static_assert(!std::is_copy_constructible_v<Owner>);
 need(argc==3,"Expected ORIGINAL_INSTALL CANONICAL_ASSETS; no SAVE is accepted");Window w;need(w.h&&!IsWindowVisible(w.h),"Actual hidden HWND missing");
 const auto hwnd=reinterpret_cast<std::uintptr_t>(w.h);std::uint64_t revision=1;
 auto provider=runtime::bnw_stock_lua_win32_client_extent(hwnd,"actual-installation-no-save",[&]{return revision;});
 refused([&]{Owner::acquire(argv[1],argv[2],0,provider);});
 auto owned=Owner::acquire(argv[1],argv[2],hwnd,provider);const auto before=owned->observe();
 need(before.installation==std::filesystem::canonical(argv[1])&&before.canonical_assets==std::filesystem::canonical(argv[2])&&
  before.pinned_files.size()>=2&&before.client.revision==1,"Actual installation epoch was not captured");
 bool other=false;std::thread foreign([&]{try{owned->require_current();}catch(const std::exception&){other=true;}});foreign.join();
 need(other,"Foreign thread consumed genuine installation epoch");owned->require_current();
 const auto exe=before.installation/L"CivilizationV.exe";
 const auto write=CreateFileW(exe.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,
  OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(write!=INVALID_HANDLE_VALUE){CloseHandle(write);throw std::runtime_error("Original executable was not read-only locked");}
 need(GetLastError()==ERROR_SHARING_VIOLATION,"Write open failed for a reason other than held source lock");
 const auto after=owned->observe();need(after.pinned_files.size()==before.pinned_files.size(),"Unissued selection mutated installation pins");
 ++revision;refused([&]{owned->require_current();});revision=1;refused([&]{owned->require_current();});
 // A SECOND actual installation allocation only checks pre-handoff window
 // invalidation. It cannot issue/stand in for a post-return Parent epoch.
 auto window_owned=Owner::acquire(argv[1],argv[2],hwnd,provider);
 need(SetWindowPos(w.h,nullptr,0,0,942,722,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE)!=FALSE,"Actual test HWND resize failed");
 refused([&]{window_owned->require_current();});
 need(SetWindowPos(w.h,nullptr,0,0,841,621,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE)!=FALSE,"Actual test HWND restore failed");
 refused([&]{window_owned->require_current();});
 std::cout<<"Actual installation/client issuer without save: pinned="<<before.pinned_files.size()
  <<" selected_game=0 Boot_join=0 post_handoff_epoch=unissued runtime_epoch_validation=unavailable"
  <<" full_game=0 can_continue=0\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
