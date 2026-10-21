#pragma once
#include "civ5/ui/win32_client_window.hpp"
#ifdef _WIN32
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
namespace civ5::validation {template<class LandRenderer>struct CameraPreviewLoop;}
#include <limits>
#include <memory>
#include <exception>
#include <stdexcept>
#include <utility>
#include <deque>

namespace civ5::graphics {class GlContext;class BnwOwnedCameraRendererWindowLease;}

namespace civ5::ui::detail {class CivisFrontendPartyRendererBackend;}

namespace civ5::ui {

// UI-thread lifecycle only. This does not attest GPU allocation, a rendered
// frame, or native fog: SourceTargetsCurrent must validate those actual owners.
// Present framebuffer dimensions are currently exactly the native client size.
// The borrowed router and HWND outlive this owner; detach before DestroyWindow,
// as required by Win32ClientWindowBinding. Callback retirement retains only
// private operation storage and refuses before touching retired borrowed input.
class Win32ViewportTransaction {
    friend class ::civ5::graphics::BnwOwnedCameraRendererWindowLease;
    template<class LandRenderer>friend struct ::civ5::validation::CameraPreviewLoop;
    struct Identity {};
    struct Impl;
  public:
    enum class RetirementReason { initial, resize, dpi_changed, native_change,
                                  source_rejected, close, failure };
    struct Retirement {
        RetirementReason reason;
        std::uint64_t version;
        bool cancel_camera;
    };
    class Ticket {
      public:
        const ClientViewport &viewport() const noexcept { return viewport_; }
        std::uint64_t version() const noexcept { return version_; }
        HWND window() const noexcept { return window_; }
      private:
        friend class Win32ViewportTransaction;
        friend struct Impl;
        friend class ::civ5::graphics::BnwOwnedCameraRendererWindowLease;
        Ticket(std::shared_ptr<const Identity> identity, HWND window,
               ClientViewport viewport, std::uint64_t version)
            : identity_(std::move(identity)), window_(window), viewport_(viewport),
              version_(version) {}
        std::shared_ptr<const Identity> identity_;
        HWND window_;
        ClientViewport viewport_;
        std::uint64_t version_;
    };
    using Retire = std::function<void(const Retirement &)>;
    using SourceTargetsCurrent = std::function<bool(const Ticket &)>;

  private:
    struct Impl final : std::enable_shared_from_this<Impl> {
        friend class ::civ5::graphics::BnwOwnedCameraRendererWindowLease;
        Impl(HWND window,ClientWindowInput& input,Retire retire,Win32ClientWindowBinding::Sink sink)
            :window_(window),thread_(GetCurrentThreadId()),input_(input),
             retire_(std::move(retire)),sink_(std::move(sink)){}
        void attach(const void* facade) {
            facade_=facade;
        if (!retire_ || !sink_ || !IsWindow(window_) ||
            GetWindowThreadProcessId(window_, nullptr) != thread_ ||
            GetPropW(window_, property_))
            throw std::invalid_argument("Cannot attach viewport transaction");
        if (!SetPropW(window_, property_, const_cast<void*>(facade_)))
            throw std::runtime_error("Cannot bind viewport transaction");
        try {
            binding_ = std::make_unique<Win32ClientWindowBinding>(window_, input_,
                [weak=weak_from_this()](const ClientWindowEvent &event) {
                    const auto operation=weak.lock();if(!operation)return;
                    if(GetCurrentThreadId()!=operation->thread_)
                        throw std::runtime_error("Viewport event foreign thread");
                    operation->on_event(event);
                });
            invalidate(RetirementReason::initial);
            native_ = read_native();
            if (!native_) failed_ = true;
        } catch (...) {
            binding_.reset();
            RemovePropW(window_, property_);
            throw;
        }
        }
        void close()noexcept {
            if(GetCurrentThreadId()!=thread_)std::terminate();
            if(closing_)return;
            closing_=true;
            disable_input();
            binding_.reset();
            if(GetPropW(window_,property_)==facade_)RemovePropW(window_,property_);
            published_.reset();published_gate_={};native_.reset();pending_close_.reset();pending_close_ticket_.reset();native_close_capture_.reset();
        }
    void require_activation_window_owner(const BnwAudioWindowActivationLoan& loan)const {
        loan.require_current(); //Immutable loan thread check before mutable Impl.
        if(closing_||failed_||!actual_binding_ptr()||loan.window()!=window_||loan.ui_thread()!=thread_||
           GetPropW(window_,property_)!=facade_)
            throw std::runtime_error("Actual audio viewport/window owner retired");
        loan.require_same_window_owner(*actual_binding_ptr(),input_);
    }
    bool failed() const noexcept {
        try {if(GetCurrentThreadId()!=thread_)return true;
            return closing_||failed_||(actual_binding_ptr()&&actual_binding_ptr()->failed());
        }catch(...){return true;}
    }

