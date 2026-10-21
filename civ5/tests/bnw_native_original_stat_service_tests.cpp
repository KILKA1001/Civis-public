#include "bnw_native_original_stat_service.hpp"
#include <cstdlib>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <sys/stat.h>
#include <thread>
#include <type_traits>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
namespace {
void need(bool value,const char* message) { if(!value)throw std::runtime_error(message); }
template<class F>void refuses(F&&f){bool no=false;try{f();}catch(const std::exception&){no=true;}need(no,"Original stat accepted detached/unknown source");}
// Optional attribution of the real owners and individual fresh OS operations.
// This does not change their checks or replace the default regression below.
template<class F>void measure(const char* name,unsigned count,unsigned round,
    const std::uint64_t& callbacks,F&& work) {
    const auto before_callbacks=callbacks;
    const auto start=std::chrono::steady_clock::now();
    for(unsigned n=0;n<count;++n)work();
    const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now()-start).count();
    std::cout<<"STAT_COST operation="<<name<<" round="<<round<<" count="<<count
        <<" elapsed_us="<<elapsed<<" client_callbacks="<<callbacks-before_callbacks<<'\n'<<std::flush;
}
}
int main(int argc,char**argv){try{
    using ui::detail::BnwNativeOriginalStatRead;using ui::detail::BnwNativeOriginalStatService;
    static_assert(!std::is_default_constructible_v<BnwNativeOriginalStatRead> &&
                  !std::is_copy_constructible_v<BnwNativeOriginalStatRead> &&
                  !std::is_default_constructible_v<BnwNativeOriginalStatService>);
    need(argc==4||(argc==5&&std::string_view(argv[4])=="--benchmark"),
        "Actual source save/install/canonical roots required; optional --benchmark");
    struct Window {HWND h=nullptr;~Window(){if(h)DestroyWindow(h);}}window;
    window.h=CreateWindowExW(0,L"STATIC",L"",WS_OVERLAPPEDWINDOW,0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    need(window.h&&!IsWindowVisible(window.h),"Actual hidden owned HWND required");
    std::uint64_t revision=7,callbacks=0;
    const auto client=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window.h),"original-stat-before-package",[&]{++callbacks;return revision;});
    auto lease=ui::BnwNativeEarlyInputLease::acquire(argv[1],argv[2],argv[3],reinterpret_cast<std::uintptr_t>(window.h),client);
    refuses([&]{BnwNativeOriginalStatService::acquire(decltype(lease){});});
    auto service=BnwNativeOriginalStatService::acquire(lease);service->require_same_input(*lease);
    const auto& source=service->source();need(source.code_page==1251&&!source.loaded_crt_sha256.empty(),"Actual CRT/ACP proof absent");
    auto original=service->read_original_relative("Resource/DX9/AmericanWashingtonTextures.fpk");
    original->require_same_service(*service);
    const auto& fields=original->current();need(fields.return_value==0&&fields.size&&fields.mtime&&*fields.size>0,"Actual original archive stat failed");
    if(argc==5) {
        HMODULE crt=nullptr;
        need(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<const wchar_t*>(&_stat64i32),&crt),"Benchmark actual CRT module unavailable");
        const auto archive=(std::filesystem::path(argv[2])/"Resource/DX9/AmericanWashingtonTextures.fpk").string();
        TIME_ZONE_INFORMATION timezone{};const auto timezone_state=GetTimeZoneInformation(&timezone);
        need(timezone_state!=TIME_ZONE_ID_INVALID,"Benchmark timezone unavailable");
        for(unsigned round=0;round<3;++round) {
            const auto lease_cost=[&]{measure("early_lease",1024,round,callbacks,[&]{lease->require_current();});};
            const auto service_cost=[&]{measure("original_stat_service",512,round,callbacks,[&]{service->require_current();});};
            if(round%2){service_cost();lease_cost();}else{lease_cost();service_cost();}
            measure("original_stat_read",256,round,callbacks,[&]{need(original->current()==fields,"Benchmark original metadata changed");});
            measure("timezone",1024,round,callbacks,[&]{
                TIME_ZONE_INFORMATION now{};const auto state=GetTimeZoneInformation(&now);
                need(state==timezone_state&&std::memcmp(&now,&timezone,sizeof(now))==0,"Benchmark timezone changed");
            });
            measure("module_path_canonical",512,round,callbacks,[&]{
                std::wstring path(32768,L'\0');
                const auto length=GetModuleFileNameW(crt,path.data(),static_cast<DWORD>(path.size()));
                need(length&&length<path.size(),"Benchmark module path unavailable");
                path.resize(length);
                need(std::filesystem::canonical(path)==source.loaded_crt,"Benchmark module path changed");
            });
            measure("named_crt_open_info",512,round,callbacks,[&]{
                struct File {HANDLE h=INVALID_HANDLE_VALUE;~File(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}}file;
                file.h=CreateFileW(source.loaded_crt.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
                BY_HANDLE_FILE_INFORMATION info{};
                need(file.h!=INVALID_HANDLE_VALUE&&GetFileInformationByHandle(file.h,&info),"Benchmark named CRT unavailable");
            });
            measure("raw_stat64i32",1024,round,callbacks,[&]{
                struct _stat64i32 value{};errno=0;const auto rc=_stat64i32(archive.c_str(),&value);
                need(rc==fields.return_value&&errno==fields.error_number&&value.st_size==*fields.size&&value.st_mtime==*fields.mtime,
                    "Benchmark raw stat differs from actual retained source");
            });
            service->require_current();
        }
    }
    auto dotted=service->read_original_relative("Resource/./DX9/AmericanWashingtonTextures.fpk");
    need(dotted->current()==fields && dotted->sequence()>original->sequence(),"Source normalized relative read/sequence differs");
    auto missing=service->read_original_relative("Resource/DX9/__civis_stat_service_missing_source_20261002__.fpk");
    need(missing->current().return_value!=0 && missing->current().error_number!=0 &&
         !missing->current().size && !missing->current().mtime,"Failed original stat fabricated zero payload fields");
    for(const auto* path:{"Assets/../../outside","Assets\\..\\..\\outside","../outside","C:/outside","/outside","Assets/file:stream","Assets/folder. /file"})
        refuses([&]{service->read_original_relative(path);});
    auto same_inputs_different_service=BnwNativeOriginalStatService::acquire(lease);
    refuses([&]{original->require_same_service(*same_inputs_different_service);});
    auto other_lease=ui::BnwNativeEarlyInputLease::acquire(argv[1],argv[2],argv[3],reinterpret_cast<std::uintptr_t>(window.h),client);
    refuses([&]{service->require_same_input(*other_lease);});
    bool foreign_refused=false;std::thread foreign([&]{try{service->require_current();}catch(const std::exception&){foreign_refused=true;}});foreign.join();
    need(foreign_refused,"Original stat accepted foreign thread");
    need(!std::getenv("TZ"),"Custom TZ unsupported by finite proof");
    struct RestoreTZ{~RestoreTZ(){_putenv_s("TZ","");}}restore;
    need(_putenv_s("TZ","UTC0")==0,"Cannot mutate test process TZ");
    refuses([&]{original->current();});refuses([&]{BnwNativeOriginalStatService::acquire(lease);});
    need(_putenv_s("TZ","")==0,"Cannot restore test process TZ");service->require_current();
    std::cout<<"ORIGINAL STAT before-package size="<<*fields.size<<" mtime="<<*fields.mtime<<" missing errno="<<missing->current().error_number<<" CRT="<<source.loaded_crt_sha256<<'\n';
    service.reset();lease->require_current();refuses([&]{original->current();});
    ++revision;refuses([&]{same_inputs_different_service->require_current();});
    std::cout<<"PASS actual EARLY lease original stat; sealed read/failed unknown; same-valued owner, traversal, TZ, thread, expired service and stale client refused; no package/cache/ready authority\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
