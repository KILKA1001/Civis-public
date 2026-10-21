#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include <iostream>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F>void refuses(F&& fn){bool refused=false;try{fn();}catch(const std::exception&){refused=true;}
    need(refused,"Native primary32 accepted replay/stale/reentered/lost source");}
struct Window {
    HWND h=CreateWindowExW(0,L"STATIC",L"Civis primary32 CPU gate",WS_OVERLAPPEDWINDOW,
        0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    ~Window(){if(h)DestroyWindow(h);}
};
void first(const std::shared_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner>& p){
    (void)p->prepare_native_dx9_first_archive_metadata();
    (void)p->admit_native_dx9_first_archive_entries();
    (void)p->publish_native_dx9_first_archive_cache();
}
}
int main(int argc,char** argv){try{
    need(argc==4,"Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS");
    Window window;need(window.h&&!IsWindowVisible(window.h),"Hidden owned HWND unavailable");
    std::uint64_t revision=1,callbacks=0,trigger=0;bool caught_reentry=false,release=false;
    std::shared_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner> owner;
    auto client=civ5::runtime::bnw_stock_lua_win32_client_extent(
        reinterpret_cast<std::uintptr_t>(window.h),"primary32-source",[&]{
            if(trigger&&++callbacks==trigger&&owner){
                if(release)owner.reset();
                else {try{(void)owner->current();}catch(const std::exception&){caught_reentry=true;}}
            }
            return revision;
        });
    auto lease=civ5::ui::BnwNativeEarlyInputLease::acquire(argv[1],argv[2],argv[3],
        reinterpret_cast<std::uintptr_t>(window.h),client);
    owner=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
    refuses([&]{(void)owner->complete_native_dx9_primary32_archives();});
    first(owner);
    bool foreign_refused=false;
    std::thread foreign([&]{try{(void)owner->complete_native_dx9_primary32_archives();}
        catch(const std::exception&){foreign_refused=true;}});
    foreign.join();need(foreign_refused,"Foreign thread consumed primary32 attempt");
    const auto* table=&owner->resources();
    const auto completed=owner->complete_native_dx9_primary32_archives();
    need(completed.positive_primary32_complete&&completed.completed_archives==32&&
         completed.executed_admitted_total==4882,"Executed stock32 receipts differ from expected source fixture");
    const auto retained=owner->inspect_native_dx9_primary32_archives();
    need(retained.completed_archives==completed.completed_archives&&
         retained.executed_admitted_total==completed.executed_admitted_total&&retained.positive_primary32_complete,
         "Earlier source receipts expired after later ordinal append");
    need(&owner->resources()==table&&owner->resources().keys().empty()&&owner->resources().mounts().empty()&&
         !owner->current().primary_dx9_registration_complete,"Source32 diagnostic fabricated enabled VFS/UI readiness");
    refuses([&]{(void)owner->complete_native_dx9_primary32_archives();});
    refuses([&]{owner->require_native_dx9_first_archive_entry_admission();});
    // Four revision calls belong to outer Package.validate. Sixth is inside
    // journal retained validation, with its actual ONE weak-lock Package pin.
    // Removing the last outside facade must refuse despite those TWO pins.
    const auto* raw=owner.get();callbacks=0;trigger=6;release=true;
    refuses([&]{(void)raw->inspect_native_dx9_primary32_archives();});
    trigger=0;release=false;
    need(!owner&&callbacks>=6,"Actual retained journal last-facade callback not exercised");

    owner=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);first(owner);
    callbacks=0;trigger=6;
    refuses([&]{(void)owner->complete_native_dx9_primary32_archives();});
    trigger=0;
    need(caught_reentry&&callbacks>=6,"Caught actual journal-adoption Package reentry not exercised");
    refuses([&]{(void)owner->current();});
    refuses([&]{(void)owner->complete_native_dx9_primary32_archives();});
    owner.reset();
    std::cout<<"PASS actual source32 cache miss/stat/header/prefix/metadata/entry/cache: "
        <<completed.completed_archives<<" archives, "<<completed.executed_admitted_total
        <<" admitted; retained append/replay/thread/reentry/last-parent refused; VFS closed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