    // Poll even when no resize message was observed. A minimized or empty client
    // has no drawable ticket; old frames and measurements have already retired.
    std::optional<Ticket> request() noexcept {
        if(GetCurrentThreadId()!=thread_)return std::nullopt;
        if (closing_ || retiring_ || committing_ || pending_close_) return std::nullopt;
        if (!synchronize()) return std::nullopt;
        const auto &n = *native_;
        if (n.minimized || n.width <= 0 || n.height <= 0) return std::nullopt;
        return Ticket(identity_, window_, {n.width, n.height, n.width, n.height, n.dpi}, version_);
    }

    // The gate must query current source and actual rebuilt targets, not compare
    // only a cached requested size. Native HWND state is checked independently
    // before/after it; an arbitrary true callback cannot accept a stale ticket.
    bool commit(const Ticket &ticket, std::span<const ClientRect> measured_hits,
                bool focused_modal, bool text_focus,
                SourceTargetsCurrent source_and_targets_current) noexcept {
        if(GetCurrentThreadId()!=thread_)return false;
        if (committing_ || checking_ || retiring_ || closing_ || pending_close_) return false;
        if (!source_and_targets_current || !ticket_owned(ticket)) {
            invalidate(RetirementReason::source_rejected);
            return false;
        }
        committing_ = true;
        bool accepted = false;
        try {
            if (ticket_current(ticket) && source_and_targets_current(ticket) &&
                ticket_current(ticket) && source_and_targets_current(ticket) &&
                ticket_current(ticket)) {
                // Replacing a callable can run user code in its destructor.
                // Retire the old gate before borrowing input or publishing.
                SourceTargetsCurrent old_gate;
                old_gate.swap(published_gate_);
                published_.reset();
                old_gate = {};
                if (!ticket_current(ticket)) throw std::runtime_error("Viewport retired replacing gate");
                input_.set_viewport(ticket.viewport());
                input_.set_ui(measured_hits, focused_modal, text_focus);
                if (ticket_current(ticket) &&
                    input_.viewport() == ticket.viewport()) {
                    // The destination is empty: swap cannot destroy a prior
                    // callback-bearing target while accepting this ticket.
                    published_ = ticket;
                    published_gate_.swap(source_and_targets_current);
                    accepted = ticket_current(ticket) && published_ &&
                        published_->version() == ticket.version() && published_gate_ &&
                        input_.viewport() == ticket.viewport();
                }
            }
        } catch (...) { /* Callback/allocation errors never publish. */ }
        committing_ = false;
        if (!accepted) invalidate(RetirementReason::source_rejected);
        return accepted;
    }

    // Use immediately around swap/capture/actions, with their genuine target
    // and source check. Input forwarding also uses the retained commit gate.
    bool current(const Ticket &ticket, const SourceTargetsCurrent &gate) noexcept {
        if(GetCurrentThreadId()!=thread_)return false;
        if (checking_ || committing_ || retiring_ || closing_) return false;
        if (!gate || !ticket_owned(ticket)) {
            invalidate(RetirementReason::source_rejected);
            return false;
        }
        checking_ = true;
        bool accepted = false;
        try {
            accepted = published_ && published_->version() == ticket.version() &&
                ticket_current(ticket) && input_.viewport() == ticket.viewport() &&
                gate(ticket) && ticket_current(ticket) &&
                gate(ticket) && ticket_current(ticket) &&
                input_.viewport() == ticket.viewport();
        } catch (...) { /* Fail closed on a source query exception. */ }
        checking_ = false;
        if (!accepted) invalidate(RetirementReason::source_rejected);
        return accepted;
    }

