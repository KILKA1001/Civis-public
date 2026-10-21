#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include <iostream>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
template<class F>void refused(F&&f,const char*s){bool bad=false;try{f();}catch(const std::exception&){bad=true;}need(bad,s);}
template<class F>void refused_exact(F&&f,std::string_view expected,const char*s){
 bool bad=false;try{f();}catch(const std::exception&e){bad=std::string_view(e.what())==expected;}need(bad,s);
}
struct Window {
 HWND hwnd=CreateWindowExW(0,L"STATIC",L"Own WorldView font issuer CPU gate",WS_OVERLAPPEDWINDOW,0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 std::uint64_t revision=1;std::function<void()>callback;
 ~Window(){if(hwnd)DestroyWindow(hwnd);}
 runtime::BnwStockLuaClientExtentProvider provider(){
  auto native=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(hwnd),"worldview-issuer-actual-cpu-window",[this]{return revision;});
  return {[this,native]()->std::optional<runtime::BnwStockLuaClientExtent>{const auto copy=callback;if(copy)copy();return native.current();}};
 }
};
assets::ResourceMountTable sources(const std::filesystem::path&root){
 const assets::AssetEnvironment environment(root);auto source=environment.loose_source();
 return assets::ResourceMountTable(assets::resource_lookup(source),assets::resource_lookup_all(source),assets::resource_keys(source));
}
}
int main(int argc,char**argv){try{
 need(argc==4,"Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS");const std::filesystem::path save=argv[1],game=argv[2],root=argv[3];
 Window window;need(window.hwnd!=nullptr,"Actual retained CPU window absent");
 auto lease=ui::BnwNativeEarlyInputLease::acquire(save,game,root,reinterpret_cast<std::uintptr_t>(window.hwnd),window.provider());
 auto package=ui::BnwNativePackageDiscoveryOwner::create(lease);package->register_native_dx9_primary_prefix();
 auto settings=ui::BnwNativeResourceBootSettingsOwner::create(package);package->register_native_dx9_texture_quality_branch(std::move(settings));package->register_native_common_high_quality_prefix();
 auto boot=ui::bootstrap_selected_bnw_preview_from_early_package(package,0);need(static_cast<bool>(boot.preview),"Actual genuine adopted host unavailable");
 // The real preview moves unique_ptr into its genuine shared parent. This
 // named handoff preserves exact private adoption; no alias or observer move.
 ui::SelectedBnwReadOnlyPreview preview(std::move(boot.preview));bool reached=false;
 preview.with_owned_host_camera_input([&](std::shared_ptr<const ui::SelectedBnwPreviewHost>host,const ui::LiveBnwPreviewInput*input){
  reached=true;need(host&&input&&&host->session()==&input->session,"Owned handoff substituted session");host->require_same_early_package(*package);
  const auto c=lease->client_extent();const ui::ClientViewport v{static_cast<int>(c.width),static_cast<int>(c.height),static_cast<int>(c.width),static_cast<int>(c.height),96};
  auto fresh_scope=[&]{return ui::BnwWorldViewFontSourceScope::create(sources(root),"ru_RU");};
  // Empty selection / no source quote must NOT become a current native title.
  ui::BnwClientFrameOwner owner(host->session(),host->identity().payload_sha256,0,input->frame_sources);auto frame=owner.capture(v,{});need(static_cast<bool>(frame),"Actual current CPU frame absent");
  refused([&]{ui::BnwWorldViewTitleFontIssuer::create_from_adopted_host({},owner,frame,fresh_scope());},"Missing genuine shared host admitted");
  refused([&]{ui::BnwWorldViewTitleFontIssuer::create_from_adopted_host(host,owner,frame,{});},"Missing actual source scope admitted");
  refused([&]{ui::BnwWorldViewTitleFontIssuer::create_from_adopted_host(host,owner,frame,fresh_scope());},"Unbound locale/absent selected title admitted");
  auto foreign=ui::bootstrap_selected_bnw_preview(save,game,0);need(static_cast<bool>(foreign.preview),"Actual separate legacy host unavailable");
  ui::BnwClientFrameOwner other(foreign.preview.host->session(),foreign.preview.host->identity().payload_sha256,0,foreign.preview.host->preview_input().frame_sources);auto other_frame=other.capture(v,{});
  refused([&]{ui::BnwWorldViewTitleFontIssuer::create_from_adopted_host(host,other,other_frame,fresh_scope());},"Foreign actual session accepted");
  owner.notify_world_changed();refused([&]{ui::BnwWorldViewTitleFontIssuer::create_from_adopted_host(host,owner,frame,fresh_scope());},"Retired publication admitted");
  // Genuine SAME-session owners: neither a different alive selected player
  // nor a distinct nonempty identity may enter the selected host issuer.
  // Existing visibility extraction already rejects a foreign active-player
  // capture, so we do NOT fabricate one. Its genuine FrameOwner is refused by
  // the new player guard before current_frame/source/lease callbacks.
  std::optional<std::int64_t> other_alive;
  for(const auto&[id,p]:host->session().state().setup.players)
   if(id!=host->active_player_id()&&p.alive&&p.team>=0){other_alive=id;break;}
  need(other_alive.has_value(),"Actual saved-session second alive player absent");
  auto identity_scope=fresh_scope();ui::LiveSessionUiSources identity_sources;
  identity_sources.augment_snapshot=[identity_scope](const runtime::LiveSession&,ui::UiFrameSnapshot f){
   return identity_scope->augment_snapshot(std::move(f));};
  ui::BnwClientFrameOwner selected_owner(host->session(),host->identity().payload_sha256,
      host->active_player_id(),identity_sources);
  const auto selected_frame=selected_owner.capture(v,{});
  ui::BnwClientFrameOwner foreign_player(host->session(),host->identity().payload_sha256,*other_alive);
  int client_callbacks=0;window.callback=[&]{++client_callbacks;};
  refused_exact([&]{ui::BnwWorldViewTitleFontIssuer::create_from_adopted_host(
       host,foreign_player,selected_frame,identity_scope);},
       "WorldView font frame owner selected host player differs","SAME-session foreign selected player reached issuer");
  window.callback={};need(client_callbacks==0,"Foreign selected player reached lease callback");
  identity_scope->require_current(); // Refusal never retires a valid source alone.
  const auto wrong_identity=host->identity().payload_sha256+":distinct-nonempty-identity";
  ui::BnwClientFrameOwner wrong_session_identity(host->session(),wrong_identity,
      host->active_player_id(),identity_sources);
  const auto wrong_frame=wrong_session_identity.capture(v,{});
  need(wrong_frame&&wrong_frame->stamp.identity.session==wrong_identity&&
       wrong_frame->stamp.identity.player==host->active_player_id(),"Actual wrong-identity publication absent");
  client_callbacks=0;window.callback=[&]{++client_callbacks;};
  refused_exact([&]{ui::BnwWorldViewTitleFontIssuer::create_from_adopted_host(
       host,wrong_session_identity,wrong_frame,identity_scope);},
       "WorldView font frame owner selected host payload identity differs","SAME-session wrong payload identity reached issuer");
  window.callback={};need(client_callbacks==0,"Wrong payload identity reached lease callback");
  identity_scope->require_current();
  // Scope issuance guard must precede the FIRST real lease callback; nested
  // admission can be caught, but neither issuer may then be published.
  auto scope=fresh_scope();auto bound_sources=input->frame_sources;
  bound_sources.augment_snapshot=[scope](const runtime::LiveSession&,ui::UiFrameSnapshot f){return scope->augment_snapshot(std::move(f));};
  ui::BnwClientFrameOwner bound_owner(host->session(),host->identity().payload_sha256,0,bound_sources);frame=bound_owner.capture(v,{});bool nested=false;
  window.callback=[&]{try{ui::BnwWorldViewTitleFontIssuer::create_from_adopted_host(host,bound_owner,frame,scope);}catch(const std::exception&){nested=true;}};
  refused([&]{ui::BnwWorldViewTitleFontIssuer::create_from_adopted_host(host,bound_owner,frame,scope);},"Caught factory reentry authorized outer issuer");window.callback={};need(nested,"Real client callback never reached nested admission");refused([&]{scope->require_current();},"Factory poison did not remain on SAME scope");
  scope=fresh_scope();bound_sources.augment_snapshot=[scope](const runtime::LiveSession&,ui::UiFrameSnapshot f){return scope->augment_snapshot(std::move(f));};
  ui::BnwClientFrameOwner closing_owner(host->session(),host->identity().payload_sha256,0,bound_sources);frame=closing_owner.capture(v,{});
  const auto old=window.revision;window.callback=[&]{++window.revision;scope->retire();};
  refused([&]{ui::BnwWorldViewTitleFontIssuer::create_from_adopted_host(host,closing_owner,frame,scope);},"Initial client retirement admitted issuer");window.callback={};window.revision=old;
  // This test's scalar provider is diagnostic, unlike the actual prepared
  // viewport's irrevocable retirement producer. The issuer scope itself MUST
  // remain poisoned even when that diagnostic scalar is restored.
  refused([&]{scope->require_current();},"Failed issuer scope resurrected after diagnostic scalar reset");
 });need(reached,"Real owned-parent handoff skipped");
 std::cout<<"PASS actual CPU negative/adoption/handoff/client/publication gates; positive current title descriptor, copied seal, GDI raster and camera are pending real title-source binding. No GPU/initialized Label claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
