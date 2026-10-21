#include "civ5/ui/win32_viewport_transaction.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <thread>
#include <type_traits>

#ifdef _WIN32
namespace {
using namespace civ5::ui;
using Transaction = Win32ViewportTransaction;
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
LRESULT CALLBACK original(HWND window, UINT message, WPARAM w, LPARAM l) {
    return DefWindowProcW(window, message, w, l);
}
struct Window {
    HWND handle = nullptr;
    Window() {
        static const bool registered = [] {
            WNDCLASSW cls{};
            cls.lpfnWndProc = original;
            cls.hInstance = GetModuleHandleW(nullptr);
            cls.lpszClassName = L"Civis.HiddenViewportTransactionTest";
            return RegisterClassW(&cls) != 0;
        }();
        require(registered, "Cannot register hidden test window");
        handle = CreateWindowExW(0, L"Civis.HiddenViewportTransactionTest", L"",
            WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        require(handle != nullptr, "Cannot create hidden HWND");
        SetWindowLongPtrW(handle, GWLP_USERDATA, 0x1234);
    }
    ~Window() { if (IsWindow(handle)) DestroyWindow(handle); }
    void resize(int width, int height) {
        require(SetWindowPos(handle, nullptr, 0, 0, width, height,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE, "Native resize failed");
    }
};
// Explicitly a CPU lifecycle fault probe. It models current target/source
// validation to exercise transaction ordering, and attests no GPU/FOW pixels.
struct Probe {
    std::optional<ClientViewport> target;
    bool source_current = true, frame = false, hits = false, capture = false;
    bool throw_retire = false, throw_gate = false, throw_sink = false;
    int retirements = 0, cancellations = 0, actions = 0, gates = 0;
    Transaction::Retire retire() {
        return [this](const Transaction::Retirement &r) {
            ++retirements;
            cancellations += r.cancel_camera;
            frame = hits = capture = false;
            if (throw_retire) throw std::runtime_error("Retirement fault probe");
        };
    }
    Win32ClientWindowBinding::Sink sink() {
        return [this](const ClientWindowEvent &event) {
            if (throw_sink) throw std::runtime_error("Input fault probe");
            if (event.owner != ClientInputOwner::none) ++actions;
        };
    }
    Transaction::SourceTargetsCurrent gate() {
        return [this](const Transaction::Ticket &ticket) {
            ++gates;
            if (throw_gate) throw std::runtime_error("Source query fault probe");
            return source_current && target && *target == ticket.viewport();
        };
    }
    bool commit(Transaction &transaction, const Transaction::Ticket &ticket) {
        target = ticket.viewport();
        const ClientRect measured[]{{10, 10, 20, 20}};
        const bool accepted = transaction.commit(ticket, measured, false, false, gate());
        frame = hits = capture = accepted;
        return accepted;
    }
};
void resize_and_input() {
    Window window;
    ClientWindowInput input;
    Probe probe;
    {
        Transaction transaction(window.handle, input, probe.retire(), probe.sink());
        auto ticket = transaction.request();
        require(ticket.has_value(), "Initial real HWND has no ticket");
        require(!input.viewport().drawable(), "Uncommitted initial input published");
        require(probe.commit(transaction, *ticket), "Initial lifecycle commit failed");
        require(transaction.current(*ticket, probe.gate()), "Initial commit not current");
        SendMessageW(window.handle, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(100, 100));
        require(input.gesture_owner() == ClientInputOwner::camera && GetCapture() == window.handle,
                "Actual camera gesture/capture not established");
        window.resize(800, 600);
        require(!probe.frame && !probe.hits && !probe.capture && probe.cancellations > 0,
                "Resize did not retire old frame/hits/capture and camera gesture");
        require(!input.viewport().drawable() && input.keyboard_owner() == ClientInputOwner::none &&
                input.gesture_owner() == ClientInputOwner::none && GetCapture() != window.handle,
                "Resize left old input/capture enabled");
        require(!transaction.current(*ticket, probe.gate()), "Resized ticket remained current");
        auto resized = transaction.request();
        require(resized && resized->viewport() != ticket->viewport(), "Actual resized viewport absent");
        require(!transaction.commit(*resized, {}, false, false, probe.gate()),
                "Unrebuilt lifecycle targets accepted");
        resized = transaction.request();
        require(resized && probe.commit(transaction, *resized), "Fresh resized commit failed");
        probe.source_current = false;
        const auto previous_actions = probe.actions;
        SendMessageW(window.handle, WM_KEYDOWN, 'A', 0);
        require(probe.actions == previous_actions && !input.viewport().drawable() && !probe.frame,
                "Stale source forwarded an input action or retained old publication");
        require(GetWindowLongPtrW(window.handle, GWLP_USERDATA) == 0x1234,
                "Renderer HWND userdata changed");
    }
    require(reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window.handle, GWLP_WNDPROC)) == original,
            "Native procedure not restored after detach");
}
void minimize_and_dpi() {
    Window window;
    ClientWindowInput input;
    Probe probe;
    Transaction transaction(window.handle, input, probe.retire(), probe.sink());
    auto ticket = transaction.request();
    require(ticket && probe.commit(transaction, *ticket), "Pre-minimize commit failed");
    // WS_MINIMIZE changes the genuine IsIconic state without displaying the
    // hidden window. Polling must retire even if no WM_SIZE was delivered.
    SetWindowLongPtrW(window.handle, GWL_STYLE,
        GetWindowLongPtrW(window.handle, GWL_STYLE) | WS_MINIMIZE);
    require(IsIconic(window.handle) != FALSE, "Native iconic state not established");
    require(!transaction.request() && !transaction.current(*ticket, probe.gate()) &&
            !input.viewport().drawable() && !probe.frame, "Minimized window published");
    SetWindowLongPtrW(window.handle, GWL_STYLE,
        GetWindowLongPtrW(window.handle, GWL_STYLE) & ~LONG_PTR(WS_MINIMIZE));
    ticket = transaction.request();
    require(ticket && probe.commit(transaction, *ticket), "Restored window could not commit");
    RECT suggested{0, 0, 720, 540};
    // Synthetic event exercises native suggested-rect application, not an
    // assertion that this machine has a second DPI monitor.
    SendMessageW(window.handle, WM_DPICHANGED, MAKELONG(192, 192), reinterpret_cast<LPARAM>(&suggested));
    require(!transaction.current(*ticket, probe.gate()), "DPI event did not retire old ticket");
    auto changed = transaction.request();
    require(changed && !input.viewport().drawable(), "DPI resize enabled uncommitted input");
    using Query = UINT (WINAPI *)(HWND);
    const auto query = reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    const auto actual_dpi = query ? query(window.handle) : 96u;
    require(changed->viewport().dpi == actual_dpi, "Synthetic event DPI replaced actual HWND DPI");
    require(probe.commit(transaction, *changed), "DPI transaction commit failed");
    window.resize(0, 0);
    require(!transaction.request() && !input.viewport().drawable() && !probe.frame,
            "Zero native client extent published");
}
void missed_event_and_reentrancy() {
    Window window;
    ClientWindowInput input;
    Probe probe;
    Transaction transaction(window.handle, input, probe.retire(), probe.sink());
    auto ticket = transaction.request();
    require(ticket && probe.commit(transaction, *ticket), "Pre-missed-event commit failed");
    // Bypass only this hidden test HWND's subclass during the actual resize,
    // then restore the exact observed procedure. No wrapper event is delivered.
    const auto binding_proc = GetWindowLongPtrW(window.handle, GWLP_WNDPROC);
    SetWindowLongPtrW(window.handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original));
    window.resize(900, 700);
    SetWindowLongPtrW(window.handle, GWLP_WNDPROC, binding_proc);
    const auto old_version = ticket->version();
    require(!transaction.current(*ticket, probe.gate()) && !input.viewport().drawable() && !probe.frame,
                "Missed native resize accepted cached HWND dimensions");
    ticket = transaction.request();
    require(ticket && ticket->version() > old_version, "Missed resize did not advance lifecycle");
    probe.target = ticket->viewport();
    bool changed = false;
    auto reentrant = [&](const Transaction::Ticket &candidate) {
        if (!changed) { changed = true; window.resize(760, 570); }
        return probe.target == candidate.viewport();
    };
    require(!transaction.commit(*ticket, {}, false, false, reentrant) &&
            !input.viewport().drawable(), "Resize inside gate published stale target");
    ticket = transaction.request();
    require(ticket && probe.commit(transaction, *ticket), "Reentrant resize recovery failed");
    int calls = 0;
    auto second_gate_resize = [&](const Transaction::Ticket &) {
        if (++calls == 2) window.resize(780, 590);
        return true;
    };
    require(!transaction.commit(*ticket, {}, false, false, second_gate_resize) &&
            !input.viewport().drawable(), "Resize inside final gate escaped native recheck");
    ticket = transaction.request();
    require(ticket && probe.commit(transaction, *ticket), "Native input reentrancy fixture failed");
    const auto old_actions = probe.actions;
    auto native_press_inside_gate = [&](const Transaction::Ticket &) {
        SendMessageW(window.handle, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(100, 100));
        return true;
    };
    require(!transaction.current(*ticket, native_press_inside_gate) &&
            probe.actions == old_actions && !input.viewport().drawable() &&
            input.gesture_owner() == ClientInputOwner::none && GetCapture() != window.handle,
            "Source query retained a reentrant press with no delivered action");
}
void foreign_dead_and_failures() {
    Window first, second;
    ClientWindowInput first_input, second_input;
    Probe first_probe, second_probe;
    Transaction first_owner(first.handle, first_input, first_probe.retire(), first_probe.sink());
    Transaction second_owner(second.handle, second_input, second_probe.retire(), second_probe.sink());
    auto first_ticket = first_owner.request();
    auto second_ticket = second_owner.request();
    require(first_ticket && second_ticket, "Foreign test windows lack tickets");
    require(!second_owner.commit(*first_ticket, {}, false, false, [](const auto &) { return true; }) &&
            !second_input.viewport().drawable(), "Foreign HWND ticket accepted with true callback");
    require(first_probe.commit(first_owner, *first_ticket), "First HWND fixture commit failed");
    require(!first_owner.current(*first_ticket, {}), "Missing source/targets gate accepted");
    first_ticket = first_owner.request();
    require(first_ticket.has_value(), "Ticket after missing gate absent");
    first_probe.target = first_ticket->viewport();
    first_probe.throw_gate = true;
    require(!first_owner.commit(*first_ticket, {}, false, false, first_probe.gate()) &&
            !first_input.viewport().drawable(), "Throwing gate published");
    first_probe.throw_gate = false;
    first_ticket = first_owner.request();
    require(first_ticket && first_probe.commit(first_owner, *first_ticket), "Recovery after query exception failed");
    DestroyWindow(first.handle);
    require(!first_owner.current(*first_ticket, first_probe.gate()) && !first_owner.request() &&
            !first_input.viewport().drawable(), "Dead HWND retained publication");
    second_probe.throw_retire = true;
    second.resize(700, 510);
    require(second_owner.failed() && !second_owner.request() && !second_input.viewport().drawable(),
            "Retirement exception left a ready client");
}
void failed_binding_and_input_sink() {
    Window window;
    ClientWindowInput input;
    Probe probe;
    Transaction transaction(window.handle, input, probe.retire(), probe.sink());
    auto ticket = transaction.request();
    require(ticket && probe.commit(transaction, *ticket), "Fault fixture commit failed");
    // Existing native binding rejects a missing DPI suggested rectangle before
    // delivering the wrapper event. A later poll must retire external owners.
    SendMessageW(window.handle, WM_DPICHANGED, MAKELONG(96, 96), 0);
    require(transaction.failed() && !transaction.current(*ticket, probe.gate()) &&
            !probe.frame && !input.viewport().drawable(), "Failed binding retained a current frame");
    Window other;
    ClientWindowInput other_input;
    Probe other_probe;
    Transaction other_owner(other.handle, other_input, other_probe.retire(), other_probe.sink());
    auto other_ticket = other_owner.request();
    require(other_ticket && other_probe.commit(other_owner, *other_ticket), "Sink fault fixture commit failed");
    other_probe.throw_sink = true;
    SendMessageW(other.handle, WM_KEYDOWN, 'B', 0);
    require(other_owner.failed() && !other_owner.request() && !other_input.viewport().drawable(),
            "Throwing input sink left enabled input");
}
void reentrant_retirement() {
    Window window;
    ClientWindowInput input;
    Probe probe;
    Transaction *owner = nullptr;
    bool attempt_rebind = false, attempted = false;
    Transaction transaction(window.handle, input,
        [&](const Transaction::Retirement &r) {
            probe.retire()(r);
            if (attempt_rebind) {
                attempted = true;
                window.resize(780, 560); // Real nested WM_SIZE; no recursive retirement callback.
                require(!owner->request(), "Retirement callback requested publication before retiring finished");
            }
        }, probe.sink());
    owner = &transaction;
    auto ticket = transaction.request();
    require(ticket && probe.commit(transaction, *ticket), "Retirement reentrancy fixture failed");
    attempt_rebind = true;
    window.resize(760, 540);
    require(attempted && !transaction.failed() && !input.viewport().drawable(),
            "Reentrant retirement resurrected measurements or failed unexpectedly");
    ticket = transaction.request();
    require(ticket && probe.commit(transaction, *ticket), "Post-retirement native commit failed");
}
// Real hidden HWND/property/dispatch lifecycle fault probes. These do not
// attest Original activation history, an initial MASTER value, or GPU pixels.
using ActivationLoan = BnwAudioWindowActivationLoan;
using ActivationSource = ActivationLoan::SourceHandle;
static_assert(!std::is_default_constructible_v<ActivationSource>);
static_assert(!std::is_default_constructible_v<ActivationLoan>);
template<class F> void must_refuse(F&& f) {
    bool rejected=false;
    try { f(); } catch(const std::exception&) { rejected=true; }
    require(rejected,"Retired/foreign activation source accepted");
}
void activation_identity_and_lifetime() {
    Window window;
    ClientWindowInput input;
    std::optional<ActivationSource> source;
    std::shared_ptr<const ActivationLoan> retired_loan;
    Transaction* actual=nullptr;
    unsigned calls=0;
    auto owner=std::make_unique<Transaction>(window.handle,input,
        [](const auto&){},[&](const ClientWindowEvent& event) {
            if(event.kind!=ClientWindowEventKind::activation)return;
            require(event.activation!=nullptr,"Native activation omitted loan");
            actual->require_activation_window_owner(*event.activation);
            event.activation->require_current();
            ++calls;
            require(event.activation->sequence()==calls,"Actual activation sequence skipped");
            retired_loan=event.activation;
            if(!source)source.emplace(event.activation->source_handle());
            else source->require_same_source(event.activation->source_handle());
        });
    actual=owner.get();
    require(!source,"Window creation invented an observed activation source");
    // Controlled native dispatch inputs test HIWORD semantics, not the HWND's
    // physical iconic state or unobserved startup history.
    SendMessageW(window.handle,WM_ACTIVATE,MAKEWPARAM(WA_INACTIVE,1),0);
    require(calls==1&&source&&source->observed_sequence()==1&&
        source->native_minimized_word()==1,"Actual activation HIWORD/sequence not retained");
    must_refuse([&]{retired_loan->require_current();});
    SendMessageW(window.handle,WM_ACTIVATE,MAKEWPARAM(WA_INACTIVE,0),0);
    require(calls==2&&source->observed_sequence()==2&&source->native_minimized_word()==0,
        "Actual restore activation not observed by same source");
    bool foreign_rejected=false;
    std::thread foreign([&]{try{source->require_current();}catch(const std::exception&){foreign_rejected=true;}});
    foreign.join();require(foreign_rejected,"Foreign source thread accepted");
    Window other;
    ClientWindowInput other_input;
    std::optional<ActivationSource> other_source;
    auto other_owner=std::make_unique<Transaction>(other.handle,other_input,[](const auto&){},
        [&](const ClientWindowEvent& event){if(event.activation)other_source.emplace(event.activation->source_handle());});
    SendMessageW(other.handle,WM_ACTIVATE,MAKEWPARAM(WA_INACTIVE,0),0);
    require(other_source.has_value(),"Other actual window source absent");
    must_refuse([&]{source->require_same_source(*other_source);});
    owner.reset();must_refuse([&]{source->require_current();});
    require(GetPropW(window.handle,L"Civis.ProductionClientWindow")==nullptr&&
        GetPropW(window.handle,L"Civis.WindowResizeTransaction")==nullptr&&
        reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window.handle,GWLP_WNDPROC))==original,
        "Retired actual window still published binding/transaction/procedure");
}
struct TargetEffects {
    std::function<void()> copy, destroy, invoke;
    std::optional<ClientViewport> target;
    std::optional<ActivationSource> activation;
    unsigned actions=0,activations=0;
    bool destructor_threw=false;
};
struct GateTarget {
    std::shared_ptr<TargetEffects> state;
    explicit GateTarget(std::shared_ptr<TargetEffects> s):state(std::move(s)){}
    GateTarget(const GateTarget& other):state(other.state) {
        auto action=std::move(state->copy);if(action)action();
    }
    ~GateTarget() {
        try{auto action=std::move(state->destroy);if(action)action();}
        catch(...){state->destructor_threw=true;}
    }
    bool operator()(const Transaction::Ticket& ticket)const {
        return state->target&&*state->target==ticket.viewport();
    }
};
struct SinkTarget {
    std::shared_ptr<TargetEffects> state;
    explicit SinkTarget(std::shared_ptr<TargetEffects> s):state(std::move(s)){}
    SinkTarget(const SinkTarget& other):state(other.state) {
        auto action=std::move(state->copy);if(action)action();
    }
    void operator()(const ClientWindowEvent& event)const {
        if(event.kind==ClientWindowEventKind::pointer||event.kind==ClientWindowEventKind::key_down)
            ++state->actions;
        if(event.activation){++state->activations;state->activation.emplace(event.activation->source_handle());}
        auto action=std::move(state->invoke);if(action)action();
    }
};
void target_destruction_retires_input(bool replacing_gate) {
    Window window;
    auto input=std::make_unique<ClientWindowInput>();
    auto effects=std::make_shared<TargetEffects>();
    auto owner=std::make_unique<Transaction>(window.handle,*input,[](const auto&){},[](const auto&){});
    const auto ticket=owner->request();require(ticket.has_value(),"Destructor fixture ticket absent");
    effects->target=ticket->viewport();
    require(owner->commit(*ticket,{},false,false,GateTarget(effects)),"Destructor fixture commit failed");
    effects->destroy=[&]{owner.reset();input.reset();};
    if(replacing_gate) {
        const auto raw=owner.get();
        const bool accepted=raw->commit(*ticket,{},false,false,
            [v=ticket->viewport()](const auto& t){return t.viewport()==v;});
        require(!accepted,"Gate target destructor retired owner yet commit accepted");
    } else window.resize(780,540);
    require(!owner&&!input&&!effects->destructor_threw,"Gate destruction failed to retire borrowed input");
    require(GetPropW(window.handle,L"Civis.ProductionClientWindow")==nullptr,
        "Gate destructor left dangling native property");
}
void sink_target_copy_changes_geometry(bool pointer) {
    Window window;
    ClientWindowInput input;
    Probe probe;
    auto effects=std::make_shared<TargetEffects>();
    Transaction owner(window.handle,input,probe.retire(),SinkTarget(effects));
    const auto ticket=owner.request();require(ticket&&probe.commit(owner,*ticket),"Sink copy fixture failed");
    effects->copy=[&]{window.resize(810,610);};
    SendMessageW(window.handle,pointer?WM_LBUTTONDOWN:WM_KEYDOWN,
        pointer?MK_LBUTTON:'A',pointer?MAKELPARAM(100,100):0);
    require(effects->actions==0&&!input.viewport().drawable()&&
        input.gesture_owner()==ClientInputOwner::none&&GetCapture()!=window.handle,
        "Sink target copy resized yet forwarded stale drawable action");
    require(!owner.current(*ticket,probe.gate()),"Copy-retired publication remained current");
}
void activation_target_retirement(bool copying) {
    Window window;
    auto input=std::make_unique<ClientWindowInput>();
    auto effects=std::make_shared<TargetEffects>();
    auto owner=std::make_unique<Transaction>(window.handle,*input,[](const auto&){},SinkTarget(effects));
    auto drop=[&]{owner.reset();input.reset();};
    if(copying)effects->copy=drop;else effects->invoke=drop;
    SendMessageW(window.handle,WM_ACTIVATE,MAKEWPARAM(WA_INACTIVE,1),0);
    require(!owner&&!input,"Activation target did not retire facade/input");
    require(effects->activations==(copying?0u:1u),"Callback copied after close was invoked");
    if(effects->activation)must_refuse([&]{effects->activation->require_current();});
}
void activation_reentry_and_exception(bool throwing) {
    Window window;
    ClientWindowInput input;
    auto effects=std::make_shared<TargetEffects>();
    Transaction owner(window.handle,input,[](const auto&){},SinkTarget(effects));
    effects->invoke=[&]{
        if(throwing)throw std::runtime_error("Activation sink fault");
        SendMessageW(window.handle,WM_ACTIVATE,MAKEWPARAM(WA_INACTIVE,0),0);
    };
    SendMessageW(window.handle,WM_ACTIVATE,MAKEWPARAM(WA_INACTIVE,1),0);
    require(owner.failed()&&effects->activations==1&&effects->activation,
        "Nested/throwing actual activation was not poisoned");
    must_refuse([&]{effects->activation->require_current();});
}
void activation_and_callable_lifecycle() {
    activation_identity_and_lifetime();
    target_destruction_retires_input(false);
    target_destruction_retires_input(true);
    sink_target_copy_changes_geometry(false);
    sink_target_copy_changes_geometry(true);
    activation_target_retirement(false);
    activation_target_retirement(true);
    activation_reentry_and_exception(false);
    activation_reentry_and_exception(true);
}

} // namespace
int main() {
    try {
        resize_and_input();
        minimize_and_dpi();
        missed_event_and_reentrancy();
        foreign_dead_and_failures();
        failed_binding_and_input_sink();
        reentrant_retirement();
        activation_and_callable_lifecycle();
        std::cout << "PASS: real hidden HWND resize lifecycle; CPU target/source fault probes only, no GPU/FOW proof\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
#else
int main() { return 0; }
#endif
