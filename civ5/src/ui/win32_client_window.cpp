#include "civ5/ui/win32_client_window.hpp"
#ifdef _WIN32
#include <exception>
#include <stdexcept>
#include <limits>
#include <optional>
#include <utility>

namespace civ5::ui {
namespace {
constexpr wchar_t binding_property[] = L"Civis.ProductionClientWindow";
std::uint32_t window_dpi(HWND window) {
    using Query = UINT (WINAPI *)(HWND);
    const auto query = reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"user32.dll"),
                                                             "GetDpiForWindow"));
    const auto dpi = query ? query(window) : 96u;
    return dpi ? dpi : 96u;
}
ClientPoint coordinates(LPARAM value) {
    return {static_cast<std::int16_t>(LOWORD(value)), static_cast<std::int16_t>(HIWORD(value))};
}
} // namespace
// Retained operation storage owns the callable and native continuation.
// Borrowed input is touched only while the faГ§ade remains attached/alive.
struct Win32ClientWindowBinding::Impl {
    const HWND window_;
    const DWORD thread_=GetCurrentThreadId();
    ClientWindowInput& input_;
    const Win32ClientWindowBinding* const facade_;
    Sink sink_;
    const ClientWindowEvent* active_sink_event_=nullptr;
    bool sink_event_claimed_=false;
    struct SinkEntry {
        Impl& owner;
        const ClientWindowEvent* previous;
        const bool claimed;
        SinkEntry(Impl& p,const ClientWindowEvent& event):owner(p),previous(p.active_sink_event_),claimed(p.sink_event_claimed_){
            p.require_active();
            p.active_sink_event_=&event;p.sink_event_claimed_=false;
        }
        ~SinkEntry(){owner.active_sink_event_=previous;owner.sink_event_claimed_=claimed;}
    };
    WNDPROC previous_=nullptr;
    std::atomic<bool> alive_{true},failed_{false};
    std::shared_ptr<BnwAudioWindowActivationLoan::State> activation_state_;
    std::shared_ptr<BnwTechAwardWindowPointerLoan::State> pointer_state_;
    std::shared_ptr<BnwWorldViewWindowInputLoan::State> worldview_input_state_;
    Impl(HWND window,ClientWindowInput& input,Sink sink,const Win32ClientWindowBinding* facade)
        :window_(window),input_(input),facade_(facade),sink_(std::move(sink)),
         activation_state_(std::make_shared<BnwAudioWindowActivationLoan::State>(window,input,*facade)),
         pointer_state_(std::make_shared<BnwTechAwardWindowPointerLoan::State>(window,input,*facade)),
         worldview_input_state_(std::make_shared<BnwWorldViewWindowInputLoan::State>(window,input,*facade)){}
    void require_active()const {
        if(GetCurrentThreadId()!=thread_)
            throw std::runtime_error("Production window operation foreign thread");
        if(!alive_.load()||failed_.load()||!IsWindow(window_)||
           GetWindowThreadProcessId(window_,nullptr)!=thread_||
           GetPropW(window_,binding_property)!=facade_||!alive_.load())
            throw std::runtime_error("Production window operation retired");
    }
    void attach() {
        if(GetCurrentThreadId()!=thread_||!IsWindow(window_)||!sink_||
           GetPropW(window_,binding_property)||GetWindowThreadProcessId(window_,nullptr)!=thread_)
            throw std::invalid_argument("Cannot attach production client window");
        RECT rectangle{};
        if(!GetClientRect(window_,&rectangle))throw std::runtime_error("Cannot query client size");
        input_.set_viewport({rectangle.right,rectangle.bottom,rectangle.right,rectangle.bottom,window_dpi(window_)});
        if(!SetPropW(window_,binding_property,const_cast<Win32ClientWindowBinding*>(facade_)))
            throw std::runtime_error("Cannot bind client property");
        SetLastError(0);
        previous_=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window_,GWLP_WNDPROC,
                         reinterpret_cast<LONG_PTR>(&Win32ClientWindowBinding::procedure)));
        if(!previous_) {
            RemovePropW(window_,binding_property);
            throw std::runtime_error("Cannot bind production client procedure");
        }
    }
    void detach()noexcept {
        if(GetCurrentThreadId()!=thread_)std::terminate();
        alive_.store(false);activation_state_->alive.store(false);pointer_state_->alive.store(false);worldview_input_state_->alive.store(false);
        // Restore/remove our address BEFORE synchronous capture notification.
        if(!IsWindow(window_))return;
        if(reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window_,GWLP_WNDPROC))==
                &Win32ClientWindowBinding::procedure)
            SetWindowLongPtrW(window_,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(previous_));
        if(GetPropW(window_,binding_property)==facade_)RemovePropW(window_,binding_property);
        if(GetCapture()==window_)ReleaseCapture();
    }
    void mark_failed()noexcept {
        failed_.store(true);activation_state_->failed.store(true);pointer_state_->failed.store(true);worldview_input_state_->failed.store(true);
        // A callback may have retired the faГ§ade and its borrowed router.
        if(GetCurrentThreadId()!=thread_||!alive_.load()||
           GetPropW(window_,binding_property)!=facade_)return;
        input_.cancel();
        if(alive_.load()&&GetCapture()==window_)ReleaseCapture();
    }
    void deliver(const ClientWindowEvent& event) {
        require_active();const auto callback=sink_;
        require_active(); // A callable target copy constructor may retire us.
        SinkEntry entered(*this,event);
        callback(event);
        require_active(); // No input/faГ§ade access after callback retirement.
    }
    void invalidate_pointer_epoch() {
        require_active();
        if(pointer_state_->epoch==std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error("TechAward native pointer epoch exhausted");
        ++pointer_state_->epoch;
    }
    void invalidate_worldview_epoch() {
        require_active();
        if(worldview_input_state_->epoch==std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error("WorldView native input epoch exhausted");
        ++worldview_input_state_->epoch;
    }
    void deliver_native_input(ClientWindowEvent& event,UINT message,WPARAM w,LPARAM l,ClientPoint point,
                              std::shared_ptr<const BnwWorldViewWindowInputLoan>* completion) {
        require_active();const auto input_state=worldview_input_state_;
        if(!input_state->alive.load()||input_state->failed.load()||input_state->active_sequence||
           input_state->sequence==std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error("WorldView native input retired/reentered");
        const auto sequence=++input_state->sequence;input_state->active_sequence=sequence;
        struct EndInput {std::shared_ptr<BnwWorldViewWindowInputLoan::State> state;
            ~EndInput(){state->active_sequence=0;}} end_input{input_state};
        auto mutable_input=std::shared_ptr<BnwWorldViewWindowInputLoan>(
            new BnwWorldViewWindowInputLoan(input_state,sequence,message,w,l,input_.viewport()));
        mutable_input->self_=mutable_input;
        std::shared_ptr<const BnwWorldViewWindowInputLoan> native=std::move(mutable_input);
        event.worldview_input=native;
        if(!completion||*completion)throw std::runtime_error("WorldView native procedure receipt already issued");
        *completion=native;
        const auto pointer_state=pointer_state_;
        std::shared_ptr<const BnwTechAwardWindowPointerLoan> pointer;
        struct EndPointer {std::shared_ptr<BnwTechAwardWindowPointerLoan::State> state;
            bool entered=false;~EndPointer(){if(entered)state->active_sequence=0;}} end_pointer{pointer_state};
        if(message==WM_MOUSEMOVE){
            if(!pointer_state->alive.load()||pointer_state->failed.load()||pointer_state->active_sequence||
               pointer_state->sequence==std::numeric_limits<std::uint64_t>::max())
                throw std::runtime_error("TechAward native pointer dispatch retired/reentered");
            const auto pointer_sequence=++pointer_state->sequence;
            pointer_state->active_sequence=pointer_sequence;end_pointer.entered=true;
            auto mutable_pointer=std::shared_ptr<BnwTechAwardWindowPointerLoan>(
                new BnwTechAwardWindowPointerLoan(pointer_state,pointer_sequence,point,input_.viewport()));
            mutable_pointer->self_=mutable_pointer;pointer=std::move(mutable_pointer);event.pointer_move=pointer;
        }
        const auto current=[&]{
            require_active();
            if(message!=WM_CLOSE||native->captured_)native->require_entered();
            else {
                // Never-bound diagnostic close legitimately retires viewport
                // inside its sink. It issues no captured Source authority;
                // keep the independent real binding/target lifetime fence.
                if(!input_state->alive.load()||input_state->failed.load()||
                   input_state->active_sequence!=sequence||input_state->sequence!=sequence||
                   native->dispatch_completed_||native->claimed_)
                    throw std::runtime_error("Uncaptured close actual native dispatch retired/reentered");
            }
            if(pointer)pointer->require_entered();require_active();
        };
        std::optional<Sink> target;
        try{
            current();target.emplace(sink_);current();
            SinkEntry entered(*this,event);
            (*target)(event);current();
            target.reset();current(); // real copied target destruction stays fenced
            // The real procedure issues input completion after message() and
            // any preserved original native continuation have both returned.
            if(pointer)pointer->dispatch_completed_=true;
        }catch(...){input_state->failed.store(true);pointer_state->failed.store(true);target.reset();throw;}
    }
    void record_mouse_cache_owned(UINT message,WPARAM w,LPARAM l) {
        require_active();
        if(message<WM_MOUSEMOVE||message>WM_MOUSEHWHEEL)return;
        const auto state=worldview_input_state_;
        if(!state->alive.load()||state->failed.load()||state->active_sequence||
           state->mouse_cache_sequence==std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error("Persistent native mouse cache retired/reentered/exhausted");
        // Exact original32 raw field stores before routing/callbacks. Values
        // persist through resize/cancellation as the actual cache does.
        state->mouse_w_650=static_cast<std::uint32_t>(w);
        if(message==WM_MOUSEMOVE)state->mouse_l_64c_64e=static_cast<std::uint32_t>(l);
        ++state->mouse_cache_sequence;require_active();
    }
    void update_viewport(std::int32_t width,std::int32_t height) {
        require_active();const auto dpi=window_dpi(window_);require_active();
        invalidate_pointer_epoch();invalidate_worldview_epoch();input_.set_viewport({width,height,width,height,dpi});
    }
    void set_framebuffer_size(std::int32_t width,std::int32_t height) {
        require_active();auto viewport=input_.viewport();
        viewport.framebuffer_width=width;viewport.framebuffer_height=height;
        invalidate_pointer_epoch();invalidate_worldview_epoch();input_.set_viewport(viewport);
    }
    void complete_worldview_input(const std::shared_ptr<const BnwWorldViewWindowInputLoan>& native) {
        // Only a genuine selected Source capture earns deferred completion.
        // Uncaptured diagnostic receipts cannot be used after their sink.
        if(!native||!native->captured_)return;
        require_active();
        native->require_source();
        if(native->dispatch_completed_||worldview_input_state_->active_sequence||
           !native->sequence_||native->sequence_>worldview_input_state_->sequence)
            throw std::runtime_error("WorldView native continuation reentered input delivery");
        native->dispatch_completed_=true;
    }
    bool message(UINT,WPARAM,LPARAM,std::shared_ptr<const BnwWorldViewWindowInputLoan>*);
};
Win32ClientWindowBinding::ExistingInputLoan::ExistingInputLoan(std::shared_ptr<Impl> p):creator_(GetCurrentThreadId()),operation_(std::move(p)){}
std::shared_ptr<Win32ClientWindowBinding::Impl> Win32ClientWindowBinding::ExistingInputLoan::retain_current()const{
    if(GetCurrentThreadId()!=creator_)throw std::runtime_error("Party binding loan foreign creator");
    const auto p=operation_.lock();if(!p)throw std::runtime_error("Party actual binding storage retired");p->require_active();return p;
}
void Win32ClientWindowBinding::ExistingInputLoan::require_current()const{const auto p=retain_current();p->require_active();}
void Win32ClientWindowBinding::ExistingInputLoan::require_idle()const{
    const auto p=retain_current();
    if(p->active_sink_event_||p->pointer_state_->active_sequence||p->worldview_input_state_->active_sequence||p->activation_state_->active_sequence)
        throw std::runtime_error("Party execution remains inside actual native input callback");p->require_active();
}
void Win32ClientWindowBinding::ExistingInputLoan::require_same_input(const ClientWindowInput& input)const{
    const auto p=retain_current();if(&p->input_!=&input)throw std::runtime_error("Party route has another actual input owner");p->require_active();
}
HWND Win32ClientWindowBinding::ExistingInputLoan::window()const{const auto p=retain_current();return p->window_;}
const Win32ClientWindowBinding* Win32ClientWindowBinding::ExistingInputLoan::binding()const{const auto p=retain_current();return p->facade_;}
void Win32ClientWindowBinding::ExistingInputLoan::claim_sink_event_owned(const ClientWindowEvent& event)const{
    const auto p=retain_current();
    if(p->active_sink_event_!=&event||p->sink_event_claimed_)throw std::runtime_error("Party route has no unique actual native sink event");
    p->sink_event_claimed_=true;p->require_active();
}
void Win32ClientWindowBinding::ExistingInputLoan::require_retirement_idle_owned(const ClientWindowInput& actual)const{
    if(GetCurrentThreadId()!=creator_)throw std::runtime_error("Party retirement foreign creator");
    const auto p=operation_.lock();
    // Quarantine cleanup alone may proceed after a failed dispatch. This is
    // not retain_current, source readiness, a normal borrow or a GL gate.
    if(!p||!p->alive_.load()||&p->input_!=&actual||
       GetPropW(p->window_,binding_property)!=p->facade_||GetWindowThreadProcessId(p->window_,nullptr)!=creator_||
       p->active_sink_event_||p->pointer_state_->active_sequence||p->worldview_input_state_->active_sequence||p->activation_state_->active_sequence)
        throw std::runtime_error("Party targets must retire while the actual binding is live and idle");
}
std::shared_ptr<const Win32ClientWindowBinding::ExistingInputLoan> Win32ClientWindowBinding::retain_existing_input_owned()const{
    const auto p=impl_;p->require_active();
    const auto result=std::shared_ptr<const ExistingInputLoan>(new ExistingInputLoan(p));
    result->require_idle();return result;
}
Win32ClientWindowBinding::Win32ClientWindowBinding(HWND window,ClientWindowInput& input,Sink sink)
    :impl_(std::make_shared<Impl>(window,input,std::move(sink),this)) {
    // Store the actual operation anchor before installing the native procedure.
    impl_->activation_state_->operation=impl_;impl_->pointer_state_->operation=impl_;impl_->worldview_input_state_->operation=impl_;
    impl_->attach();
}
Win32ClientWindowBinding::~Win32ClientWindowBinding(){impl_->detach();}
void Win32ClientWindowBinding::set_framebuffer_size(std::int32_t width,std::int32_t height) {
    const auto operation=impl_;operation->set_framebuffer_size(width,height);
}
std::shared_ptr<const BnwWorldViewWindowMouseCacheLoan>
Win32ClientWindowBinding::borrow_mouse_cache_owned(const graphics::BnwOwnedCameraRendererWindowLease::Loan& window){
    window.require_current();const auto native=reinterpret_cast<HWND>(window.window());
    if(GetCurrentThreadId()!=window.ui_thread())throw std::runtime_error("Mouse cache window foreign thread");
    const auto* facade=static_cast<const Win32ClientWindowBinding*>(GetPropW(native,binding_property));
    if(!facade)throw std::runtime_error("Mouse cache actual native window binding missing");
    // No callable crosses this lookup/copy. The actual property gives this
    // retained implementation; every later check uses its facade alive bit.
    const auto operation=facade->impl_;operation->require_active();
    if(operation->window_!=native||operation->thread_!=window.ui_thread())
        throw std::runtime_error("Mouse cache belongs to another actual renderer window");
    operation->require_active();const auto state=operation->worldview_input_state_;
    if(!state->alive.load()||state->failed.load()||state->active_sequence||
       !state->mouse_cache_sequence||!state->mouse_w_650||!state->mouse_l_64c_64e)
        throw std::runtime_error("Mouse cache needs actual completed native mouse payload observation");
    auto result=std::shared_ptr<const BnwWorldViewWindowMouseCacheLoan>(
        new BnwWorldViewWindowMouseCacheLoan(state,operation.get()));
    result->require_source_owned();window.require_current();operation->require_active();
    result->require_source_owned();return result;
}
bool Win32ClientWindowBinding::failed()const noexcept {
    const auto operation=impl_;
    if(GetCurrentThreadId()!=operation->thread_)return true;
    return !operation->alive_.load()||operation->failed_.load();
}
LRESULT CALLBACK Win32ClientWindowBinding::procedure(HWND window,UINT message,WPARAM w,LPARAM l) {
    const auto* binding=static_cast<const Win32ClientWindowBinding*>(GetPropW(window,binding_property));
    if(!binding)return DefWindowProcW(window,message,w,l);
    // UI-thread property lookup precedes any callback. Thereafter the faГ§ade
    // may be destroyed; never dereference it or its callable again.
    const auto operation=binding->impl_;const auto previous=operation->previous_;
    std::shared_ptr<const BnwWorldViewWindowInputLoan> native_input;
    try {
        if(message==WM_NCDESTROY){operation->message(message,w,l,&native_input);return CallWindowProcW(previous,window,message,w,l);}
        if(operation->message(message,w,l,&native_input)){operation->complete_worldview_input(native_input);return 0;}
        operation->require_active();
        const auto result=CallWindowProcW(previous,window,message,w,l);
        operation->require_active();operation->complete_worldview_input(native_input);return result;
    }catch(...) {
        operation->mark_failed();return 0;
    }
    return 0;
}
bool Win32ClientWindowBinding::Impl::message(UINT message, WPARAM w, LPARAM l, std::shared_ptr<const BnwWorldViewWindowInputLoan>* completion) {
    if(message==WM_NCDESTROY) {
        alive_.store(false);activation_state_->alive.store(false);pointer_state_->alive.store(false);worldview_input_state_->alive.store(false);
        pointer_state_->failed.store(true);worldview_input_state_->failed.store(true);return false;
    }
    if(pointer_state_->active_sequence||worldview_input_state_->active_sequence) {
        pointer_state_->failed.store(true);worldview_input_state_->failed.store(true);
        throw std::runtime_error("TechAward pointer native message reentry");
    }
    require_active();record_mouse_cache_owned(message,w,l);
    if (message == WM_ACTIVATE) {
        const auto state=activation_state_;
        if(!state||GetCurrentThreadId()!=state->thread)
            throw std::runtime_error("Audio activation dispatch foreign thread");
        if(!state->alive.load()||state->failed.load()||state->active_sequence||
           state->sequence==std::numeric_limits<std::uint64_t>::max()) {
            state->failed.store(true);
            throw std::runtime_error("Audio activation dispatch retired or reentered");
        }
        const auto sequence=++state->sequence;
        state->observed_minimized_word=HIWORD(w);state->active_sequence=sequence;
        struct Retire {
            std::shared_ptr<BnwAudioWindowActivationLoan::State> state;
            ~Retire(){state->active_sequence=0;}
        } retire{state};
        auto loan=std::shared_ptr<const BnwAudioWindowActivationLoan>(
            new BnwAudioWindowActivationLoan(state,sequence,w));
        loan->require_current();
        ClientWindowEvent event{ClientWindowEventKind::activation};event.activation=loan;
        deliver(event);
        loan->require_current();
        return false; // Preserve actual native activation processing.
    }
    if(message==WM_CLOSE){
        // Retire the MouseOver presentation epoch, while prior actual input
        // receipts keep their live window epoch until ordered shutdown drains.
        if(pointer_state_->epoch==std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error("TechAward close epoch exhausted");
        ++pointer_state_->epoch;
        ClientWindowEvent event{ClientWindowEventKind::close};
        deliver_native_input(event,message,w,l,{},completion);return true;
    }
    if (message == WM_SIZE || message == WM_DPICHANGED) {
        if (message == WM_DPICHANGED) {
            const auto *suggested = reinterpret_cast<const RECT *>(l);
            if (!suggested || !SetWindowPos(window_, nullptr, suggested->left, suggested->top,
                    suggested->right - suggested->left, suggested->bottom - suggested->top,
                    SWP_NOZORDER | SWP_NOACTIVATE))
                throw std::runtime_error("Cannot apply DPI window size");
            require_active();
        }
        RECT rectangle{};
        if (!GetClientRect(window_, &rectangle)) throw std::runtime_error("Cannot query resized client");
        update_viewport(rectangle.right, rectangle.bottom);
        if (GetCapture() == window_){ReleaseCapture();require_active();}
        deliver({message == WM_SIZE ? ClientWindowEventKind::resize : ClientWindowEventKind::dpi_changed});
        return false; // The renderer still owns allocation and GL viewport updates.
    }
    if (message == WM_KILLFOCUS || message == WM_CANCELMODE ||
        (message == WM_CAPTURECHANGED && reinterpret_cast<HWND>(l) != window_)) {
        invalidate_pointer_epoch();if(message!=WM_CAPTURECHANGED)invalidate_worldview_epoch();const bool cancelled_camera = input_.cancel();
        if (GetCapture() == window_){ReleaseCapture();require_active();}
        ClientWindowEvent event{ClientWindowEventKind::cancelled};
        event.native_flags = message;
        event.pointer.cancel_camera = cancelled_camera;
        deliver(event);
        return false;
    }
    if (message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN ||
        message == WM_LBUTTONUP || message == WM_MOUSEWHEEL) {
        ClientPoint point = coordinates(l);
        ClientWindowEvent event{ClientWindowEventKind::pointer};
        event.pointer_kind = message == WM_MOUSEMOVE ? ClientPointerKind::move :
            message == WM_LBUTTONDOWN ? ClientPointerKind::left_down :
            message == WM_LBUTTONUP ? ClientPointerKind::left_up : ClientPointerKind::wheel;
        if (message == WM_MOUSEWHEEL) {
            POINT native{point.x, point.y};
            if (!ScreenToClient(window_, &native)) throw std::runtime_error("Cannot map wheel coordinates");
            point = {native.x, native.y};
            event.wheel_delta = static_cast<std::int16_t>(HIWORD(w));
            event.native_flags = LOWORD(w);
        } else event.native_flags = static_cast<std::uint32_t>(w);
        event.pointer = input_.pointer(event.pointer_kind, point);
        event.owner = event.pointer.owner;
        if (event.pointer.capture) {
            SetCapture(window_);require_active();
            if (GetCapture() != window_) throw std::runtime_error("Cannot capture production pointer");
        }
        // Only genuine WM_MOUSEMOVE issues the closed animation pointer loan.
        if(message==WM_MOUSEMOVE){
            // Modal router cancellation may request release on a move. Complete
            // its native notification before minting the new pointer authority.
            if(event.pointer.release&&!event.pointer.capture&&GetCapture()==window_){ReleaseCapture();require_active();}
            deliver_native_input(event,message,w,l,point,completion);return true;
        }
        // Preserve the existing gesture notification before its synchronous
        // capture cancellation. Only the independent WorldView receipt is
        // issued after that cancellation; the ordinary event is not replayed.
        if(event.pointer.release&&!event.pointer.capture&&GetCapture()==window_){
            deliver(event);ReleaseCapture();require_active();
            ClientWindowEvent native_event{ClientWindowEventKind::native_input};
            deliver_native_input(native_event,message,w,l,point,completion);
        }else deliver_native_input(event,message,w,l,point,completion);
        return true;
    }
    if (message == WM_KEYDOWN || message == WM_KEYUP || message == WM_CHAR) {
        ClientWindowEvent event{message == WM_KEYDOWN ? ClientWindowEventKind::key_down :
            message == WM_KEYUP ? ClientWindowEventKind::key_up : ClientWindowEventKind::character};
        event.owner = input_.keyboard_owner();
        event.key = static_cast<std::uint32_t>(w);
        event.native_flags = static_cast<std::uint32_t>(l);
        deliver_native_input(event,message,w,l,{},completion);
        return true;
    }
    if(message==WM_RBUTTONDOWN||message==WM_RBUTTONUP||message==WM_MBUTTONDOWN||message==WM_MBUTTONUP||
       message==WM_LBUTTONDBLCLK||message==WM_RBUTTONDBLCLK||message==WM_MBUTTONDBLCLK||
       message==WM_SYSKEYDOWN||message==WM_SYSKEYUP){
        ClientWindowEvent event{ClientWindowEventKind::native_input};
        deliver_native_input(event,message,w,l,coordinates(l),completion);
        return false; // Existing native continuation still observes these messages.
    }
    return false;
}
} // namespace civ5::ui
#endif
