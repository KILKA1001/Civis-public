#include "civ5/ui/bnw_native_package_archive_time_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include <iostream>
#include <cstdlib>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F> void refuses(F&& action){bool refused=false;try{action();}catch(const std::exception&){refused=true;}check(refused,"Native archive time accepted detached/currentness failure");}
}
int main(int argc,char** argv){try{
    check(argc==4,"Actual save/install/canonical roots required");
    struct Window{HWND h=nullptr;~Window(){if(h)DestroyWindow(h);}}window;
    window.h=CreateWindowExW(0,L"STATIC",L"",WS_OVERLAPPEDWINDOW,0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    check(window.h&&!IsWindowVisible(window.h),"Actual hidden owned HWND required");
    std::uint64_t revision=5;
    auto client=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window.h),"actual-category-archive-time",[&]{return revision;});
    auto lease=ui::BnwNativeEarlyInputLease::acquire(argv[1],argv[2],argv[3],reinterpret_cast<std::uintptr_t>(window.h),client);
    auto package=ui::BnwNativePackageDiscoveryOwner::create(lease);
    package->register_native_dx9_primary_prefix();
    auto boot=ui::BnwNativeResourceBootSettingsOwner::create(package);
    check(boot->current().effective_texture_quality && *boot->current().effective_texture_quality!=0,"Actual low Common source remains pending");
    package->register_native_dx9_texture_quality_branch(boot);package->register_native_common_high_quality_prefix();
    auto predecessor=ui::BnwNativePackagePreSkinOwner::create(package);
    refuses([&]{ui::BnwNativePackageArchiveTimeOwner::create(predecessor,{});});
    auto time=ui::BnwNativePackageArchiveTimeOwner::create(predecessor,package);
    time->require_same_resources(*package);const auto& frame=time->current();
    check(frame.archive_metadata_source_complete && frame.archives_loaded>20 && frame.eligible_categories>20 &&
          !frame.category_mount_stage_complete && !predecessor->current().category_mount_stage_complete,
          "Native archive times falsely completed category/table publication");
    // Real process environment mutation exercises the finite proof boundary;
    // restore absence even if a refusal assertion throws. No OS timezone changes.
    check(!std::getenv("TZ"),"Actual custom TZ not supported by finite differential");
    struct RestoreTZ{~RestoreTZ(){_putenv_s("TZ","");}} restore_timezone;
    check(_putenv_s("TZ","UTC0")==0,"Test custom TZ mutation failed");
    refuses([&]{(void)time->current();});
    refuses([&]{ui::BnwNativePackageArchiveTimeOwner::create(predecessor,package);});
    check(_putenv_s("TZ","")==0,"Test custom TZ restoration failed");
    time->require_same_resources(*package);
    std::size_t different=0;
    for(const auto& attempt:frame.attempts){
        if(!attempt.archive_loaded){check(!attempt.native_cached_archive_mtime,"Missing archive acquired cache time");continue;}
        check(attempt.native_cached_archive_mtime && *attempt.native_cached_archive_mtime==attempt.original.mtime.value_or(0),
              "Native cache uses canonical or FindA time");
        different+=attempt.original.mtime!=attempt.canonical_payload.mtime;
    }
    check(different>0,"Actual canonical copy-time difference not exercised");
    bool foreign_refused=false;std::thread foreign([&]{try{(void)time->current();}catch(const std::exception&){foreign_refused=true;}});foreign.join();check(foreign_refused,"Native archive time accepted foreign thread");
    std::cout<<"ARCHIVE TIME eligible="<<frame.eligible_categories<<" attempts="<<frame.attempts.size()<<" loaded="<<frame.archives_loaded<<" original-canonical-time-differences="<<different<<'\n';
    // The actual input lease stays alive while the source parent disappears.
    // Neither PreSkin nor metadata may retain Package through a future cycle.
    package.reset();lease->require_current();
    refuses([&]{(void)predecessor->current();});
    refuses([&]{(void)time->current();});
    refuses([&]{(void)boot->current();});
    ++revision;refuses([&]{lease->require_current();});
    std::cout<<"PASS: actual original stat64/cache times, distinct canonical metadata, SAME process/preSkin/package; expired parent refused with lease alive; category mounts/Skin remain pending\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}}
