#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "gl_context_internal.hpp"
#include "civ5/graphics/detail/bnw_renderer_context_pin.hpp"
#include "civ5/graphics/detail/bnw_owned_gl_retirement_node.hpp"
#include "civ5/ui/win32_viewport_transaction.hpp"
#include <commctrl.h>
#include <atomic>
#include <limits>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <utility>
#include "detail/bnw_owned_gl_retirement_node_core.inc"
namespace civ5::graphics {
namespace {void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}}
struct BnwOwnedCameraRendererWindowLease::Impl final {
    using Transaction=ui::Win32ViewportTransaction;
    inline static constexpr wchar_t property[]=L"Civis.OwnedRenderer.InitialWindow";
    const std::thread::id thread=std::this_thread::get_id();
    const DWORD native_thread=GetCurrentThreadId(),native_process=GetCurrentProcessId();
    const std::shared_ptr<GlContextState> physical;
    const std::weak_ptr<GlContext> outside_context;
    const std::shared_ptr<Transaction::Impl> transaction;
    const Transaction::Ticket ticket;
    const HWND window;
    const HDC dc;
    const HGLRC context;
    std::atomic<bool> retired{false};
    bool loan_active=false,close_observer_active=false;
    std::weak_ptr<const BnwOwnedCameraRendererWindowLease> facade;
    const BnwOwnedCameraRendererWindowLease* facade_get=nullptr;
    struct Registration {const std::shared_ptr<Impl> storage;};
    Registration* registration=nullptr;
    // Foreign last-release cannot destroy real WGL storage on the wrong thread.
    // A private retirement queue holds only closed storage until an actual
    // creator operation/native observer can release it on that creator thread.
    struct Deferred {std::mutex mutex;std::vector<std::shared_ptr<Impl>> storage;};
    static Deferred& deferred(){static auto* value=new Deferred;return *value;}
    static UINT cleanup_message(){static const UINT value=RegisterWindowMessageW(L"Civis.OwnedRenderer.CreatorCleanup");return value;}
    static void defer(const std::shared_ptr<Impl>& storage){
        auto& queue=deferred();std::lock_guard lock(queue.mutex);queue.storage.push_back(storage);
    }
    static void drain_creator(){
        std::vector<std::shared_ptr<Impl>> retired_here;
        {auto& queue=deferred();std::lock_guard lock(queue.mutex);
            for(auto i=queue.storage.begin();i!=queue.storage.end();){
                if((*i)->thread==std::this_thread::get_id()){
                    retired_here.push_back(std::move(*i));i=queue.storage.erase(i);
                }else ++i;
            }
        }
        for(const auto& storage:retired_here)storage->detach();
        // Strong physical storage releases only after dropping the queue lock.
    }
    Impl(const std::shared_ptr<GlContextState>& actual,const std::shared_ptr<GlContext>& facade,
        std::shared_ptr<Transaction::Impl> viewport,Transaction::Ticket issued)
        :physical(actual),outside_context(facade),transaction(std::move(viewport)),ticket(std::move(issued)),
         window(actual->window),dc(actual->dc),context(actual->context){}
    static LRESULT CALLBACK observer(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data) {
        const auto* registered=reinterpret_cast<Registration*>(data);
        const auto storage=registered->storage;
        if(storage->thread!=std::this_thread::get_id()||GetCurrentThreadId()!=storage->native_thread){
            storage->retired.store(true);return DefSubclassProc(window,message,wp,lp);
        }
        if(message==WM_CLOSE){
            // The request arrives before its real binding receipt exists.
            // Retain the live physical source through the native continuation;
            // a cancelled close creates no selected terminal authority.
            if(storage->close_observer_active){storage->retired.store(true);return DefSubclassProc(window,message,wp,lp);}
            storage->close_observer_active=true;
            const auto before=storage->transaction->native_close_capture_;
            const auto result=DefSubclassProc(window,message,wp,lp);
            try{
                const auto native=storage->transaction->native_close_capture_;
                if(native.get()!=before.get()){
                    storage->current();
                    need(native&&!before&&native->message_==WM_CLOSE&&native->w_==wp&&native->l_==lp&&
                         native->state_->window==window&&native->state_->thread==storage->native_thread&&
                         native->state_->binding==storage->transaction->actual_binding_ptr()&&
                         native->state_->input==&storage->transaction->input_,
                         "Renderer close observer has another actual native occurrence");
                    native->require_completed();storage->current();
                    need(!storage->facade.expired()&&storage->facade_get&&native->close_window_owner_.expired()&&
                         !native->close_window_get_,"Renderer close observer owner retired or already issued");
                    native->close_window_owner_=storage->facade;native->close_window_get_=storage->facade_get;
                    native->require_completed();storage->current();
                }else{
                    need(!before,"Renderer close request repeated after selected capture");
                    // Never-bound diagnostic close legitimately retires its
                    // ticket inside the native sink. Its expected retirement
                    // is not a failed selected-source transition. A cancelled
                    // menu close retains only a genuinely still-current source.
                    try{storage->current();}catch(...){storage->retired.store(true);}
                }
            }catch(...){storage->retired.store(true);storage->transaction->failed_=true;}
            storage->close_observer_active=false;drain_creator();return result;
        }
        if(message==WM_SIZE||message==WM_DPICHANGED)storage->retired.store(true);
        if(message==WM_NCDESTROY){
            storage->retired.store(true);
            if(GetPropW(window,property)==registered)RemovePropW(window,property);
            RemoveWindowSubclass(window,observer,id);
            storage->registration=nullptr;delete registered;
        }
        drain_creator();
        return DefSubclassProc(window,message,wp,lp);
    }
    void current(bool require_registration=true)const {
        need(thread==std::this_thread::get_id()&&GetCurrentThreadId()==native_thread,
            "Initial renderer window foreign creator thread");
        need(!retired.load()&&physical&&transaction&&!physical->facade_retired_.load()&&
             outside_context.use_count()>static_cast<long>(physical->renderer_context_pins_),
            "Initial renderer window source owner retired");
        need(physical->owner==thread&&physical->window==window&&physical->dc==dc&&physical->context==context&&
             window&&dc&&context&&IsWindow(window)&&!IsIconic(window),"Initial renderer physical state changed");
        DWORD process=0;
        need(GetWindowThreadProcessId(window,&process)==native_thread&&process==native_process,
            "Initial renderer native process/thread changed");
        wchar_t actual_class[256]{};
        need(GetClassNameW(window,actual_class,256)>0&&physical->window_class==actual_class&&
             reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(window,GWLP_HINSTANCE))==physical->module,
             "Initial renderer native class/module changed");
        need(wglGetCurrentContext()==context&&wglGetCurrentDC()==dc&&WindowFromDC(dc)==window,
            "Initial renderer SAME WGL/DC is not already current; context switching refused");
        // Read real private native transaction facts, never a caller gate.
        need(!transaction->closing_&&!transaction->failed_&&!transaction->retiring_&&
             !transaction->committing_&&!transaction->checking_&&transaction->actual_binding_ptr()&&
             !transaction->actual_binding_ptr()->failed()&&transaction->window_==window&&transaction->thread_==native_thread&&
             transaction->ticket_owned(ticket)&&transaction->version_==ticket.version()&&
             GetPropW(window,Transaction::Impl::property_)==transaction->facade_&&
             GetPropW(window,L"Civis.ProductionClientWindow")==transaction->actual_binding_ptr(),
             "Initial renderer actual viewport owner/ticket retired");
        const auto actual=transaction->read_native();
        need(actual&&!actual->minimized&&actual->width>0&&actual->height>0&&
             ticket.viewport()==ui::ClientViewport{actual->width,actual->height,actual->width,actual->height,actual->dpi},
             "Initial renderer actual native viewport differs from issued ticket");
        if(require_registration){DWORD_PTR actual_observer=0;
            need(registration&&GetPropW(window,property)==registration&&
                 GetWindowSubclass(window,observer,reinterpret_cast<UINT_PTR>(registration),&actual_observer)&&
                 actual_observer==reinterpret_cast<DWORD_PTR>(registration),
                 "Initial renderer native occurrence observer retired");
        }
        const auto after=transaction->read_native();
        need(after&&*after==*actual&&!retired.load()&&outside_context.use_count()>static_cast<long>(physical->renderer_context_pins_)&&
             !physical->facade_retired_.load()&&
             !transaction->closing_&&!transaction->failed_&&!transaction->retiring_&&
             !transaction->committing_&&!transaction->checking_&&transaction->actual_binding_ptr()&&!transaction->actual_binding_ptr()->failed()&&
             GetPropW(window,Transaction::Impl::property_)==transaction->facade_&&
             GetPropW(window,L"Civis.ProductionClientWindow")==transaction->actual_binding_ptr()&&
             transaction->version_==ticket.version()&&wglGetCurrentContext()==context&&wglGetCurrentDC()==dc&&
             WindowFromDC(dc)==window&&IsWindow(window)&&!IsIconic(window)&&
             GetWindowThreadProcessId(window,&process)==native_thread&&process==native_process,
             "Initial renderer source changed during native observation");
    }
    void attach(const std::shared_ptr<Impl>& storage) {
        current(false);need(cleanup_message()!=0&&!GetPropW(window,property),"Initial renderer window already issued/cleanup unavailable");
        auto owned=std::make_unique<Registration>(Registration{storage});
        need(SetPropW(window,property,owned.get())!=FALSE,"Initial renderer occurrence property installation failed");
        if(!SetWindowSubclass(window,observer,reinterpret_cast<UINT_PTR>(owned.get()),reinterpret_cast<DWORD_PTR>(owned.get()))){
            if(GetPropW(window,property)==owned.get())RemovePropW(window,property);
            throw std::runtime_error("Initial renderer occurrence observer installation failed");
        }
        registration=owned.release();current();
    }
    void detach()noexcept {
        retired.store(true);
        if(thread!=std::this_thread::get_id()||GetCurrentThreadId()!=native_thread)return;
        if(!registration)return;
        auto* owned=registration;DWORD_PTR actual=0;
        if(!IsWindow(window)){
            // NCDESTROY normally already removed it; no OS target remains.
            registration=nullptr;delete owned;return;
        }
        if(GetWindowSubclass(window,observer,reinterpret_cast<UINT_PTR>(owned),&actual)&&
            actual==reinterpret_cast<DWORD_PTR>(owned)){
            if(!RemoveWindowSubclass(window,observer,reinterpret_cast<UINT_PTR>(owned)))return;
            if(GetPropW(window,property)==owned)RemovePropW(window,property);
            registration=nullptr;delete owned;
        }
        // A remaining actual OS registration holds storage until NCDESTROY.
    }
};
void BnwOwnedCameraRendererWindowLease::Loan::require_context_gui_client_profile_owned()const {
    require_current();const auto p=owner_.impl_;
    need(p->physical->context_gui_client_creation_==p->window&&p->window!=nullptr,
        "Context GUI profile is not the actual created Civis client window");
    require_current();
}
class BnwOwnedCameraRendererWindowLease::Pin final {
    const BnwOwnedCameraRendererWindowLease& owner_;
    const std::shared_ptr<const BnwOwnedCameraRendererWindowLease> storage_;
  public:
    explicit Pin(const BnwOwnedCameraRendererWindowLease& owner)
        :owner_(owner),storage_(owner.self_.lock()) {
        need(storage_&&storage_.get()==&owner_&&owner_.self_pins_<static_cast<std::size_t>(std::numeric_limits<long>::max()),
            "Initial renderer lease actual storage/pin overflow");++owner_.self_pins_;
    }
    ~Pin(){--owner_.self_pins_;}
    const std::shared_ptr<const BnwOwnedCameraRendererWindowLease>& canonical_owner()const noexcept{return storage_;}
    void current()const {
        need(storage_.use_count()>static_cast<long>(owner_.self_pins_),
            "Initial renderer lease lost genuine outside owner during callback");
        owner_.impl_->current();
        need(storage_.use_count()>static_cast<long>(owner_.self_pins_),
            "Initial renderer lease outside owner retired during native observation");
    }
};
BnwOwnedCameraRendererWindowLease::BnwOwnedCameraRendererWindowLease(std::shared_ptr<Impl> actual):impl_(std::move(actual)){}
void BnwOwnedCameraRendererWindowLease::require_origin_lifetime_without_native_callbacks()const {
    need(thread_==std::this_thread::get_id(),"Renderer origin foreign creator");
    Pin pin(*this);const auto& canonical=pin.canonical_owner();const auto p=impl_;
    need(canonical.get()==this&&!canonical.owner_before(self_)&&!self_.owner_before(canonical)&&
         canonical.use_count()>static_cast<long>(self_pins_),"Renderer origin canonical facade retired");
    need(p->thread==thread_&&!p->retired.load()&&p->physical&&
         p->outside_context.use_count()>static_cast<long>(p->physical->renderer_context_pins_)&&
         !p->physical->facade_retired_.load()&&p->physical->owner==thread_&&
         p->physical->window==p->window&&p->physical->dc==p->dc&&p->physical->context==p->context&&
         p->window&&p->dc&&p->context&&p->registration,
         "Renderer origin actual physical/observer lifetime retired");
    need(p->transaction&&!p->transaction->closing_&&!p->transaction->failed_&&
         !p->transaction->retiring_&&!p->transaction->committing_&&!p->transaction->checking_&&
         p->transaction->actual_binding_ptr()&&!p->transaction->actual_binding_ptr()->failed()&&
         p->transaction->window_==p->window&&p->transaction->thread_==p->native_thread&&
         p->transaction->ticket_owned(p->ticket)&&p->transaction->version_==p->ticket.version(),
         "Renderer origin actual viewport source/ticket retired");
    need(p->outside_context.use_count()>static_cast<long>(p->physical->renderer_context_pins_)&&
         !p->physical->facade_retired_.load(),"Renderer origin outside Context retired during pure gate");
    // No geometry query, callback, WGL switch or ticket/provider currentness.
}
BnwOwnedCameraRendererWindowLease::RendererPin::RendererPin(
 const std::shared_ptr<const BnwOwnedCameraRendererWindowLease>& retained):owner_(retained) {
 need(owner_&&owner_->thread_==std::this_thread::get_id(),"Renderer Window pin absent/foreign creator");
 {const auto canonical=owner_->self_.lock();
  need(canonical&&canonical.get()==owner_.get()&&!canonical.owner_before(owner_)&&!owner_.owner_before(canonical),
       "Renderer Window pin no-op/foreign control");}
 need(owner_->self_pins_<static_cast<std::size_t>(std::numeric_limits<long>::max()),"Renderer Window pin overflow");
 ++owner_->self_pins_;
 try{require_current_without_native_callbacks();}catch(...){--owner_->self_pins_;throw;}
}
BnwOwnedCameraRendererWindowLease::RendererPin::~RendererPin(){--owner_->self_pins_;}
void BnwOwnedCameraRendererWindowLease::RendererPin::require_current_without_native_callbacks()const {
 need(owner_&&owner_->thread_==std::this_thread::get_id(),"Renderer Window pin retired/foreign creator");
 {const auto canonical=owner_->self_.lock();
  need(canonical&&canonical.get()==owner_.get()&&!canonical.owner_before(owner_)&&!owner_.owner_before(canonical),
       "Renderer Window pinned slot/control changed");}
 // The temporary control lock above is gone. The existing genuine pure gate
 // accounts its own Pin plus this exact retained lender through self_pins_.
 owner_->require_origin_lifetime_without_native_callbacks();
}
BnwOwnedCameraRendererWindowLease::~BnwOwnedCameraRendererWindowLease(){
    impl_->retired.store(true);
    if(thread_!=std::this_thread::get_id()){
        Impl::defer(impl_);
        // Registration still owns actual storage. Only its creator observer
        // removes that OS target; a failed post retains closed storage safely.
        PostMessageW(impl_->window,Impl::cleanup_message(),0,0);return;
    }
    (void)detail::BnwOwnedGlRetirementNode::drain_current_creator_owned();
    impl_->detach();
}
std::shared_ptr<const BnwOwnedCameraRendererWindowLease> BnwOwnedCameraRendererWindowLease::capture_from_actual_transaction(
    const std::shared_ptr<GlContext>& input,const ui::Win32ViewportTransaction& viewport) {
    std::shared_ptr<const BnwOwnedCameraRendererWindowLease> result;
    const auto physical=input?input->state_:std::shared_ptr<GlContextState>{};
    need(physical&&physical->owner==std::this_thread::get_id()&&!physical->facade_retired_.load(),
         "Initial renderer physical context absent/foreign/retired");
    {
        // Same sealed constructor-only count mechanism as the Pin getter.
        detail::BnwRendererContextPin::Capture counted_capture(std::weak_ptr<GlContext>(input),physical);
        const auto& outside=counted_capture.context_;
        const auto& canonical=counted_capture.canonical_;
        const auto native=counted_capture.identity_;
        const auto context_origin=[&]{
            need(physical->owner==std::this_thread::get_id()&&!physical->facade_retired_.load()&&
                 canonical.use_count()>static_cast<long>(physical->renderer_context_pins_)&&
                 outside&&!canonical.owner_before(outside)&&!outside.owner_before(canonical)&&
                 outside->state_.get()==physical.get()&&!outside->state_.owner_before(physical)&&
                 !physical.owner_before(outside->state_),"Initial renderer capture outside/state/control retired");
            const auto after=outside->native_identity();
            need(after.owner_thread&&after.window==native.window&&after.dc==native.dc&&after.context==native.context&&
                 after.window==reinterpret_cast<std::uintptr_t>(physical->window)&&
                 after.dc==reinterpret_cast<std::uintptr_t>(physical->dc)&&
                 after.context==reinterpret_cast<std::uintptr_t>(physical->context)&&
                 canonical.use_count()>static_cast<long>(physical->renderer_context_pins_),
                 "Initial renderer capture immutable identity/outside retired");
        };
        context_origin();
        {
            const auto transaction=viewport.impl_;
            need(transaction&&GetCurrentThreadId()==transaction->thread_,"Initial renderer actual transaction foreign thread");
            context_origin();Impl::drain_creator();context_origin();
            context_origin();const auto ticket=transaction->request();context_origin();
            need(bool(ticket),"Initial renderer actual ticket absent during capture");
            const auto actual=std::make_shared<Impl>(physical,outside,transaction,*ticket);
            context_origin();actual->current(false);context_origin();
            auto lease=std::shared_ptr<BnwOwnedCameraRendererWindowLease>(new BnwOwnedCameraRendererWindowLease(actual));
            lease->self_=lease;actual->facade=lease;actual->facade_get=lease.get();context_origin();
            need(!physical->renderer_creator_cleanup_||physical->renderer_creator_cleanup_==&drain_retired_creator_storage,
                "Initial renderer actual cleanup issuer changed");
            physical->renderer_creator_cleanup_=&drain_retired_creator_storage;
            context_origin();actual->attach(actual);context_origin();
            actual->current();context_origin();lease->require_current();context_origin();
            result=std::move(lease);
        } // All transaction/ticket/installation temporary cleanup before pure POST.
        context_origin();result->require_origin_lifetime_without_native_callbacks();context_origin();
    } // Counted capture drops its Context slot BEFORE final outside-authority gate.
    result->require_origin_lifetime_without_native_callbacks();
    return result;
}

