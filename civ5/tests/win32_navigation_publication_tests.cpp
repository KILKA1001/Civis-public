#include "civ5/ui/win32_viewport_transaction.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
using namespace civ5::ui;
static_assert(sizeof(void*)==8);
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
LRESULT CALLBACK base(HWND w,UINT m,WPARAM a,LPARAM b){return DefWindowProcW(w,m,a,b);}
struct HiddenWindow {
    HWND hwnd=nullptr;
    HiddenWindow(){
        WNDCLASSW c{};c.lpfnWndProc=base;c.hInstance=GetModuleHandleW(nullptr);c.lpszClassName=L"Civis.NavigationPublicationRegression";
        need(RegisterClassW(&c)!=0,"Register hidden regression class");
        hwnd=CreateWindowExW(0,c.lpszClassName,L"",WS_OVERLAPPEDWINDOW,0,0,640,480,nullptr,nullptr,c.hInstance,nullptr);
        need(hwnd!=nullptr,"Create hidden regression window");
    }
    ~HiddenWindow(){if(IsWindow(hwnd))DestroyWindow(hwnd);}
};
int main()try{
    HiddenWindow w;ClientWindowInput input;
    std::uint64_t sequence=10;std::string focus="";
    unsigned pointers=0,retirements=0,close_retirements=0,closes=0;
    bool source_metadata_live=true;
    std::optional<Win32ViewportTransaction::Ticket> last_ticket;
    Win32ViewportTransaction::SourceTargetsCurrent last_gate;
    Win32ViewportTransaction transaction(w.hwnd,input,[&](const auto &r){
        if(r.reason==Win32ViewportTransaction::RetirementReason::source_rejected)++retirements;
        if(r.reason==Win32ViewportTransaction::RetirementReason::close){++close_retirements;source_metadata_live=false;}
    },[&](const ClientWindowEvent &e){
        if(e.kind==ClientWindowEventKind::pointer)++pointers;
        if(e.kind==ClientWindowEventKind::close){
            need(close_retirements==1&&!source_metadata_live&&!input.viewport().drawable(),
                 "Native close sink ran before presented metadata retirement");
            ++closes;
        }
    });
    const auto paint=[&] {
        const auto ticket=transaction.request();need(bool(ticket),"Current hidden HWND ticket");
        const auto pinned_sequence=sequence;const auto pinned_focus=focus;
        // Explicit injected source/currentness identity; no actual GPU target,
        // Pedia callback, science popup admission or render receipt is claimed.
        const auto gate=[&,pinned_sequence,pinned_focus](const auto &) {
            return source_metadata_live && sequence==pinned_sequence && focus==pinned_focus;
        };
        const ClientRect hits[]{{10,10,30,30}};
        need(transaction.commit(*ticket,hits,false,false,gate),"Publish CPU source identity");
        need(transaction.current(*ticket,gate),"Fresh publication current");
        last_ticket=ticket;last_gate=gate;
    };
    const auto click=[&]{
        SendMessageW(w.hwnd,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(20,20));
        SendMessageW(w.hwnd,WM_LBUTTONUP,0,MAKELPARAM(20,20));
    };
    paint();
    // Same order as camera: previous paint/commit, then Help navigation.
    ++sequence;focus="CivilopediaScreen";
    click();
    need(pointers==0 && retirements==1 && !input.viewport().drawable() &&
         input.gesture_owner()==ClientInputOwner::none && GetCapture()!=w.hwnd,
         "Post-navigation stale science input escaped retained publication guard");
    paint();click();
    need(pointers==2,"Science pair was not forwarded after new parent publication");
    ++sequence;focus="TechTree";
    SendMessageW(w.hwnd,WM_MOUSEMOVE,0,MAKELPARAM(20,20));
    need(pointers==2&&retirements==2,"Post-science stale hover was forwarded");
    paint();click();need(pointers==4,"Close pair was not forwarded after TechTree publication");
    ++sequence;focus="CivilopediaScreen";
    SendMessageW(w.hwnd,WM_MOUSEMOVE,0,MAKELPARAM(20,20));
    need(pointers==4&&retirements==3,"Post-close stale hover was forwarded");
    paint();SendMessageW(w.hwnd,WM_MOUSEMOVE,0,MAKELPARAM(20,20));
    need(pointers==5&&!IsWindowVisible(w.hwnd),"Fresh parent hover failed or hidden window became visible");
    need(last_ticket&&last_gate&&transaction.current(*last_ticket,last_gate)&&source_metadata_live,
         "Final proof absent before native WM_CLOSE");
    need(retirements==3,"Navigation retirement count changed before native close");
    SendMessageW(w.hwnd,WM_CLOSE,0,0);
    need(closes==1&&close_retirements==1&&!source_metadata_live&&!input.viewport().drawable(),
         "Native WM_CLOSE failed to retire final frame before forwarding close");
    need(!transaction.current(*last_ticket,last_gate)&&retirements==4&&IsWindow(w.hwnd),
         "Historical pre-close ticket remained current after close metadata retirement");
    std::cout<<"PASS: stale navigation inputs refused;5 fresh pointer events;3 navigation retirements; final frame current before WM_CLOSE and retired before close sink; historical ticket refused; hidden HWND CPU only, GPU=false\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
