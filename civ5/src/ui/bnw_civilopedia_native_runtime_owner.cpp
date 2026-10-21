#include "civ5/ui/bnw_civilopedia_native_runtime_owner.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "bnw_native_completed_color_query.hpp"
#include "civ5/ui/civis_resource_bank_font_color_snapshot.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <bit>
#include <xmmintrin.h>
#include <atomic>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
namespace civ5::ui {
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool same(const runtime::BnwStockLuaClientExtent&a,const runtime::BnwStockLuaClientExtent&b){return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;}
}
struct BnwCivilopediaNativeWindowBinding::Impl {
 static constexpr const wchar_t* property=L"Civis.StockNativeUI.WindowObserver";
 const DWORD thread=GetCurrentThreadId(),process=GetCurrentProcessId();
 HWND window=nullptr;
 std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> renderer;
 BnwCivilopediaNativeWindowBinding::RendererFacts renderer_facts;
 std::atomic<bool>retired{false},poisoned{false};
 bool querying=false;
 runtime::BnwStockLuaClientExtentProvider provider;
 std::optional<runtime::BnwStockLuaClientExtent> expected;
 // The OS registration must not keep canonical renderer/window storage alive.
 // Only a currently executing creator callback locks a strong operation pin.
 struct Registration {
  const std::weak_ptr<Impl> state;
  const DWORD creator_thread;
 };
 Registration* registration=nullptr;
 static LRESULT CALLBACK procedure(HWND hwnd,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
  auto* registration=reinterpret_cast<Registration*>(data);
  // Thread metadata is immutable registration storage, not a borrowed facade.
  if(GetCurrentThreadId()!=registration->creator_thread){
   if(const auto state=registration->state.lock())state->poisoned.store(true);
   return 0; // Never perform creator-only Win32 teardown on a foreign caller.
  }
  const auto state=registration->state.lock(); // Callback-local operation pin only.
  if(!state||state->retired.load()||message==WM_NCDESTROY){
   if(state)state->retired.store(true);
   if(GetPropW(hwnd,property)==registration)RemovePropW(hwnd,property);
   const bool removed=RemoveWindowSubclass(hwnd,procedure,id)!=FALSE;
   if(removed||message==WM_NCDESTROY){
    if(state&&state->registration==registration)state->registration=nullptr;
    delete registration;
   }
  }
  // A typed foreign final release drops Impl and its canonical Window owner.
  // Window3's actual creator-cleanup message reaches this weak observer FIRST;
  // it removes our expired registration, then its own observer drains storage.
  // Never touch registration/facade after downstream callbacks can destroy it.
  return DefSubclassProc(hwnd,message,wp,lp);
 }
 void native_current(RECT* rect=nullptr)const {
  need(GetCurrentThreadId()==thread,"Native UI window observer foreign thread");
  need(!retired.load()&&!poisoned.load(),"Native UI borrowed window binding retired/poisoned");
  const auto owned=renderer; // Strong canonical physical owner across OS queries.
  if(owned)owned->require_current();
  DWORD owner_process=0;DWORD_PTR actual=0;RECT current{};
  need(registration&&IsWindow(window)&&GetWindowThreadProcessId(window,&owner_process)==thread&&owner_process==process&&
       GetPropW(window,property)==registration&&GetWindowSubclass(window,procedure,reinterpret_cast<UINT_PTR>(registration),&actual)&&
       actual==reinterpret_cast<DWORD_PTR>(registration)&&GetClientRect(window,&current),
       "Native UI actual HWND/observer occurrence changed");
  if(owned){
   need(renderer_facts.window==reinterpret_cast<std::uintptr_t>(window)&&
        renderer_facts.process==process&&renderer_facts.thread==thread&&
        renderer_facts.dc==reinterpret_cast<std::uintptr_t>(wglGetCurrentDC())&&
        renderer_facts.wgl==reinterpret_cast<std::uintptr_t>(wglGetCurrentContext())&&
        WindowFromDC(reinterpret_cast<HDC>(renderer_facts.dc))==window,
        "Native UI canonical renderer handles/thread changed");
   owned->require_current();
  }
  if(rect)*rect=current;
 }
 void detach()noexcept {
  retired.store(true);
  // Creator-only Win32 teardown. A foreign release leaves a WEAK OS record;
  // Impl can die and release canonical Window3 ownership immediately. Its real
  // deferred creator message, or NCDESTROY, removes the expired registration.
  if(GetCurrentThreadId()!=thread||!registration)return;
  auto* registered=registration;
  DWORD_PTR actual=0;
  if(IsWindow(window)&&GetWindowSubclass(window,procedure,reinterpret_cast<UINT_PTR>(registered),&actual)&&
     actual==reinterpret_cast<DWORD_PTR>(registered)){
   if(!RemoveWindowSubclass(window,procedure,reinterpret_cast<UINT_PTR>(registered)))return;
   if(GetPropW(window,property)==registered)RemovePropW(window,property);
   registration=nullptr;delete registered;
  }
 }
};
BnwCivilopediaNativeWindowBinding::BnwCivilopediaNativeWindowBinding(std::shared_ptr<Impl>p):impl_(std::move(p)){}
BnwCivilopediaNativeWindowBinding::~BnwCivilopediaNativeWindowBinding(){impl_->detach();}
std::shared_ptr<BnwCivilopediaNativeWindowBinding>BnwCivilopediaNativeWindowBinding::attach(std::uintptr_t window,runtime::BnwStockLuaClientExtentProvider provider){
 return attach_impl(window,std::move(provider),{},{});
}
std::shared_ptr<BnwCivilopediaNativeWindowBinding>BnwCivilopediaNativeWindowBinding::attach_renderer(
 std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> input,
 runtime::BnwStockLuaClientExtentProvider provider){
 need(input&&provider.current,"Actual typed renderer/provider missing");
 std::shared_ptr<BnwCivilopediaNativeWindowBinding> result;
 input->with_actual_window_loan([&](const graphics::BnwOwnedCameraRendererWindowLease::Loan& loan){
  loan.require_current();
  // Graphics-owned seam: canonical issuance, never persistence of a stack Loan
  // or of the incoming potentially forged control block.
  auto canonical=loan.retain_owner();
  need(canonical&&canonical.get()==input.get()&&!canonical.owner_before(input)&&!input.owner_before(canonical),
       "Native UI typed renderer canonical control differs");
  RendererFacts facts;
  facts.window=loan.window();facts.dc=loan.dc();facts.wgl=loan.wgl_context();
  facts.process=loan.process();facts.thread=loan.ui_thread();
  facts.revision=loan.viewport_revision();facts.occurrence=loan.window_occurrence();
  need(facts.window&&facts.dc&&facts.wgl&&facts.occurrence&&
       facts.process==GetCurrentProcessId()&&facts.thread==GetCurrentThreadId(),
       "Native UI actual renderer loan operands unavailable");
  result=attach_impl(facts.window,std::move(provider),std::move(canonical),facts);
  loan.require_current();
 });
 input->require_current();
 return result;
}
std::shared_ptr<BnwCivilopediaNativeWindowBinding>BnwCivilopediaNativeWindowBinding::attach_impl(
 std::uintptr_t window,runtime::BnwStockLuaClientExtentProvider provider,
 std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> renderer,RendererFacts facts){
 need(window&&provider.current,"Actual native UI window/provider missing");
 auto state=std::make_shared<Impl>();state->window=reinterpret_cast<HWND>(window);state->provider=std::move(provider);
 state->renderer=std::move(renderer);state->renderer_facts=facts;
 DWORD process=0;RECT rect{};
 need(IsWindow(state->window)&&GetWindowThreadProcessId(state->window,&process)==state->thread&&process==state->process&&
      GetClientRect(state->window,&rect)&&!GetPropW(state->window,Impl::property),"Native UI attachment requires unbound SAME current-process UI-thread HWND");
 // Allocate every owning facade BEFORE installation or external callbacks.
 auto owner=std::shared_ptr<BnwCivilopediaNativeWindowBinding>(new BnwCivilopediaNativeWindowBinding(state));
 auto registration=std::make_unique<Impl::Registration>(Impl::Registration{state,state->thread});
 need(SetPropW(state->window,Impl::property,registration.get())!=0,"Native UI observer property registration failed");
 if(!SetWindowSubclass(state->window,Impl::procedure,reinterpret_cast<UINT_PTR>(registration.get()),reinterpret_cast<DWORD_PTR>(registration.get()))){
  RemovePropW(state->window,Impl::property);throw std::runtime_error("Native UI destruction observer registration failed");
 }
 state->registration=registration.release();
 need(owner->current_extent().has_value(),"Actual native UI attachment extent unavailable");
 return owner;
}
std::optional<runtime::BnwStockLuaClientExtent>BnwCivilopediaNativeWindowBinding::current_extent()const {
 const auto state=impl_;
 if(GetCurrentThreadId()!=state->thread){state->poisoned.store(true);throw std::runtime_error("Native UI actual window query foreign thread");}
 if(state->querying){state->poisoned.store(true);throw std::runtime_error("Native UI actual window query reentry");}
 state->querying=true;
 try {
  RECT before{},after{};state->native_current(&before);
  std::optional<runtime::BnwStockLuaClientExtent> extent;
  {
   const auto callback=state->provider.current; // owned across self-destruction
   state->native_current(); // Includes callback target COPY side effects.
   extent=callback();
   state->native_current();
  } // Callback target destruction can retire its real source.
  state->native_current(&after);
  need(before.left==after.left&&before.top==after.top&&before.right==after.right&&before.bottom==after.bottom,
       "Native UI GetClientRect changed during extent callback");
  if(!extent){state->querying=false;return {};}
  need(!extent->surface_owner.empty()&&extent->width>0&&extent->height>0&&
       extent->width==after.right-after.left&&extent->height==after.bottom-after.top&&!IsIconic(state->window),
       "Native UI callback extent differs from actual GetClientRect");
  need(!state->renderer||extent->revision==state->renderer_facts.revision,
       "Native UI typed renderer viewport revision differs");
  need(!state->expected||same(*state->expected,*extent),"Native UI actual client epoch changed");
  if(!state->expected)state->expected=extent;
  state->querying=false;return extent;
 }catch(...){state->querying=false;state->poisoned.store(true);throw;}
}
std::uintptr_t BnwCivilopediaNativeWindowBinding::native_window()const{const auto state=impl_;state->native_current();return reinterpret_cast<std::uintptr_t>(state->window);}
std::uintptr_t BnwCivilopediaNativeWindowBinding::occurrence()const{const auto state=impl_;state->native_current();return reinterpret_cast<std::uintptr_t>(state->registration);}
void BnwCivilopediaNativeWindowBinding::require_owned_renderer_current()const{
 const auto state=impl_;
 need(GetCurrentThreadId()==state->thread,"Color physical window foreign creator thread");
 need(static_cast<bool>(state->renderer),"Color requires genuine typed physical renderer owner");
 state->native_current(); // No caller extent provider/callback is invoked.
}
void BnwCivilopediaNativeWindowBinding::retire()noexcept{const auto state=impl_;state->detach();}

