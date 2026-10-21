#include "civ5/ui/measured_readonly_text.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::ui;
namespace {
void check(bool value,const char *message) {if(!value) throw std::runtime_error(message);}
void run() {
    int owner=0,foreign_owner=0;
    MeasuredReadonlyTextReport report{{&owner,7,41,1274,925},
        {{8,{402,844,70,23},"UnitPanel","UnitStatNameStrength","lower"},
         {15,{462,844,23,23},"UnitPanel","UnitStatStrength","upper"},
         {3,{460,844,25,23},"UnitPanel","other-lower","lower-last-in-container"}}};
    int checks=0;
    const auto current=[&] {++checks;return true;};
    const auto picked=pick_measured_readonly_text(report,report.frame,470,850,{},current);
    check(picked && picked->source_paint_index==15 && picked->tooltip=="upper" && checks==2,
        "Paint-order readonly picking or before/after current gate wrong");
    check(pick_measured_readonly_text(report,report.frame,462,844,{},current)->source_paint_index==15,
        "Actual left/top boundary excluded");
    check(!pick_measured_readonly_text(report,report.frame,485,850,{},current),"Actual half-open right boundary included");
    check(!pick_measured_readonly_text(report,report.frame,470,867,{},current),"Actual half-open bottom boundary included");
    check(!pick_measured_readonly_text(report,report.frame,470,850,std::string("GameMenu"),current),
        "Current modal/focused screen leaked WorldView hover");
    for(int which=0;which<5;++which) {
        auto stale=report.frame;
        if(which==0) stale.frame_owner=&foreign_owner;
        if(which==1) ++stale.frame_sequence;
        if(which==2) ++stale.snapshot_frame;
        if(which==3) --stale.framebuffer_width;
        if(which==4) --stale.framebuffer_height;
        check(!pick_measured_readonly_text(report,stale,470,850,{},current),"Wrong owner/frame/viewport report accepted");
    }
    checks=0;
    check(!pick_measured_readonly_text(report,report.frame,470,850,{},[&]{return ++checks==1;}) && checks==2,
        "After-pick owner invalidation leaked readonly tooltip");
    checks=0;
    check(!pick_measured_readonly_text(report,report.frame,470,850,{},[&]{++checks;return false;}) && checks==1,
        "Before-pick owner invalidation ignored");
    check(!pick_measured_readonly_text(report,report.frame,470,850,{},{}),"Absent current owner granted hover");
    check(!pick_measured_readonly_text(report,report.frame,470,850,{},[]{throw std::runtime_error("source fault");return true;}),
        "Throwing source current gate granted hover");
    report.regions={{20,{std::numeric_limits<std::int32_t>::max(),844,23,23},"UnitPanel","bad","bad"},
        {21,{-1,844,23,23},"UnitPanel","unclipped","bad"},
        {22,{462,844,23,23},"UnitPanel","empty",""}};
    check(!pick_measured_readonly_text(report,report.frame,470,850,{},current),
        "Overflow/unclipped/empty-tooltip report granted hover");
}
}
int main() {try {run();std::cout<<"Measured readonly text frame/viewport/owner/pick PASS; no_gameplay_or_native_hitbox_claim=1\n";return 0;}
catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}}