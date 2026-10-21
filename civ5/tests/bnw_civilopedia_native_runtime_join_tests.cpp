#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/ui/bnw_civilopedia_native_runtime_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
using namespace civ5;
using namespace civ5::runtime;
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F&&f){bool refused=false;try{f();}catch(const std::exception&){refused=true;}need(refused,"Invalid native production join accepted");}
struct Window {
 HWND handle=CreateWindowExW(0,L"STATIC",L"Civis native runtime join CPU",WS_OVERLAPPEDWINDOW,0,0,840,620,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 std::uint64_t revision=17;int routed=0;static constexpr LONG_PTR sentinel=0x3e18;
 static LRESULT CALLBACK route(HWND hwnd,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
  auto* self=reinterpret_cast<Window*>(data);
  if(message==WM_APP+10){++self->routed;return 3137;}
  if(message==WM_NCDESTROY)RemoveWindowSubclass(hwnd,route,id);
  return DefSubclassProc(hwnd,message,wp,lp);
 }
 Window(){need(handle&&!IsWindowVisible(handle),"Own hidden native window unavailable");SetWindowLongPtrW(handle,GWLP_USERDATA,sentinel);need(SetWindowSubclass(handle,route,1,reinterpret_cast<DWORD_PTR>(this))!=0,"Actual peer router fixture attach failed");}
 ~Window(){destroy();}
 void destroy(){if(handle&&IsWindow(handle))need(DestroyWindow(handle)!=0,"Actual own DestroyWindow failed");handle=nullptr;}
 void routing(){need(IsWindow(handle)&&GetWindowLongPtrW(handle,GWLP_USERDATA)==sentinel&&SendMessageW(handle,WM_APP+10,0,0)==3137&&routed>0,"Native observer replaced/broke actual router/userdata");}
 BnwStockLuaClientExtentProvider provider(){return bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(handle),"owned-native-ui-client",[this]{return revision;});}
};
struct Fixture {
 std::filesystem::path root,assets;content::Catalog catalog;SavedContentProfile profile;
 std::shared_ptr<ui::SelectedBnwPreviewHost>host;
 std::shared_ptr<BnwSelectedStockLuaSourceOwner>source(){
  auto out=std::make_shared<BnwSelectedStockLuaSourceOwner>(root,catalog,profile,host->identity());
  const auto p=out->start_stock_bootstrap(host,assets);
  need(p.first_prefix_executed&&p.prefix_executions==1&&p.pending_api=="UIManager.GetScreenSizeVal","Actual retained stock VM prefix absent");return out;
 }
};
ui::BnwCivilopediaNativeRuntimeFrame enter(Fixture&f,const std::shared_ptr<BnwSelectedStockLuaSourceOwner>&s,Window&w){
 need(!s->stock_native_runtime_frame(),"Native receiver appeared before real factory execution");
 need(s->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(w.handle),w.provider()),"Actual native observer attach failed");
 w.routing();need(!s->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(w.handle),w.provider()),"Actual native attachment replayed");
 need(!s->attach_stock_client_extent(w.provider()),"Native client binding replaced by scalar extent");
 need(s->advance_stock_bootstrap().pending_api=="Controls.ScrollBar.GetSize","Actual stock screen getter failed");
 need(s->advance_stock_bootstrap().pending_api=="Controls.ScrollPanel.GetSize","Actual stock bar getter failed");
 need(s->advance_stock_bootstrap().pending_api=="Controls.ScrollBar.SetSize","Actual stock panel getter failed");
 need(!s->stock_native_runtime_frame(),"Native factory skipped its exact source execution point");
 const auto p=s->advance_stock_bootstrap();const auto frame=s->stock_native_runtime_frame();
 need(frame&&frame->box&&p.pending_api=="Controls.ScrollBar.SetSize"&&p.prefix_executions==1&&p.native_screen_queries==1&&
      p.native_scrollbar_queries==1&&p.native_panel_queries==1&&!p.native_scrollbar_resizes&&!p.loading_complete&&!p.gameplay_callbacks_ready,
      "Native source operation replayed prefix or bypassed first Slider setter");
 const auto&b=*frame->box;
 need(frame->ordinary_factory_executions==1&&frame->box_read_entries==1&&frame->root_source_cursor==33&&frame->instance_metadata_records==33&&
      frame->call_stack.size()==5&&frame->call_stack.back().operation==ui::BnwCivilopediaNativeOperation::named_color_lookup&&
      frame->window_observer_occurrence&&frame->right_tree_occurrence&&frame->root_context_occurrence&&
      b.receiver_occurrence&&b.read_context_occurrence==frame->root_context_occurrence&&b.id=="BackDrop"&&b.pending_color_token=="0,0,0,160"&&
      b.cache.width==0&&b.cache.height==0&&b.cache.edge_x==0&&b.cache.edge_y==0&&b.cache.bounds==std::array<float,4>{}&&
      b.color==std::array<float,4>{1,1,1,1}&&b.flags==0&&b.native_kind==14&&!b.parent_field_written&&!b.context_field_written&&
      !b.read_returned&&!b.source_append_completed&&!frame->native_slider_setter_complete&&!frame->production_window_lease_bound,
      "Actual Box constructor/Read/context call stack replaced by authored Full/Color/append success");
 need(frame->xml_sha256==p.xml_sha256&&p.initial_native_dependency.find("Box#BackDrop")!=std::string::npos&&
      frame->pending_dependency.find("named/dynamic Color")!=std::string::npos,"Actual native source request lost its same-VM provenance");
 need(!f.host->session().readiness().can_continue(),"Partial native runtime opened gameplay");return *frame;
}
void retained(Fixture&f){
 Window window;auto s=f.source();const auto first=enter(f,s,window);
 for(int i=0;i<3;++i){const auto p=s->advance_stock_bootstrap();const auto frame=s->stock_native_runtime_frame();
  need(frame&&frame->box&&frame->box->receiver_occurrence==first.box->receiver_occurrence&&frame->root_context_occurrence==first.root_context_occurrence&&
       frame->right_tree_occurrence==first.right_tree_occurrence&&frame->window_observer_occurrence==first.window_observer_occurrence&&
       frame->ordinary_factory_executions==1&&frame->box_read_entries==1&&frame->root_source_cursor==33&&frame->call_stack.size()==5&&
       !p.native_scrollbar_resizes&&p.prefix_executions==1,"Suspended actual source receiver/call stack was rebuilt or completed from flags");}
 window.routing();s->retire_stock_bootstrap();rejects([&]{s->advance_stock_bootstrap();});
 // Normal retirement removes this binding even while its facade/VM is retained.
 auto next=f.source();enter(f,next,window);next.reset();window.routing();
 auto after_destruction=f.source();enter(f,after_destruction,window);
 window.destroy();rejects([&]{after_destruction->stock_native_runtime_frame();});
 Window replacement;rejects([&]{after_destruction->advance_stock_bootstrap();});
 rejects([&]{after_destruction->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(replacement.handle),replacement.provider());});
}
void callbacks(Fixture&f){
 for(int mode=0;mode<7;++mode){
  Window window;auto s=f.source();auto native=window.provider();std::size_t calls=0;
  auto provider=BnwStockLuaClientExtentProvider{[&,native]{
   ++calls;auto extent=native.current();need(extent.has_value(),"Actual native callback observation unavailable");
   if(mode==0)++extent->width;
   else if(mode==1)need(SetWindowPos(window.handle,nullptr,0,0,900,650,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE)!=0,"Actual native resize failed");
   else if(mode==2)window.destroy();
   else if(mode==3)rejects([&]{s->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(window.handle),window.provider());});
   else if(mode==4)rejects([&]{s->stock_native_runtime_frame();});
   else if(mode==5)throw std::runtime_error("Late throw after actual native GetClientRect");
   else s.reset();
   return extent;
  }};
  auto* borrowed=s.get();rejects([&]{borrowed->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(window.handle),std::move(provider));});
  need(calls==1,"Native attachment callback repeated after observed effect");
  if(s){rejects([&]{s->advance_stock_bootstrap();});rejects([&]{s->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(window.handle),window.provider());});}
  need(calls==1,"Poisoned native provider replayed");
 }
 {
  Window window;auto s=f.source();std::size_t calls=0;auto native=window.provider();bool refused=false;
  std::thread worker([&]{try{s->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(window.handle),{[&]{++calls;return native.current();}});}catch(const std::exception&){refused=true;}});worker.join();
  need(refused&&calls==0,"Foreign native attachment touched actual source/window callback");rejects([&]{s->advance_stock_bootstrap();});
 }
 {
  Window window;auto s=f.source();bool drop=false;auto native=window.provider();
  need(s->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(window.handle),{[&,native]{const auto extent=native.current();if(drop)s.reset();return extent;}}),"Actual self-retiring query client attach failed");
  for(int i=0;i<4;++i)s->advance_stock_bootstrap();
  drop=true;auto* borrowed=s.get();rejects([&]{borrowed->stock_native_runtime_frame();});need(!s,"Late actual source owner release not observed");window.routing();
 }
}
void foreign_retirement(Fixture&f){
 {
  Window window;auto s=f.source();auto native=window.provider();bool revoke=false;std::size_t calls=0;
  need(s->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(window.handle),{[&,native]{
   ++calls;const auto extent=native.current();need(extent.has_value(),"Actual native observation absent before concurrent revocation");
   if(revoke){
    // Retain the facade while its real UI-thread callback is active. The worker
    // may revoke the stable atomic control, never read published VM members.
    const auto retained=s;
    std::thread worker([retained]{retained->retire_stock_bootstrap();});worker.join();
   }
   return extent;
  }}),"Actual client absent before concurrent native retirement");
  for(int i=0;i<4;++i)s->advance_stock_bootstrap();
  need(s->stock_native_runtime_frame()->box.has_value(),"Actual native runtime not created before concurrent retirement");
  revoke=true;const auto before=calls;rejects([&]{s->stock_native_runtime_frame();});
  need(calls==before+1,"Concurrent retirement replayed current native callback");
  rejects([&]{s->advance_stock_bootstrap();});rejects([&]{s->stock_native_runtime_frame();});
  need(calls==before+1,"Retired source called actual native provider again");window.routing();
  // Foreign revocation leaves a dormant observer until genuine UI-thread
  // cleanup. Its unique property must prevent a new binding on that HWND.
  auto blocked=f.source();rejects([&]{blocked->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(window.handle),window.provider());});
  s->retire_stock_bootstrap(); // UI-thread teardown remains available after revoke.
  auto next=f.source();enter(f,next,window);window.routing();
 }
 {
  Window window;auto s=f.source();enter(f,s,window);
  // No public operation is outstanding. Last facade/VM release occurs on a
  // worker; the registered strong state survives until actual WM_NCDESTROY.
  std::thread worker([owned=std::move(s)]()mutable{owned.reset();});worker.join();
  need(!s,"Foreign last-facade release retained a hidden source facade");window.routing();
  auto blocked=f.source();rejects([&]{blocked->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(window.handle),window.provider());});
  window.destroy(); // Same UI thread dispatches and frees the exact registration.
  Window replacement;auto next=f.source();enter(f,next,replacement);replacement.routing();
 }
}
}
int main(int argc,char**argv){
 try{
  need(argc==4,"Requires actual BNW save, source/rules root and canonical assets");
  const auto before=identify_file(argv[1]);Fixture f;f.root=argv[2];f.assets=argv[3];
  auto selected=ui::bootstrap_selected_bnw_preview(argv[1],f.root,0,true);
  need(static_cast<bool>(selected.preview),"Actual immutable selected host unavailable");
  f.host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(selected.preview.host));f.profile=selected.profile;
  f.catalog=content::discover_packages({f.root/"Assets/DLC"});need(f.catalog.diagnostics.empty(),"Actual selected package discovery failed");
  const auto rng_map=f.host->session().state().random.map.state(),rng_game=f.host->session().state().random.gameplay.state();
  retained(f);callbacks(f);foreign_retirement(f);
  {
   Window window;auto s=f.source();bool retire_rules=false;std::size_t attempts=0;auto native=window.provider();
   const auto identity=f.host->identity();const auto current=f.host->database().immutable_preview_fingerprint_currentness();
   need(current&&current(),"Actual immutable rules currentness missing before native test");
   need(s->attach_stock_native_client(reinterpret_cast<std::uintptr_t>(window.handle),{[&,native]{
    const auto extent=native.current();need(extent.has_value(),"Actual native extent unavailable before rules retirement");
    if(retire_rules){++attempts;auto rejected=f.host->database().script_query("UPDATE Defines SET Value=Value WHERE Name='MAX_HIT_POINTS'",{});need(!rejected&&!current(),"Actual rules UPDATE did not retire SAME scope");}
    return extent;
   }}),"Actual native late-rules client attach failed");
   for(int i=0;i<4;++i)s->advance_stock_bootstrap();
   need(s->stock_native_runtime_frame()->box.has_value(),"Genuine native Box receiver absent before rules retirement");
   retire_rules=true;rejects([&]{s->stock_native_runtime_frame();});
   need(attempts==1&&f.host->identity().rules_sha256==identity.rules_sha256,"Native late-rules check used only cached identity");
   rejects([&]{s->advance_stock_bootstrap();});need(attempts==1,"Retired native rules callback replayed");
  }
  need(f.host->session().state().random.map.state()==rng_map&&f.host->session().state().random.gameplay.state()==rng_game&&
       !f.host->session().readiness().can_continue()&&identify_file(argv[1]).sha256==before.sha256,"Native source join changed actual world/RNG/source save/readiness");
  std::cout<<"PASS actual SAME stock/native runtime source receiver and borrowed HWND lifecycle; Color supplier/WindowLease/Slider setter/epoch remain unavailable\n";
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
}