namespace detail {
BnwRendererContextPin::Capture::Capture(std::weak_ptr<GlContext> canonical,std::shared_ptr<GlContextState> physical)
 :physical_(std::move(physical)),canonical_(std::move(canonical)),thread_(std::this_thread::get_id()) {
 // All throwing preconditions happen before acquiring any Context strong.
 need(physical_&&physical_->owner==thread_&&!physical_->facade_retired_.load(),
      "Renderer Context capture absent/foreign/retired physical state");
 need(physical_->renderer_context_pins_<static_cast<std::size_t>(std::numeric_limits<long>::max()-1),
      "Renderer Context actual pin count overflow");
 context_=canonical_.lock(); // noexcept; immediately count this exact slot.
 if(!context_)throw std::runtime_error("Renderer Context capture expired actual facade");
 ++physical_->renderer_context_pins_;engaged_=true;
 struct CaptureRefund final {
  std::size_t& count;std::shared_ptr<GlContext>& slot;bool& engaged;bool committed=false;
  ~CaptureRefund(){if(!committed){--count;engaged=false;slot.reset();}}
 } refund{physical_->renderer_context_pins_,context_,engaged_};
 need(canonical_.use_count()>static_cast<long>(physical_->renderer_context_pins_)&&
      !canonical_.owner_before(context_)&&!context_.owner_before(canonical_)&&
      context_->state_.get()==physical_.get()&&!context_->state_.owner_before(physical_)&&
      !physical_.owner_before(context_->state_),"Renderer Context pin canonical outside owner lost");
 identity_=context_->native_identity();
 need(identity_.owner_thread&&
      identity_.window==reinterpret_cast<std::uintptr_t>(physical_->window)&&
      identity_.dc==reinterpret_cast<std::uintptr_t>(physical_->dc)&&
      identity_.context==reinterpret_cast<std::uintptr_t>(physical_->context),
      "Renderer Context capture immutable identity changed");
 refund.committed=true;
}
BnwRendererContextPin::Capture::~Capture(){
 if(!engaged_)return;
 if(std::this_thread::get_id()!=thread_||!physical_||!physical_->renderer_context_pins_)std::terminate();
 --physical_->renderer_context_pins_;engaged_=false;context_.reset();
}
BnwRendererContextPin::BnwRendererContextPin(Capture& captured)
 :context_(std::move(captured.context_)),physical_(captured.physical_),canonical_(captured.canonical_),
  thread_(captured.thread_),identity_(captured.identity_) {
 // Move the SAME ONE strong slot and its already-acquired count. No callback,
 // increment, or uncounted interval occurs in this ownership transfer.
 if(!captured.engaged_)std::terminate();captured.engaged_=false;
 struct ConstructorRefund final {
  std::size_t& count;bool committed=false;
  ~ConstructorRefund(){if(!committed)--count;}
 } refund{physical_->renderer_context_pins_};
 first_without_callbacks();refund.committed=true;
}
BnwRendererContextPin::~BnwRendererContextPin(){
 // Closed receivers defer their GPU storage to this creator. Fail fast rather
 // than undercounting a foreign still-live Context or deleting WGL abroad.
 if(std::this_thread::get_id()!=thread_||!physical_||!physical_->renderer_context_pins_)std::terminate();
 --physical_->renderer_context_pins_;
 // Members release without any intervening user/native callback. Context is
 // last in member destruction order; its own cleanup runs after the drop.
}
void BnwRendererContextPin::first_without_callbacks()const {
 need(std::this_thread::get_id()==thread_&&physical_&&physical_->owner==thread_&&context_&&
      !physical_->facade_retired_.load()&&physical_->renderer_context_pins_&&
      canonical_.use_count()>static_cast<long>(physical_->renderer_context_pins_),
      "Renderer Context original outside authority retired");
 // No temporary Context strong lock, no native liveness/reset/current query.
 need(!canonical_.owner_before(context_)&&!context_.owner_before(canonical_)&&
      context_->state_.get()==physical_.get()&&!context_->state_.owner_before(physical_)&&
      !physical_.owner_before(context_->state_),"Renderer Context actual state/control changed");
 const auto native=context_->native_identity();
 need(native.owner_thread&&native.window==identity_.window&&native.dc==identity_.dc&&
      native.context==identity_.context&&identity_.window==reinterpret_cast<std::uintptr_t>(physical_->window)&&
      identity_.dc==reinterpret_cast<std::uintptr_t>(physical_->dc)&&
      identity_.context==reinterpret_cast<std::uintptr_t>(physical_->context),
      "Renderer Context actual immutable identity retired");
 need(canonical_.use_count()>static_cast<long>(physical_->renderer_context_pins_)&&
      !physical_->facade_retired_.load(),"Renderer Context outside retired during pure pin gate");
}
const GlApi* BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(
 const BnwRendererContextPin& pin)noexcept {
 // Snapshot immutable custody before native observations. State retention and
 // weak control copies never clone a Context strong owner or admit Source.
 const auto physical=pin.physical_;
 const auto creator=pin.thread_;
 const auto issued=pin.identity_;
 const auto* const facade_identity=pin.context_.get(); // Identity only, never dereferenced.
 const std::weak_ptr<GlContext> retained_control=pin.context_;
 const auto canonical=pin.canonical_;
 const auto physical_origin=[&]()noexcept {
  // Creator refusal precedes reading the creator-only actual pin counter.
  return std::this_thread::get_id()==creator&&physical&&physical->owner==creator&&
   facade_identity&&physical->renderer_context_pins_&&issued.owner_thread&&
   issued.window&&issued.dc&&issued.context&&
   !canonical.owner_before(retained_control)&&!retained_control.owner_before(canonical)&&
   issued.window==reinterpret_cast<std::uintptr_t>(physical->window)&&
   issued.dc==reinterpret_cast<std::uintptr_t>(physical->dc)&&
   issued.context==reinterpret_cast<std::uintptr_t>(physical->context);
 };
 const auto native_current=[&]()noexcept {
  return reinterpret_cast<std::uintptr_t>(wglGetCurrentContext())==issued.context&&
   reinterpret_cast<std::uintptr_t>(wglGetCurrentDC())==issued.dc&&
   reinterpret_cast<std::uintptr_t>(WindowFromDC(reinterpret_cast<HDC>(issued.dc)))==issued.window;
 };
 if(!physical_origin()||!native_current()||!physical_origin())return nullptr;
 const auto* const api=&physical->gl;
 if(!physical_origin()||!native_current()||!physical_origin())return nullptr;
 // SAME node must retain SAME Pin through use and deletion-effect POST.
 // No facade-retired/outside-owner/Source/Paint check, no WGL bind/switch.
 return api;
}
const GlApi* BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(
 std::shared_ptr<const BnwRendererContextPin> actual_pin){
 // SAME Pin custody is captured by VALUE before all native observations.
 const auto pin=std::move(actual_pin);
 need(bool(pin),"Normal Renderer Context loaded dispatch Pin absent");
 pin->first_without_callbacks();
 const auto physical=pin->physical_;
 const auto canonical=pin->canonical_;
 const auto creator=pin->thread_;
 const auto issued=pin->identity_;
 const auto*const facade_identity=pin->context_.get();
 const std::weak_ptr<GlContext> retained_control=pin->context_;
 const auto normal_first=[&]{
  pin->first_without_callbacks();
  need(std::this_thread::get_id()==creator&&physical&&physical->owner==creator&&
       !physical->facade_retired_.load()&&facade_identity&&
       pin->context_.get()==facade_identity&&pin->physical_.get()==physical.get()&&
       !canonical.owner_before(retained_control)&&!retained_control.owner_before(canonical)&&
       issued.owner_thread&&issued.window&&issued.dc&&issued.context&&
       issued.window==reinterpret_cast<std::uintptr_t>(physical->window)&&
       issued.dc==reinterpret_cast<std::uintptr_t>(physical->dc)&&
       issued.context==reinterpret_cast<std::uintptr_t>(physical->context),
       "Normal Renderer Context captured original identity retired");
  pin->first_without_callbacks();
 };
 const auto native_current=[&]{
  normal_first();
  const auto actual_context=reinterpret_cast<std::uintptr_t>(wglGetCurrentContext());normal_first();
  const auto actual_dc=reinterpret_cast<std::uintptr_t>(wglGetCurrentDC());normal_first();
  const auto actual_window=reinterpret_cast<std::uintptr_t>(
      WindowFromDC(reinterpret_cast<HDC>(issued.dc)));normal_first();
  need(actual_context==issued.context&&actual_dc==issued.dc&&actual_window==issued.window,
       "Normal Renderer Context loaded dispatch requires SAME current HWND/DC/WGL");
  normal_first();
 };
 normal_first();native_current();normal_first();
 const auto*const api=&physical->gl;
 normal_first();native_current();normal_first();
 // No make-current/proc load/Context strong clone; reset/error are caller gates.
 return api;
}
GlContext& BnwRendererContextPin::context_reference_without_callbacks()const {
 first_without_callbacks();return *context_;
}
} // namespace detail
std::shared_ptr<const detail::BnwRendererContextPin>
BnwOwnedCameraRendererWindowLease::retain_context_pin_without_native_callbacks()const {
 const auto p=impl_; // Capture physical storage before the pure origin gate.
 need(thread_==std::this_thread::get_id(),"Renderer Context capture foreign creator");
 Pin receiver(*this);const auto& canonical_window=receiver.canonical_owner();
 canonical_window->require_origin_lifetime_without_native_callbacks();
 detail::BnwRendererContextPin::Capture captured(p->outside_context,p->physical);
 const auto native=captured.identity_;
 need(native.owner_thread&&native.window==reinterpret_cast<std::uintptr_t>(p->window)&&
      native.dc==reinterpret_cast<std::uintptr_t>(p->dc)&&native.context==reinterpret_cast<std::uintptr_t>(p->context),
      "Renderer Context capture SAME actual identity changed");
 // Object/control allocations may invoke new-handler. The captured Context
 // slot is already counted throughout; constructor transfers the SAME count.
 auto result=std::shared_ptr<const detail::BnwRendererContextPin>(
     new detail::BnwRendererContextPin(captured));
 result->first_without_callbacks();
 canonical_window->require_origin_lifetime_without_native_callbacks();
 result->first_without_callbacks();
 return result;
}
void BnwOwnedCameraRendererWindowLease::require_retained_context_pin_origin_without_callbacks(
 const std::weak_ptr<const BnwOwnedCameraRendererWindowLease>& window,
 const std::shared_ptr<const detail::BnwRendererContextPin>& pin) {
 const auto held=window.lock();
 need(held&&held->thread_==std::this_thread::get_id(),"Retained renderer Window absent/foreign creator");
 need(held->self_pins_<static_cast<std::size_t>(std::numeric_limits<long>::max()),"Retained renderer Window pin overflow");
 ++held->self_pins_;
 struct CountedLock final {std::size_t& count;~CountedLock(){--count;}} counted{held->self_pins_};
 held->require_origin_lifetime_without_native_callbacks();
 need(bool(pin),"Retained renderer Context pin missing");pin->first_without_callbacks();
 const auto p=held->impl_;
 need(pin->thread_==held->thread_&&pin->physical_.get()==p->physical.get()&&
  !pin->physical_.owner_before(p->physical)&&!p->physical.owner_before(pin->physical_)&&
  !pin->canonical_.owner_before(p->outside_context)&&!p->outside_context.owner_before(pin->canonical_),
  "Retained Context pin canonical physical/control differs from Window origin");
 need(pin->identity_.owner_thread&&pin->identity_.window==reinterpret_cast<std::uintptr_t>(p->window)&&
  pin->identity_.dc==reinterpret_cast<std::uintptr_t>(p->dc)&&
  pin->identity_.context==reinterpret_cast<std::uintptr_t>(p->context),
  "Retained Context pin native identity differs from actual Window");
 pin->first_without_callbacks();held->require_origin_lifetime_without_native_callbacks();
 // counted destroys before held: this temporary strong never grants outside ownership.
}
void BnwOwnedCameraRendererWindowLease::require_current()const {
    need(thread_==std::this_thread::get_id(),"Initial renderer lease foreign creator thread");
    Pin retained(*this);retained.current();
}
void BnwOwnedCameraRendererWindowLease::with_actual_window_loan(const std::function<void(const Loan&)>& input)const {
    need(thread_==std::this_thread::get_id(),"Initial renderer loan foreign creator thread");
    Pin retained(*this);
    if(impl_->loan_active){impl_->retired.store(true);throw std::runtime_error("Initial renderer window loan reentry");}
    impl_->loan_active=true;
    struct Exit {Impl& state;~Exit(){state.loan_active=false;}} exit{*impl_};
    try{
        retained.current();
        {const auto consume=input;retained.current();need(bool(consume),"Initial renderer window loan consumer absent");
            const Loan loan(*this);loan.require_current();consume(loan);loan.require_current();retained.current();
        }retained.current(); // Includes destruction of the copied callable target.
    }catch(...){impl_->retired.store(true);throw;}
}
std::shared_ptr<const detail::BnwRendererContextPin>
BnwOwnedCameraRendererWindowLease::Loan::retain_context_pin_owned()const {
 need(owner_.thread_==std::this_thread::get_id(),"Renderer Context Loan foreign creator");
 Pin retained(owner_);retained.current();
 need(owner_.impl_->loan_active,"Renderer Context capture outside actual Window Loan");
 const auto captured=owner_.retain_context_pin_without_native_callbacks();
 retained.current();need(owner_.impl_->loan_active,"Renderer Context Window Loan retired during capture");
 return captured;
}
void BnwOwnedCameraRendererWindowLease::Loan::require_current()const {
    owner_.require_current();need(owner_.impl_->loan_active,"Initial renderer stack loan ended");
}
std::shared_ptr<const BnwOwnedCameraRendererWindowLease>
BnwOwnedCameraRendererWindowLease::Loan::retain_owner()const {
    need(owner_.thread_==std::this_thread::get_id(),"Initial renderer canonical retain foreign creator thread");
    Pin retained(owner_);retained.current();
    need(owner_.impl_->loan_active,"Initial renderer canonical retain outside actual stack loan");
    const auto& canonical=retained.canonical_owner();
    need(canonical.get()==&owner_&&!owner_.self_.owner_before(canonical)&&!canonical.owner_before(owner_.self_),
        "Initial renderer canonical owner is not SAME producer control block");
    retained.current();need(owner_.impl_->loan_active,"Initial renderer canonical retain loan retired");
    // No extra unaccounted strong local hides outside retirement during the
    // actual checks. Plain shared-owner copy follows the final native bookend.
    return canonical;
}
void BnwOwnedCameraRendererWindowLease::Loan::require_context_control_owned(const std::weak_ptr<GlContext>& actual)const {
    require_current();const auto p=owner_.impl_;
    need(!actual.expired()&&!actual.owner_before(p->outside_context)&&!p->outside_context.owner_before(actual),
         "Saved source GL control differs from actual renderer context");
    require_current(); // Existing outside-owner, native/current and creator POST unchanged.
}
std::uintptr_t BnwOwnedCameraRendererWindowLease::Loan::window()const{require_current();return reinterpret_cast<std::uintptr_t>(owner_.impl_->window);}
std::uintptr_t BnwOwnedCameraRendererWindowLease::Loan::dc()const{require_current();return reinterpret_cast<std::uintptr_t>(owner_.impl_->dc);}
std::uintptr_t BnwOwnedCameraRendererWindowLease::Loan::wgl_context()const{require_current();return reinterpret_cast<std::uintptr_t>(owner_.impl_->context);}
std::uint32_t BnwOwnedCameraRendererWindowLease::Loan::process()const{require_current();return owner_.impl_->native_process;}
std::uint32_t BnwOwnedCameraRendererWindowLease::Loan::ui_thread()const{require_current();return owner_.impl_->native_thread;}
std::uint64_t BnwOwnedCameraRendererWindowLease::Loan::viewport_revision()const{require_current();return owner_.impl_->ticket.version();}
std::uintptr_t BnwOwnedCameraRendererWindowLease::Loan::window_occurrence()const{require_current();return reinterpret_cast<std::uintptr_t>(owner_.impl_->registration);}
void BnwOwnedCameraRendererWindowLease::drain_retired_creator_storage()noexcept{(void)detail::BnwOwnedGlRetirementNode::drain_current_creator_owned();Impl::drain_creator();}
} // namespace civ5::graphics
namespace civ5::ui {
std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>
Win32ViewportTransaction::capture_renderer_window_owner(const std::shared_ptr<graphics::GlContext>& actual)const {
    return graphics::BnwOwnedCameraRendererWindowLease::capture_from_actual_transaction(actual,*this);
}
}