  private:
    friend class Win32ViewportTransaction;
    struct Native {
        std::int32_t width, height;
        std::uint32_t dpi;
        bool minimized;
        bool operator==(const Native &) const = default;
    };
    inline static constexpr wchar_t property_[] = L"Civis.WindowResizeTransaction";
    std::optional<Native> read_native() const noexcept {
        try {
        if (GetCurrentThreadId() != thread_ || closing_ || !IsWindow(window_) ||
            GetPropW(window_, property_) != facade_ ||
            (actual_binding_ptr() && GetPropW(window_, L"Civis.ProductionClientWindow") != actual_binding_ptr()) ||
            GetWindowThreadProcessId(window_, nullptr) != thread_) return std::nullopt;
        RECT r{};
        if (!GetClientRect(window_, &r)) return std::nullopt;
        const auto width = std::int64_t(r.right) - r.left;
        const auto height = std::int64_t(r.bottom) - r.top;
        if (width < 0 || height < 0 || width > std::numeric_limits<std::int32_t>::max() ||
            height > std::numeric_limits<std::int32_t>::max()) return std::nullopt;
        using DpiQuery = UINT (WINAPI *)(HWND);
        const auto query = reinterpret_cast<DpiQuery>(GetProcAddress(
            GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
        const auto dpi = query ? query(window_) : 96u;
        if (!dpi) return std::nullopt;
        return Native{static_cast<std::int32_t>(width), static_cast<std::int32_t>(height),
                      dpi, IsIconic(window_) != FALSE};
        }catch(...){return std::nullopt;}
    }

    bool ticket_owned(const Ticket &ticket) const noexcept {
        return ticket.identity_ == identity_ && ticket.window_ == window_;
    }
    bool ticket_current(const Ticket &ticket) noexcept {
        if (!synchronize() || ticket.version() != version_) return false;
        const auto &n = *native_;
        return !n.minimized && n.width > 0 && n.height > 0 &&
            ticket.viewport() == ClientViewport{n.width, n.height, n.width, n.height, n.dpi};
    }
    bool synchronize() noexcept {
        try {
        if(GetCurrentThreadId()!=thread_)return false;
        if(closing_)return false; //Borrowed input may already be destroyed.
        if(failed_) { disable_input(); return false; }
        if (actual_binding_ptr() && actual_binding_ptr()->failed()) {
            invalidate(RetirementReason::failure);
            failed_ = true;
            return false;
        }
        const auto actual = read_native();
        if (!actual) {
            invalidate(RetirementReason::failure);
            failed_ = true;
            return false;
        }
        if (!native_ || *native_ != *actual) {
            invalidate(RetirementReason::native_change);
            native_ = read_native(); // Retirement may synchronously resize again.
        }
        return !closing_ && !failed() && native_.has_value();
        }catch(...){failed_=true;return false;}
    }

    bool disable_input() noexcept {
        if(borrowed_binding_){try{borrowed_binding_->require_same_input(input_);}catch(...){failed_=true;return false;}}
        bool cancelled = input_.cancel();
        auto viewport = input_.viewport();
        viewport.framebuffer_width = viewport.framebuffer_height = 0;
        try { input_.set_viewport(viewport); } catch (...) { failed_ = true; }
        // Binding's resize update may already have cancelled the gesture.
        // Consume the router's pending cancellation while framebuffer is zero.
        cancelled |= input_.pointer(ClientPointerKind::move, {-1, -1}).cancel_camera;
        if (GetCapture() == window_) ReleaseCapture();
        return cancelled;
    }
    void invalidate(RetirementReason reason) noexcept {
        if(GetCurrentThreadId()!=thread_||closing_)return;
        published_.reset();
        SourceTargetsCurrent old_gate;
        old_gate.swap(published_gate_);
        if (version_ == std::numeric_limits<std::uint64_t>::max()) failed_ = true;
        else ++version_;
        const bool already_retiring = retiring_;
        retiring_ = true; // Gate destruction and ReleaseCapture can reenter.
        old_gate = {}; // User target destruction may close and destroy input.
        if(closing_) { retiring_=already_retiring; return; }
        const bool cancelled = disable_input();
        // Reentrant resize still advances the version and disables input. The
        // callback is already retiring external owners, so do not recurse it.
        if (already_retiring || closing_) { retiring_ = already_retiring; return; }
        try {
            const auto callback=retire_; // Copying a target may close the owner.
            if(!closing_)callback({reason, version_, cancelled});
        }
        catch (...) { failed_ = true; if(!closing_)disable_input(); }
        retiring_ = false;
    }
    void on_event(const ClientWindowEvent &event,bool captured_deferred=false) {
        if(GetCurrentThreadId()!=thread_)throw std::runtime_error("Viewport event foreign thread");
        if (closing_) return;
        if(!captured_deferred&&native_close_capture_)
            throw std::runtime_error("Viewport input arrived after actual selected native close capture");
        if(event.worldview_input&&worldview_source_bound_&&!captured_deferred){
            event.worldview_input->require_entered();
            {
                // One pure capture borrower, dropped before the real event
                // target COPY/invoke/destruction. Capture itself issues the
                // selected Source receipt; subscription alone proves none.
                const auto actual=worldview_source_.lock();
                if(!actual)throw std::runtime_error("Viewport selected input Source retired");
                actual->capture_worldview_native_input_owned(event.worldview_input);
            }
            event.worldview_input->require_entered();
            if(worldview_source_.expired())throw std::runtime_error("Viewport selected input Source retired after capture");
        }
        if(!captured_deferred)retain_captured_native_close(event);
        std::optional<Ticket> dispatch_ticket;
        SourceTargetsCurrent dispatch_gate;
        if (event.kind == ClientWindowEventKind::activation) {
            // Actual activation must also reach the owner when minimized:
            // a drawable viewport ticket cannot authorize this message loan.
            if(!event.activation)throw std::runtime_error("Actual activation loan absent");
            event.activation->require_same_window_owner(*actual_binding_ptr(),input_);
        } else if (event.kind == ClientWindowEventKind::resize ||
            event.kind == ClientWindowEventKind::dpi_changed) {
            invalidate(event.kind == ClientWindowEventKind::resize ?
                RetirementReason::resize : RetirementReason::dpi_changed);
            native_ = read_native();
            if (!native_) failed_ = true;
        } else if (event.kind == ClientWindowEventKind::close) {
            if(!worldview_source_bound_){
                // Literal diagnostic/non-WorldView close path. An uncaptured
                // Binding receipt does not create a deferred Source channel.
                invalidate(RetirementReason::close);
            }else{
            if(!event.worldview_input||!event.worldview_input->captured_)
                throw std::runtime_error("Viewport selected close has no actual Source capture");
            if(captured_deferred)event.worldview_input->require_completed();else event.worldview_input->require_entered();
            if(event.worldview_input->message_!=WM_CLOSE||!actual_binding_ptr()||
               event.worldview_input->state_->binding!=actual_binding_ptr()||event.worldview_input->state_->input!=&input_||
               pending_close_||checking_||committing_||retiring_)
                throw std::runtime_error("Viewport close identity/reentry changed");
            // Preserve this exact publication through real source/GUI/window
            // shutdown. Close request itself is not successful teardown.
            dispatch_ticket=published_;dispatch_gate=published_gate_;
            if(!dispatch_ticket||!current(*dispatch_ticket,dispatch_gate))
                throw std::runtime_error("Viewport close lacks current canonical publication");
            if(captured_deferred)event.worldview_input->require_completed();else event.worldview_input->require_entered();
            pending_close_=event.worldview_input;pending_close_ticket_=*dispatch_ticket;
            }
        } else if (event.kind != ClientWindowEventKind::cancelled) {
            if (committing_ || checking_) {
                // Binding routed this native message before our sink. Retire
                // its gesture too; never retain a press whose action was skipped
                // because a source query pumped a reentrant native message.
                invalidate(RetirementReason::source_rejected);
                return;
            }
            // Copy before current(): invalidation clears the retained members.
            dispatch_ticket = published_;
            dispatch_gate = published_gate_;
            if (!dispatch_ticket || !current(*dispatch_ticket, dispatch_gate)) return;
        }
        if(closing_)return;
        try {
            const auto callback=sink_; // Target copy can synchronously retire.
            if(closing_)throw std::runtime_error("Viewport retired copying event callback");
            // A target COPY can resize or invalidate while keeping us alive.
            // Validate the original event publication after that user code.
            if(dispatch_ticket && !current(*dispatch_ticket,dispatch_gate))return;
            if(event.kind==ClientWindowEventKind::activation)
                event.activation->require_same_window_owner(*actual_binding_ptr(),input_);
            callback(event);
            if(closing_)throw std::runtime_error("Viewport retired during event callback");
            if(event.kind==ClientWindowEventKind::activation)
                event.activation->require_same_window_owner(*actual_binding_ptr(),input_);
        }
        catch (...) {
            invalidate(RetirementReason::failure);
            failed_ = true;
            throw; // Binding records failure without unwinding the Win32 proc.
        }
    }
    void require_staged_native_close(const std::shared_ptr<const BnwWorldViewWindowInputLoan>& supplied) {
        if(GetCurrentThreadId()!=thread_)throw std::runtime_error("Viewport close finalization foreign creator");
        if(!supplied||!pending_close_||closing_||failed_||retiring_||checking_||committing_||!actual_binding_ptr())
            throw std::runtime_error("Viewport close finalization has no actual pending native occurrence");
        supplied->require_shutdown_returned();
        if(supplied.get()!=pending_close_.get()||supplied.owner_before(pending_close_)||pending_close_.owner_before(supplied)||
           supplied.get()!=native_close_capture_.get()||supplied.owner_before(native_close_capture_)||native_close_capture_.owner_before(supplied)||
           supplied->state_->binding!=actual_binding_ptr()||supplied->state_->input!=&input_||
           !pending_close_ticket_||!published_||published_->version()!=pending_close_ticket_->version()||
           !ticket_owned(*pending_close_ticket_)||!ticket_current(*pending_close_ticket_))
            throw std::runtime_error("Viewport deferred close publication/identity changed");
        supplied->require_shutdown_returned();
    }
    void finalize_native_close(const std::shared_ptr<const BnwWorldViewWindowInputLoan>& supplied) {
        require_staged_native_close(supplied);
        // Only the owning Camera/backend invokes this private transition after
        // its real shutdown and, for Party, every outer launch/target POST.
        const auto previous_version=version_;
        pending_close_.reset();pending_close_ticket_.reset();native_close_capture_.reset();
        worldview_source_.reset();worldview_source_bound_=false;
        invalidate(RetirementReason::close);
        if(failed_||closing_||retiring_||version_==previous_version||published_||published_gate_||
           input_.viewport().framebuffer_width!=0||input_.viewport().framebuffer_height!=0||
           !actual_binding_ptr()||actual_binding_ptr()->failed())
            throw std::runtime_error("Viewport terminal retirement callbacks did not return normally");
    }
    void abort_native_close()noexcept {
        if(GetCurrentThreadId()!=thread_)std::terminate();
        if(closing_||!pending_close_)return;
        pending_close_.reset();pending_close_ticket_.reset();native_close_capture_.reset();
        invalidate(RetirementReason::failure);failed_=true;
    }
    const void* facade_=nullptr;
    HWND window_;
    DWORD thread_;
    ClientWindowInput &input_;
    Retire retire_;
    Win32ClientWindowBinding::Sink sink_;
    std::shared_ptr<const Identity> identity_ = std::make_shared<const Identity>();
    std::unique_ptr<Win32ClientWindowBinding> binding_;
    std::shared_ptr<const Win32ClientWindowBinding::ExistingInputLoan> borrowed_binding_;
    std::deque<ClientWindowEvent> existing_events_;
    Win32ClientWindowBinding* actual_binding_ptr()const {
        if(binding_)return binding_.get();
        if(!borrowed_binding_)return nullptr;
        borrowed_binding_->require_same_input(input_);
        return const_cast<Win32ClientWindowBinding*>(borrowed_binding_->binding());
    }
    void attach_existing(const void* facade,std::shared_ptr<const Win32ClientWindowBinding::ExistingInputLoan> source){
        source->require_idle();source->require_same_input(input_);
        if(source->window()!=window_)throw std::runtime_error("Viewport borrowed another actual window");
        facade_=facade;borrowed_binding_=std::move(source);
        if(!retire_||!sink_||GetPropW(window_,property_))throw std::runtime_error("Existing viewport owner already installed");
        if(!SetPropW(window_,property_,const_cast<void*>(facade_)))throw std::runtime_error("Existing viewport property failed");
        try{
            // This is a fresh private transaction around the existing menu
            // binding. No old publication exists to cancel: disabling the
            // shared router here would make the menu's load button unusable.
            version_=1;native_=read_native();
            if(!native_)throw std::runtime_error("Existing viewport native measurement refused");
            {const auto callback=retire_;borrowed_binding_->require_idle();
                callback({RetirementReason::initial,version_,false});borrowed_binding_->require_idle();}
            borrowed_binding_->require_idle();
        }catch(...){RemovePropW(window_,property_);borrowed_binding_.reset();throw;}
    }
    void retain_captured_native_close(const ClientWindowEvent& event){
        if(!event.worldview_input||!worldview_source_bound_||event.kind!=ClientWindowEventKind::close)return;
        const auto& native=event.worldview_input;native->require_entered();
        if(native_close_capture_||native->message_!=WM_CLOSE||!native->captured_||
           native->state_->window!=window_||native->state_->thread!=thread_||
           native->state_->input!=&input_||native->state_->binding!=actual_binding_ptr())
            throw std::runtime_error("Viewport native close capture repeated or substituted");
        native_close_capture_=native;native->require_entered();
    }
    void capture_existing(const ClientWindowEvent& event){
        if(!borrowed_binding_||GetCurrentThreadId()!=thread_||closing_||failed_||retiring_||pending_close_||native_close_capture_)
            throw std::runtime_error("Existing viewport route retired");
        borrowed_binding_->require_same_input(input_);
        if(event.worldview_input&&worldview_source_bound_){
            event.worldview_input->require_entered();
            {const auto actual=worldview_source_.lock();
                if(!actual)throw std::runtime_error("Existing viewport native Source retired");
                actual->capture_worldview_native_input_owned(event.worldview_input);}
            event.worldview_input->require_entered();
            if(worldview_source_.expired())throw std::runtime_error("Existing viewport Source retired after capture destruction");
        }
        retain_captured_native_close(event);
        // Native receipt is captured now. Full target/GL checks and sink
        // dispatch remain after the native procedure's actual Return.
        if(existing_events_.size()>=4096)throw std::runtime_error("Existing viewport event queue overflow");
        if(event.kind==ClientWindowEventKind::activation)return; // No queued expired WM_ACTIVATE loan.
        existing_events_.push_back(event);borrowed_binding_->require_current();
    }
    void drain_existing(){
        if(!borrowed_binding_)return;borrowed_binding_->require_idle();
        while(!existing_events_.empty()){
            auto event=std::move(existing_events_.front());existing_events_.pop_front();
            on_event(event,true);borrowed_binding_->require_idle();
        }
    }
    std::optional<Native> native_;
    std::optional<Ticket> published_;
    SourceTargetsCurrent published_gate_;
    std::weak_ptr<runtime::BnwSelectedStockLuaSourceOwner> worldview_source_;
    bool worldview_source_bound_=false;
    std::shared_ptr<const BnwWorldViewWindowInputLoan> native_close_capture_;
    std::shared_ptr<const BnwWorldViewWindowInputLoan> pending_close_;
    std::optional<Ticket> pending_close_ticket_;
    std::uint64_t version_ = 0;
    bool failed_ = false, closing_ = false, retiring_ = false;
    bool committing_ = false, checking_ = false;
    };
    const std::shared_ptr<Impl> impl_;
    friend class detail::CivisFrontendPartyRoutingOwner;
    friend class detail::CivisFrontendPartyRendererBackend;
    Win32ViewportTransaction(std::shared_ptr<const Win32ClientWindowBinding::ExistingInputLoan> input,
        ClientWindowInput& actual,Retire retire,Win32ClientWindowBinding::Sink sink)
        :impl_(std::make_shared<Impl>(input->window(),actual,std::move(retire),std::move(sink))){
        impl_->attach_existing(this,std::move(input));
    }
    void capture_existing_event_owned(const ClientWindowEvent& event){impl_->capture_existing(event);}
    void drain_existing_events_owned(){impl_->drain_existing();}
    void require_worldview_input_source_owned(
        const std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner>& source)const {
        if(GetCurrentThreadId()!=impl_->thread_||!source||!impl_->worldview_source_bound_)
            throw std::runtime_error("Party route requires actual bound Camera Source");
        const auto bound=impl_->worldview_source_.lock();
        const auto canonical=source->retain_action_factory_owner();
        if(!bound||bound.get()!=source.get()||bound.owner_before(source)||source.owner_before(bound)||
           !canonical||canonical.get()!=source.get()||canonical.owner_before(source)||source.owner_before(canonical))
            throw std::runtime_error("Party Source control changed before input routing");
    }
    // Only the real camera loop binds its retained canonical Source member.
    // No public subscription flag or callback can claim captured execution.
    void bind_worldview_input_source_owned(const std::weak_ptr<runtime::BnwSelectedStockLuaSourceOwner>& source){
        if(GetCurrentThreadId()!=impl_->thread_)throw std::runtime_error("Viewport input source bind foreign creator");
        const auto operation=impl_;
        if(operation->closing_||operation->failed_||operation->retiring_||operation->checking_||
           operation->committing_||operation->pending_close_||operation->worldview_source_bound_)
            throw std::runtime_error("Viewport cannot replace native input Source channel");
        {
            const auto actual=source.lock();
            if(!actual)throw std::runtime_error("Viewport native Source member absent");
            const auto canonical=actual->retain_action_factory_owner();
            if(!canonical||canonical.get()!=actual.get()||canonical.owner_before(source)||source.owner_before(canonical))
                throw std::runtime_error("Viewport native Source member is not factory canonical control");
        }
        if(source.expired())throw std::runtime_error("Viewport canonical Source retired before publication");
        operation->worldview_source_=source;operation->worldview_source_bound_=true;
    }
    void require_staged_native_close_owned(const std::shared_ptr<const BnwWorldViewWindowInputLoan>& actual)const {
        const auto operation=impl_;operation->require_staged_native_close(actual);
    }
    void finalize_native_close(const std::shared_ptr<const BnwWorldViewWindowInputLoan>& actual) {
        const auto operation=impl_;operation->finalize_native_close(actual);
    }
  public:
    Win32ViewportTransaction(HWND window,ClientWindowInput& input,Retire retire,
                             Win32ClientWindowBinding::Sink sink)
        :impl_(std::make_shared<Impl>(window,input,std::move(retire),std::move(sink))) {
        impl_->attach(this);
    }
    ~Win32ViewportTransaction(){impl_->close();}
    Win32ViewportTransaction(const Win32ViewportTransaction&)=delete;
    Win32ViewportTransaction& operator=(const Win32ViewportTransaction&)=delete;
    void require_activation_window_owner(const BnwAudioWindowActivationLoan& loan)const {
        loan.require_current();const auto operation=impl_;
        operation->require_activation_window_owner(loan);
    }
    // Closed initial physical renderer source, before native UI initialization.
    // No raw HWND/identity DTO/bool can create this owner.
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>
        capture_renderer_window_owner(const std::shared_ptr<graphics::GlContext>& actual)const;
    void abort_native_close()noexcept {const auto operation=impl_;operation->abort_native_close();}
    bool failed()const noexcept {const auto operation=impl_;return operation->failed();}
    std::optional<Ticket> request()noexcept {const auto operation=impl_;return operation->request();}
    bool commit(const Ticket& ticket,std::span<const ClientRect> hits,bool modal,bool text,
                SourceTargetsCurrent gate)noexcept {
        const auto operation=impl_;return operation->commit(ticket,hits,modal,text,std::move(gate));
    }
    bool current(const Ticket& ticket,const SourceTargetsCurrent& gate)noexcept {
        const auto operation=impl_;return operation->current(ticket,gate);
    }
};
} // namespace civ5::ui
#endif