void BnwCivilopediaNativeWindowBinding::require_same_gui_screen(const BnwOwnedCameraGuiScreen& screen)const {
 require_owned_renderer_current();
 need(static_cast<bool>(impl_->renderer),"Actual GUI source requires canonical typed Window");
 screen.require_same_window(*impl_->renderer);require_owned_renderer_current();
}
struct BnwCivilopediaNativeRuntimeOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::atomic<bool>retired{false},failed{false};bool running=false;
 std::shared_ptr<BnwCivilopediaNativeWindowBinding>window;
 std::shared_ptr<BnwCivilopediaRightTreeOwner>right;
 std::shared_ptr<const BnwOwnedCameraGuiScreen>gui_screen;
 std::shared_ptr<const detail::BnwNativeCompletedColorQuery> color_query;
 std::weak_ptr<const BnwNativePackageDiscoveryOwner> color_parent;
 bool civis_color_binding_attempted=false;
 std::size_t civis_font_pins=0;
 std::weak_ptr<const CivisResourceBankFontColorSnapshot> civis_color;
 std::weak_ptr<assets::CivisResourceBankOwner> civis_bank;
 std::weak_ptr<const CivisResourceBankUiSnapshot> civis_ui;
 std::weak_ptr<const SelectedBnwPreviewHost> civis_host;
 const CivisResourceBankFontColorSnapshot* civis_color_get=nullptr;
 const assets::CivisResourceBankOwner* civis_bank_get=nullptr;
 const CivisResourceBankUiSnapshot* civis_ui_get=nullptr;
 std::shared_ptr<BnwNativeSliderJournalOwner>slider;
 std::shared_ptr<const BnwCivilopediaNativeInitialGraph>graph;
 // Actual root receiver retains SAME graph-owned context component/document.
 // This typed Read argument does not set any component readiness flag.
 struct RootReadReceiver {detail::BnwRootContextFieldSnapshot fields;};
 std::shared_ptr<const RootReadReceiver>context;
 struct BoxReceiver {
  std::shared_ptr<const RootReadReceiver>read_context;
  std::shared_ptr<const content::XmlNode>source;
  BnwNativeChildCache cache;std::array<float,4>color{1,1,1,1};
  std::uint32_t flags=0;
  std::string pending_color;
  bool color_lookup_attempted=false,color_read_returned=false,numeric_color_miss=false;
  bool civis_color_receiver=false;
  bool context_field_written=false,full_size_stored=false;
  std::shared_ptr<const RootReadReceiver>context_field;
  std::uint32_t color_lookup_key=0;
 };
 std::unique_ptr<BoxReceiver>box;
 BnwCivilopediaNativeRuntimeFrame frame;
 void check() {
  if(civis_color_binding_attempted){civis_check();return;}
  need(std::this_thread::get_id()==thread,"Native UI runtime foreign thread");
  need(!retired.load()&&!failed.load(),"Native UI runtime retired/failed");
  const auto current=window->current_extent();need(current&&same(*current,frame.client),"Native UI runtime actual client retired");
  const auto&tree=right->advance(*current,0);(void)graph->current();
  need(tree.xml_sha256==frame.xml_sha256&&tree.styles_sha256==frame.styles_sha256&&context&&context->fields.current(),
       "Native UI runtime SAME source/context changed");
 }
 struct Operation {
  std::shared_ptr<Impl>state;
  explicit Operation(std::shared_ptr<Impl>s):state(std::move(s)){
   if(std::this_thread::get_id()!=state->thread){state->failed.store(true);throw std::runtime_error("Native UI runtime foreign operation");}
   if(state->running){state->failed.store(true);throw std::runtime_error("Native UI runtime reentrant operation");}
   state->running=true;
  }
  ~Operation(){state->running=false;}
 };
 void civis_pure()const {
  need(std::this_thread::get_id()==thread,"Own Color receiver foreign creator");
  need(!retired.load()&&!failed.load(),"Own Color receiver retired/failed");
 }
 void civis_check(){
  civis_pure();civis_source_pure();
  const auto current=window->current_extent();civis_pure();civis_source_pure();
  need(current&&same(*current,frame.client),"Own Color actual client changed");
  const auto& tree=right->advance(*current,0);civis_pure();civis_source_pure();
  (void)graph->current();civis_pure();civis_source_pure();
  const bool context_current=context&&context->fields.current();civis_pure();civis_source_pure();
  need(tree.xml_sha256==frame.xml_sha256&&tree.styles_sha256==frame.styles_sha256&&context_current,
       "Own Color SAME source/context changed");
  window->require_owned_renderer_current();civis_pure();civis_source_pure();
  civis_bank_window();
 }
 struct CivisFontPin {
  Impl& state;bool active=true;
  explicit CivisFontPin(Impl& actual):state(actual){
   state.civis_pure();need(state.civis_font_pins==0,"Own Color persistent snapshot pin reentry");
   ++state.civis_font_pins;
  }
  void release()noexcept{if(active){--state.civis_font_pins;active=false;}}
  ~CivisFontPin(){release();}
 };
 void civis_source_pure()const {
  civis_pure();const auto font=civis_color.lock();
  // This checker pin is additional to the explicitly inventoried operation pin.
  civis_font_pure(font,1);
 }
 void civis_font_pure(const std::shared_ptr<const CivisResourceBankFontColorSnapshot>& font,std::size_t checker_pins=0)const {
  civis_pure();
  {
   const auto actual=civis_color.lock();const auto bank=civis_bank.lock();const auto ui=civis_ui.lock();
   need(actual&&font&&actual.get()==font.get()&&font.get()==civis_color_get&&
        !actual.owner_before(font)&&!font.owner_before(actual)&&bank&&ui&&
        bank.get()==civis_bank_get&&ui.get()==civis_ui_get,
        "Own Color canonical weak tuple changed");
   const auto canonical=font->retain_owner();
   const auto canonical_bank=bank->retain_owner();const auto canonical_ui=ui->retain_owner();
   need(canonical.get()==font.get()&&!canonical.owner_before(font)&&!font.owner_before(canonical),
        "Own Color snapshot control changed");
   need(canonical_bank.get()==bank.get()&&!canonical_bank.owner_before(bank)&&!bank.owner_before(canonical_bank)&&
        canonical_ui.get()==ui.get()&&!canonical_ui.owner_before(ui)&&!ui.owner_before(canonical_ui),
        "Own Color Bank/UI canonical controls changed");
   font->require_same_bank(*bank);font->require_same_ui(*ui);font->require_current();
   const auto host=civis_host.lock();
   need(static_cast<bool>(host),"Own Color selected Host retired");
   host->require_same_civis_resources(*bank); // Actual pure Host6 lineage, no callback.
  }
  // All checker pins die before this callback-free outside-owner fence.
  civis_pure();
  need(font.use_count()>static_cast<long>(civis_font_pins+checker_pins),"Own Color outside snapshot owner retired");
  need(!civis_host.expired(),"Own Color selected Host retired after pure checker release");
 }
 void civis_bank_window(){
  civis_pure();civis_source_pure();
  const auto bank=civis_bank.lock();need(static_cast<bool>(bank),"Own Color Bank retired");
  const auto hwnd=bank->actual_hwnd();civis_pure();civis_source_pure();
  const auto extent=bank->actual_client_extent();civis_pure();civis_source_pure();
  const auto actual_hwnd=window->native_window();civis_pure();civis_source_pure();
  const auto actual_extent=window->current_extent();civis_pure();civis_source_pure();
  need(hwnd==actual_hwnd&&actual_extent&&same(extent,*actual_extent)&&same(extent,frame.client),
       "Own Color Bank HWND/client differs from actual RuntimeWindow");
  if(gui_screen){
   gui_screen->require_civis_color_source_origin_without_callbacks(civis_host,*bank);
   civis_pure();civis_source_pure();
   window->require_same_gui_screen(*gui_screen);civis_pure();civis_source_pure();
  }
 }
 void civis_final_pure()const {
  // No callbacks after operation-owned Font pin release and before commit.
  civis_pure();civis_source_pure();
  if(gui_screen){
   const auto bank=civis_bank.lock();need(static_cast<bool>(bank),"Own Color final Bank retired");
   gui_screen->require_civis_color_source_origin_without_callbacks(civis_host,*bank);
   civis_pure();civis_source_pure();
  }
  need(!civis_color.expired()&&!civis_host.expired(),"Own Color final outside producer retired");
 }
 BnwCivilopediaNativeRuntimeFrame snapshot(){
  auto out=frame;
  if(box){BnwCivilopediaNativeBoxFrame b;b.receiver_occurrence=reinterpret_cast<std::uintptr_t>(box.get());
   b.read_context_occurrence=reinterpret_cast<std::uintptr_t>(box->read_context->fields.identity().instance.get());
   b.path=out.call_stack.at(1).source_path;b.id=box->source->attribute("ID");b.cache=box->cache;b.color=box->color;b.flags=box->flags;b.pending_color_token=box->pending_color;
   b.color_lookup_attempted=box->color_lookup_attempted;b.color_read_returned=box->color_read_returned;
   b.numeric_color_miss=box->numeric_color_miss;b.color_lookup_key=box->color_lookup_key;
   b.civis_color_receiver=box->civis_color_receiver;
   b.context_field_written=box->context_field_written;
   out.box=std::move(b);}
  return out;
 }
};
BnwCivilopediaNativeRuntimeOwner::BnwCivilopediaNativeRuntimeOwner(std::shared_ptr<Impl>p):impl_(std::move(p)){}
BnwCivilopediaNativeRuntimeOwner::~BnwCivilopediaNativeRuntimeOwner(){impl_->retired.store(true);}
std::shared_ptr<BnwCivilopediaNativeRuntimeOwner>BnwCivilopediaNativeRuntimeOwner::create(
 std::shared_ptr<BnwCivilopediaNativeWindowBinding>window,const std::filesystem::path&root,const assets::ResourceMountTable&resources,
 const BnwCivilopediaScrollBarOwner&shell,std::shared_ptr<BnwCivilopediaRightTreeOwner>right){
 need(window&&right,"Retained VM native runtime inputs absent");
 auto state=std::make_shared<Impl>();state->window=std::move(window);state->right=std::move(right);
 const auto current=state->window->current_extent();need(current&&shell.get_size(*current).available&&shell.render_snapshot().control_revision==0,"Native runtime requires SAME original VM shell/client");
 const runtime::BnwStockLuaClientExtentProvider provider{[window=state->window]{return window->current_extent();}};
 state->slider=BnwNativeSliderJournalOwner::create(state->window->native_window(),provider,root,resources,shell,state->right);
 state->graph=BnwCivilopediaNativeInitialGraph::create(state->window->native_window(),provider,root,resources,state->right,state->slider)
  ->fork_create_root_context_constructor_fields()->fork_read_root_context_empty_declaration()->fork_register_root_instance_metadata_prefix();
 auto fields=state->graph->staged_root_context_fields();need(fields&&fields->current(),"Actual SAME root metadata receiver absent");
 state->context=std::make_shared<const Impl::RootReadReceiver>(Impl::RootReadReceiver{*fields});
 const auto&tree=state->right->advance(*current,0);auto&f=state->frame;f.client=*current;f.xml_sha256=tree.xml_sha256;f.styles_sha256=tree.styles_sha256;
 f.window_observer_occurrence=state->window->occurrence();f.right_tree_occurrence=reinterpret_cast<std::uintptr_t>(state->right.get());
 f.root_context_occurrence=reinterpret_cast<std::uintptr_t>(fields->identity().instance.get());f.root_source_cursor=fields->fields().next_source_cursor;f.instance_metadata_records=fields->fields().instance_metadata_journal.size();
 if(state->window->impl_->renderer){
  const auto& actual=state->window->impl_->renderer_facts;
  f.initial_renderer_window_occurrence=actual.occurrence;
  f.initial_renderer_dc=actual.dc;f.initial_renderer_wgl=actual.wgl;
  f.initial_renderer_viewport_revision=actual.revision;
 }
 f.pending_dependency="Root ordinary child factory/Box.Read source operation pending; completed production graphics/native UI remains separate";
 state->check();return std::shared_ptr<BnwCivilopediaNativeRuntimeOwner>(new BnwCivilopediaNativeRuntimeOwner(std::move(state)));
}
void BnwCivilopediaNativeRuntimeOwner::bind_gui_screen(std::shared_ptr<const BnwOwnedCameraGuiScreen> incoming){
 const auto state=impl_;Impl::Operation operation(state);
 try {
  need(static_cast<bool>(incoming),"Native GUI source absent");
  const auto canonical=incoming->retain_owner();
  need(canonical.get()==incoming.get()&&!canonical.owner_before(incoming)&&!incoming.owner_before(canonical),
       "Native GUI source canonical control differs");
  state->window->require_same_gui_screen(*canonical);state->check();
  state->gui_screen=canonical;
 }catch(...){state->failed.store(true);throw;}
}
void BnwCivilopediaNativeRuntimeOwner::bind_completed_color_query(
 std::shared_ptr<const detail::BnwNativeCompletedColorQuery> query,
 std::weak_ptr<const BnwNativePackageDiscoveryOwner> actual_parent){
 const auto state=impl_;Impl::Operation operation(state);
 try{
  need(query&&!state->color_query,"Native Color binding already attempted/absent");
  state->window->require_owned_renderer_current();
  state->check();
  const auto parent=actual_parent.lock();need(static_cast<bool>(parent),"Native Color actual adopted Parent expired");
  query->require_same_parent(*parent);
  state->window->require_owned_renderer_current();state->check();
  // Both inputs came from the private Bootstrap's checked canonical channel.
  state->color_parent=std::move(actual_parent);state->color_query=std::move(query);
 }catch(...){state->failed.store(true);throw;}
}
void BnwCivilopediaNativeRuntimeOwner::bind_civis_color_snapshot(
 const std::shared_ptr<const CivisResourceBankFontColorSnapshot>& incoming,
 const std::shared_ptr<assets::CivisResourceBankOwner>& input_bank,
 const std::shared_ptr<const CivisResourceBankUiSnapshot>& input_ui,
 const std::weak_ptr<const SelectedBnwPreviewHost>& input_host){
 impl_->civis_pure();const auto state=impl_;Impl::Operation operation(state);
 try{
  need(incoming&&input_bank&&input_ui&&!input_host.expired()&&!state->color_query&&!state->civis_color_binding_attempted&&
       (!state->box||!state->box->color_lookup_attempted),"Own Color bind absent/replayed/foreign frontier");
  state->civis_color_binding_attempted=true; // Reserve before every callbackful check.
  auto font=incoming->retain_owner();
  Impl::CivisFontPin font_pin(*state);
  need(font.get()==incoming.get()&&!font.owner_before(incoming)&&!incoming.owner_before(font),
       "Own Color bind snapshot control differs");
  {
   const auto bank=input_bank->retain_owner();const auto ui=input_ui->retain_owner();
   need(bank.get()==input_bank.get()&&!bank.owner_before(input_bank)&&!input_bank.owner_before(bank)&&
        ui.get()==input_ui.get()&&!ui.owner_before(input_ui)&&!input_ui.owner_before(ui),
        "Own Color bind Bank/UI control differs");
   font->require_same_bank(*bank);font->require_same_ui(*ui);ui->require_same_bank(*bank);
   state->civis_color=font;state->civis_bank=bank;state->civis_ui=ui;
   state->civis_host=input_host;
   state->civis_color_get=font.get();state->civis_bank_get=bank.get();state->civis_ui_get=ui.get();
  }
  state->civis_font_pure(font);state->civis_check();state->civis_font_pure(font);
  font->require_boundary();state->civis_pure();state->civis_font_pure(font);
  state->civis_check();state->civis_font_pure(font);
  font.reset();font_pin.release();state->civis_pure();state->civis_final_pure();
 }catch(...){state->failed.store(true);throw;}
}
BnwCivilopediaNativeRuntimeFrame BnwCivilopediaNativeRuntimeOwner::advance_initialization(){
 const auto state=impl_;Impl::Operation operation(state);
 try {
   if(state->civis_color_binding_attempted){
    state->civis_pure();need(!state->color_query,"Own Color conflicts with Original completed receiver");
    state->civis_check();
   }else state->check();
  if(!state->box){
   const auto document=state->context->fields.identity().document;
   const auto ordinal=state->frame.root_source_cursor;
   need(ordinal==33&&ordinal<document->children.size(),"Actual root ordinary source cursor differs");
   const auto&node=document->children[ordinal];
   const std::vector<std::pair<std::string,std::string>> expected{{"Anchor","C,C"},{"Size","Full,Full"},{"Color","0,0,0,160"},{"ConsumeMouse","1"},{"ID","BackDrop"}};
   need(node.name=="Box"&&node.attributes==expected&&node.children.empty(),"First actual ordinary Box factory source differs");
   const std::string path="Context/33:Box#BackDrop";
   // Root source call stays active while ordinary virtual Read has not returned.
   state->frame.call_stack={{BnwCivilopediaNativeOperation::root_ordinary_child,0x7aa044,ordinal,"Context"},
    {BnwCivilopediaNativeOperation::ordinary_box_factory,0x529520,ordinal,path}};
   auto receiver=std::make_unique<Impl::BoxReceiver>();receiver->read_context=state->context;
   receiver->source=std::shared_ptr<const content::XmlNode>(document,&node);
   // 47D290 ->58E740 ->4F9470: source owned Box/base constructor writes.
   // Parent/context FIELDS stay null until source AddChild after Read returns.
   state->box=std::move(receiver);++state->frame.ordinary_factory_executions;
   state->frame.call_stack.push_back({BnwCivilopediaNativeOperation::box_read,0x6a3690,ordinal,path});
   ++state->frame.box_read_entries;
   // Actual first selected source has no Style. The wrapper's absent-style
   // branch is evaluated against the retained node, never a supplied flag.
   need(node.attribute("Style").empty(),"First Box style Read requires actual style owner");
   state->box->pending_color=node.attribute("Color");
   need(!state->box->pending_color.empty(),"First Box Color source absent");
   state->frame.call_stack.push_back({BnwCivilopediaNativeOperation::color_read,0x580780,ordinal,path});
   state->frame.call_stack.push_back({BnwCivilopediaNativeOperation::named_color_lookup,0x55e2b0,ordinal,path});
   state->frame.pending_dependency="Box#BackDrop Read suspended before actual named/dynamic Color provider lookup55E2B0/5307B0; numeric fallback, base Read/Full/dynamic registration, AddChild and native initialization unexecuted; accepted production WindowLease binding pending";
  }
  if(state->color_query&&state->box&&!state->box->color_read_returned){
   auto& box=*state->box;
   need(!box.color_lookup_attempted&&box.pending_color=="0,0,0,160"&&box.source->attribute("Color")==box.pending_color,
        "Native Color source/once frontier changed");
   state->window->require_owned_renderer_current();state->check();
   const auto parent=state->color_parent.lock();need(static_cast<bool>(parent),"Native Color same adopted Parent expired");
   const auto query=state->color_query; // Canonical strong leaf BEFORE any source callback.
   const auto before_receiver=state->box.get();const auto before_context=box.read_context.get();
   const auto before_source=box.source.get();const auto before_cursor=state->frame.root_source_cursor;
   const auto source=box.pending_color;
   native_color_load_detail::Tokens tokens{native_color_load_detail::trim(source)};
   const auto key=native_color_load_detail::hash_query(tokens.take());
   // All fallible output allocation is before the first callback attempt.
   auto next_stack=state->frame.call_stack;need(next_stack.size()==5,"Native Color call stack differs");
   next_stack.resize(3);
   next_stack.push_back({BnwCivilopediaNativeOperation::base_box_read,0x438170,before_cursor,next_stack.at(1).source_path});
   std::string next_dependency="Box#BackDrop Color.Read returned; base Read0x438170 Size Full/Full requires actual GUI full-size/dynamic registration. Box Read/AddChild/Font/Slider and stock activation remain unexecuted";
   box.color_lookup_key=key;box.color_lookup_attempted=true; // BEFORE callback; no replay.
   query->require_same_parent(*parent);
   state->window->require_owned_renderer_current();state->check();
   const auto value=query->query(key); // Actual55E2B0/5307B0 completed ordinary lookup.
   state->window->require_owned_renderer_current();state->check();
   query->require_same_parent(*parent);
   state->window->require_owned_renderer_current();state->check();
   need(state->box.get()==before_receiver&&box.read_context.get()==before_context&&
        box.source.get()==before_source&&state->frame.root_source_cursor==before_cursor&&
        box.pending_color==source&&!box.color_read_returned&&state->color_query.get()==query.get()&&
        !state->color_query.owner_before(query)&&!query.owner_before(state->color_query),
        "Native Color receiver/source/canonical owner changed during callback");
   auto bits=value.value_or(detail::BnwNativeCompletedColorQuery::ColorBits{});
   if(!value)native_color_load_detail::numeric(source,bits); // ONLY true completed MISS.
   for(std::size_t component=0;component<4;++component)box.color[component]=std::bit_cast<float>(bits[component]);
   box.numeric_color_miss=!value;box.color_read_returned=true;
   state->frame.call_stack=std::move(next_stack);state->frame.pending_dependency=std::move(next_dependency);
  }
   if(state->civis_color_binding_attempted&&state->box&&!state->box->color_read_returned){
    state->civis_pure();
    need(!state->color_query,"Own Color cannot coexist with Original completed receiver");
    auto font=state->civis_color.lock();state->civis_font_pure(font);
    Impl::CivisFontPin font_pin(*state);
    auto& box=*state->box;
    need(!box.color_lookup_attempted&&box.pending_color=="0,0,0,160"&&box.source->attribute("Color")==box.pending_color&&
         state->gui_screen,"Own Color requires exact first Box source and actual GUI");
    const auto before_receiver=state->box.get();const auto before_context=box.read_context.get();
    const auto before_source=box.source.get();const auto before_cursor=state->frame.root_source_cursor;
    const auto source=box.pending_color;
    native_color_load_detail::Tokens tokens{native_color_load_detail::trim(source)};
    const std::string palette_name(tokens.take());
    const auto key=native_color_load_detail::hash_query(palette_name); // Observational data only.
    auto entered_stack=state->frame.call_stack;need(entered_stack.size()==5,"Own Color call stack frontier differs");
    entered_stack[3]={BnwCivilopediaNativeOperation::civis_color_read,0,before_cursor,entered_stack[1].source_path};
    entered_stack[4]={BnwCivilopediaNativeOperation::civis_palette_color_lookup,0,before_cursor,entered_stack[1].source_path};
    auto next_stack=entered_stack;next_stack.resize(3);
    next_stack.push_back({BnwCivilopediaNativeOperation::base_box_read,0x438170,before_cursor,next_stack[1].source_path});
    std::string next_dependency="Own Civis CPU palette/value Color returned; no Original55E2B0/5307B0 completed lookup. Base Full/Full Read, AddChild and stock activation remain separate actual source gates";
    box.color_lookup_key=key;box.color_lookup_attempted=true;box.civis_color_receiver=true;
    state->frame.call_stack=std::move(entered_stack); // Reserve actual own occurrence BEFORE callbacks.
    const auto current=[&]{
     state->civis_font_pure(font);state->civis_check();state->civis_font_pure(font);
     state->gui_screen->require_current();state->civis_pure();state->civis_font_pure(font);
     state->window->require_same_gui_screen(*state->gui_screen);state->civis_pure();state->civis_font_pure(font);
     need(state->box.get()==before_receiver&&state->box->read_context.get()==before_context&&
          state->box->source.get()==before_source&&state->frame.root_source_cursor==before_cursor&&
          state->box->pending_color==source&&!state->box->color_read_returned,
          "Own Color receiver/source changed during callback");
    };
    current();font->require_boundary();state->civis_pure();current();
    const auto packed=font->strict_named_color(palette_name); // Errors propagate; null only valid palette name MISS.
    state->civis_pure();current();
    need((_mm_getcsr()&0xe040u)==0,"Own Color normalization requires supported current SSE context");
    native_color_load_detail::ColorBits bits{};
    if(packed){
     // Color resolver uses packed RRGGBBAA. Byte extraction is exact; each
     // normalized float uses the existing owned value kernel, no whole DWORD float cast.
     for(std::size_t i=0;i<4;++i)
      bits[i]=native_color_load_detail::scaled(static_cast<std::int32_t>((*packed>>(24-8*i))&255u));
    }else native_color_load_detail::numeric(source,bits); // ONLY genuine palette MISS.
    font->require_boundary();state->civis_pure();current();
    font.reset();font_pin.release();state->civis_pure();state->civis_final_pure();
    for(std::size_t i=0;i<4;++i)state->box->color[i]=std::bit_cast<float>(bits[i]);
    state->box->numeric_color_miss=!packed;state->box->color_read_returned=true;
    state->frame.call_stack=std::move(next_stack);state->frame.pending_dependency=std::move(next_dependency);
   }
   if(state->box&&state->box->color_read_returned&&!state->box->full_size_stored&&state->gui_screen){
   auto& box=*state->box;const auto source=box.source;const auto context=box.read_context;
   const auto screen=state->gui_screen;
   need(source->attribute("Size")=="Full,Full"&&source->attribute("Offset").empty()&&
        !box.context_field_written,"Selected base Box Full/Full source frontier differs");
   state->window->require_same_gui_screen(*screen);state->check();
   // Actual selected 4381AC context+3C store; parent+38 remains untouched.
   box.context_field=context;box.context_field_written=true;
   screen->with_actual_screen_loan([&](const auto& loan){
    const auto numeric=[] {need((_mm_getcsr()&0xe040u)==0,"Selected Full/Full unsupported current SSE context");};
    numeric();
    loan.require_current();state->check();
    numeric();
    const auto width=loan.width();state->check();loan.require_current();
    numeric();
    // 76DC50 selected first Full: signed32 -> float, destination cache width.
    box.cache.width=static_cast<float>(width);
    numeric();
    const auto height=loan.height();state->check();loan.require_current();
    numeric();
    box.cache.height=static_cast<float>(height);
    box.flags|=0x300000u;
    loan.require_current();state->check();
    numeric();
   });
   state->window->require_same_gui_screen(*screen);state->check();
   need(box.source.get()==source.get()&&box.read_context.get()==context.get()&&
        state->gui_screen.get()==screen.get(),"Native Full receiver/source changed");
   box.full_size_stored=true;
   state->frame.pending_dependency="Actual Civis compositor GUI loan supplied selected Full/Full signed32-to-float stores and flags0x300000; native86AC20/8A7040 registry insertion/dispatch/unregister and Box virtual+98 remain pending. BaseRead/AddChild/Font/Slider and stock loading remain unexecuted";
  }
  state->check();return state->snapshot();
 }catch(...){state->failed.store(true);throw;}
}
BnwCivilopediaNativeRuntimeFrame BnwCivilopediaNativeRuntimeOwner::current()const{
 const auto state=impl_;Impl::Operation operation(state);
 try{state->check();return state->snapshot();}catch(...){state->failed.store(true);throw;}
}
void BnwCivilopediaNativeRuntimeOwner::retire()noexcept{const auto state=impl_;state->retired.store(true);}
}
