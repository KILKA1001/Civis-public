#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include <iostream>
#include <chrono>
#include <string_view>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace {
void phase(const char* name){
    const auto now=std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    std::cerr<<"ACTUAL_PRIMARY32_SNAPSHOT phase="<<name<<" steady_ms="<<now<<'\n'<<std::flush;
}
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void refuses(F&& fn){bool refused=false;try{fn();}catch(const std::exception&){refused=true;}
    need(refused,"Actual primary32 record snapshot accepted missing/replay/lost source");}
struct Window {
    HWND hwnd=CreateWindowExW(0,L"STATIC",L"Civis primary32 record snapshot CPU gate",WS_OVERLAPPEDWINDOW,
        0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    ~Window(){if(hwnd)DestroyWindow(hwnd);}
};
}
int main(int argc,char** argv){try{
    const bool capture_cost_only=argc==5&&std::string_view(argv[4])=="--capture-cost-only";
    need(argc==4||capture_cost_only,"Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS [--capture-cost-only]");
    Window window;need(window.hwnd&&!IsWindowVisible(window.hwnd),"Actual hidden owned HWND unavailable");
    std::uint64_t revision=1,callbacks=0,trigger=0;
    bool destroy_input=false,drop_owner=false,input_destroyed=false,last_owner_dropped=false;
    std::u16string caller_buffer;
    std::shared_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner> owner;
    auto client=civ5::runtime::bnw_stock_lua_win32_client_extent(
        reinterpret_cast<std::uintptr_t>(window.hwnd),"primary32-record-source",[&]{
            if(trigger && ++callbacks==trigger){
                if(destroy_input){caller_buffer.assign(10000,u'X');caller_buffer.clear();
                    caller_buffer.shrink_to_fit();input_destroyed=true;}
                if(drop_owner){owner.reset();last_owner_dropped=true;}
            }
            return revision;
        });
    auto lease=civ5::ui::BnwNativeEarlyInputLease::acquire(argv[1],argv[2],argv[3],
        reinterpret_cast<std::uintptr_t>(window.hwnd),client);
    owner=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
    refuses([&]{(void)owner->capture_native_dx9_primary32_record_snapshot();});
    refuses([&]{(void)owner->query_native_dx9_primary32_record(u"Config.ini");});
    (void)owner->prepare_native_dx9_first_archive_metadata();
    (void)owner->admit_native_dx9_first_archive_entries();
    (void)owner->publish_native_dx9_first_archive_cache();
    const auto* table=&owner->resources();
    phase("complete-archives");
    const auto complete=owner->complete_native_dx9_primary32_archives();
    need(complete.positive_primary32_complete&&complete.executed_admitted_total==4882,
         "Actual stock primary32 source fixture incomplete");
    bool foreign_refused=false;
    std::thread foreign([&]{try{(void)owner->capture_native_dx9_primary32_record_snapshot();}
        catch(const std::exception&){foreign_refused=true;}});
    foreign.join();need(foreign_refused,"Foreign thread consumed snapshot attempt");
    phase("capture-records");
    need(owner->capture_native_dx9_primary32_record_snapshot()==4882,
         "Actual selected record capture differs from executed returns");
    refuses([&]{(void)owner->capture_native_dx9_primary32_record_snapshot();});
    // First genuine source directory record: captured exact immutable node,
    // selected offset/length and slot, using a different path/case spelling.
    if(capture_cost_only){
        phase("capture-cost-complete");
        std::cout<<"MEASURE genuine stock32 snapshot capture 4882; query/negative acceptance NOT RUN\n";
        return 0;
    }
    phase("query-present");
    const auto ordinary=owner->query_native_dx9_primary32_record(u"ART/WASHINGTON_ACCESSORIES_DIFF.DDS");
    need(ordinary.matched && ordinary.selected_crc==ordinary.requested_crc &&
         ordinary.payload_offset>0 && ordinary.payload_length>0 && ordinary.archive_ordinal==0 &&
         ordinary.source_slot==28672 && ordinary.first_raw_name=="washington_accessories_diff.dds",
         "Actual positive selected first-node snapshot differs from source fixture");
    // Pure CRC-formula collision of the actually PRESENT ordinal0 entry.
    // Unlike two absent Common names this detects an illicit filename-equality
    // guard: both different strings MUST select the same exact stock32 record.
    phase("query-crc-collision");
    const auto collision=owner->query_native_dx9_primary32_record(u"snapshot_'ux\"!(");
    need(collision.matched && collision.requested_crc==0x533ad1a9u &&
         collision.requested_crc==ordinary.requested_crc && collision.selected_crc==ordinary.selected_crc &&
         collision.payload_offset==ordinary.payload_offset && collision.payload_length==ordinary.payload_length &&
         collision.source_slot==ordinary.source_slot && collision.first_raw_name==ordinary.first_raw_name,
         "Actual positive CRC collision incorrectly used query filename equality");
    // Last slash wins before FIRST-colon processing. The retained basename
    // contains a colon: a wrapper reparsing only the prepared basename would
    // manufacture the positive Washington hit, changing native request identity.
    phase("query-colon");
    const auto colon=owner->query_native_dx9_primary32_record(u"path/:washington_accessories_diff.dds");
    need(colon.requested_crc!=ordinary.requested_crc&&!colon.matched,
         "Outer snapshot wrapper normalized a prepared basename twice");
    phase("query-absent");
    const auto config=owner->query_native_dx9_primary32_record(u"Config.ini");
    need(!config.matched,"Stock primary32 unexpectedly contains Config.ini CRC");
    phase("destroy-borrowed-input");
    caller_buffer=u"ART/WASHINGTON_ACCESSORIES_DIFF.DDS";callbacks=0;trigger=1;destroy_input=true;
    const auto prepared=owner->query_native_dx9_primary32_record(caller_buffer);
    trigger=0;destroy_input=false;
    need(input_destroyed&&caller_buffer.empty()&&prepared.requested_crc==ordinary.requested_crc&&
         prepared.matched&&prepared.first_raw_name==ordinary.first_raw_name,"Outer Root callback changed/dangled prepared caller query");
    phase("check-unpublished-state");
    need(&owner->resources()==table&&table->keys().empty()&&table->mounts().empty()&&
         !owner->current().primary_dx9_registration_complete&&
         !owner->current().native_skin_loading_complete,"Actual snapshot fabricated VFS/current UI activation");
    // Isolated final loss: no reentry/poison event hides the two-internal-pin
    // threshold. Four outer revision callbacks precede the leaf child reader;
    // the sixth occurs after SourceBindings issuance inside leaf validation.
    phase("drop-last-owner");
    const auto* raw=owner.get();callbacks=0;trigger=6;drop_owner=true;
    refuses([&]{(void)raw->query_native_dx9_primary32_record(u"Config.ini");});
    trigger=0;drop_owner=false;
    need(last_owner_dropped&&!owner&&callbacks>=6,"Actual leaf last-outside-Package loss not exercised");
    phase("complete");
    std::cout<<"PASS genuine stock32 snapshot 4882 actual record/handle/FPK nodes; CRC-only diagnostics, "
        "outer borrowed-input destruction, once/thread and isolated late last-Package refusal; VFS closed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
