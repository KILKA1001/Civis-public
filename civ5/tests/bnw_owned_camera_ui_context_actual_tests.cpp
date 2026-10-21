#include "civ5/ui/bnw_plot_yield_refresh_owned_ui_context.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_live_preview_launch.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "../src/ui/bnw_native_currentness_timing_diagnostic.hpp"
#include "../src/ui/bnw_native_resource_stamp_diagnostic.hpp"
#include <iostream>
#include <algorithm>
#include <climits>
#include "civ5/runtime/bnw_map_plot_checkpoint.hpp"
#include "civ5/runtime/bnw_live_rng_export.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <chrono>
#include <cerrno>
#include <filesystem>
#include <thread>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
using namespace civ5;
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
// Private test telemetry only: these values cannot grant owner/currentness admission.
const char* diagnostic_last_stage="before-main";
const auto diagnostic_native_epoch=std::chrono::steady_clock::now();
void report_native_provider_progress(std::uint64_t native_reads)noexcept{
 if(native_reads!=1&&native_reads%256!=0)return;
 const auto saved_error=GetLastError();const int saved_errno=errno;
 try{
  static auto previous=diagnostic_native_epoch;
  const auto now=std::chrono::steady_clock::now();
  const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(now-diagnostic_native_epoch).count();
  const auto interval=std::chrono::duration_cast<std::chrono::milliseconds>(now-previous).count();previous=now;
  std::cerr<<"Actual owned context native-provider progress: stage="<<diagnostic_last_stage
      <<" native_reads="<<native_reads<<" elapsed_ms="<<elapsed<<" since_report_ms="<<interval<<'\n';std::cerr.flush();
  ui::native_stamp_diagnostic::report();
 }catch(...){}
 SetLastError(saved_error);errno=saved_errno;
}
// Diagnostic boundaries only; no owner/provider calls or admission authority.
void report_actual_context_progress(const char* boundary,int lane=-1)noexcept{
 diagnostic_last_stage=boundary;
 try{std::cerr<<"Actual owned context progress: "<<boundary<<" lane="<<lane<<'\n';std::cerr.flush();}
 catch(...){}
}

template<class F>void refused(F&& action,const char* why){bool bad=false;try{action();}catch(const std::exception&){bad=true;}need(bad,why);}
struct Window {
 HWND hwnd=CreateWindowExW(0,L"STATIC",L"Actual owned camera UI context CPU gate",WS_OVERLAPPEDWINDOW,
     0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 ~Window(){if(hwnd)DestroyWindow(hwnd);}
};
struct Probe {std::uint64_t native_reads=0,at_read=0;std::function<void()> once,delayed;};
struct RenderCopy {
 std::function<void()>* on_copy;
 std::shared_ptr<ui::BnwOwnedCameraUiSourceInput>* result;
 RenderCopy(std::function<void()>* effect,std::shared_ptr<ui::BnwOwnedCameraUiSourceInput>* output):on_copy(effect),result(output){}
 RenderCopy(const RenderCopy& other):on_copy(other.on_copy),result(other.result){const auto effect=*on_copy;if(effect)effect();}
 void operator()(std::shared_ptr<ui::BnwOwnedCameraUiSourceInput> source)const{*result=std::move(source);}
};
// Only observes a real synchronous public function-target copy. It cannot
// mint any source, owner, pending, currentness or renderer admission.
struct ActionCopy {
 std::function<void()>* on_copy;bool* invoked;
 ActionCopy(std::function<void()>* effect,bool* ran):on_copy(effect),invoked(ran){}
 ActionCopy(const ActionCopy& other):on_copy(other.on_copy),invoked(other.invoked){const auto effect=*on_copy;if(effect)effect();}
 ui::UiCommandResult operator()()const{*invoked=true;return {ui::UiCommandStatus::performed,"actual action"};}
};
struct ComposeCopy {
 std::function<void()>* on_copy;bool* invoked;
 ComposeCopy(std::function<void()>* effect,bool* ran):on_copy(effect),invoked(ran){}
 ComposeCopy(const ComposeCopy& other):on_copy(other.on_copy),invoked(other.invoked){const auto effect=*on_copy;if(effect)effect();}
 void operator()(const ui::BnwClientFrame&)const{*invoked=true;}
};
// This is a fresh genuine test camera's actual source graph. Move it exactly
// once into its scope; the Context consumes that SAME table. No payload copy.
assets::ResourceMountTable camera_table(const std::filesystem::path& root){
 const auto loose=std::make_shared<assets::LooseSource>(root);
 assets::ResourceMountTable table(assets::resource_lookup(loose),assets::resource_lookup_all(loose),assets::resource_keys(loose));
 const assets::AssetEnvironment environment(root);std::uint64_t id=UINT64_MAX;
 for(const auto* name:{"Resource/DX9/UITextures.fpk","Resource/DX9/MiscTextures.fpk",
     "Resource/DX9/Expansion1UITextures.fpk","Resource/DX9/Expansion2UITextures.fpk"}){
  const auto opened=environment.open_fpk_archive(name);
  const auto* source=std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);need(source!=nullptr,"Actual UI archive unavailable");
  table.mount({id--,0,0,assets::resource_lookup(*source),true,true,assets::resource_lookup_all(*source),assets::resource_keys(*source)});
 }
 const auto skin=std::make_shared<assets::LooseResourceIndex>(root);
 skin->scan({root/"Assets/DLC/Expansion2/UI/InGame"});need(skin->indexed_files()!=0,"Actual BNW skin unavailable");
 table.mount({id,0,0,assets::resource_lookup(skin),true,true,assets::resource_lookup_all(skin),assets::resource_keys(skin)});
 return table;
}
}
int main(int argc,char** argv){try{
 ui::native_stamp_diagnostic::Collector stamp_collector;
 ui::native_stamp_diagnostic::ScopedCollector stamp_diagnostic(stamp_collector);
 report_actual_context_progress("main.entry");
 need(argc==4,"Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS");
 const std::filesystem::path save=argv[1],game=argv[2],canonical=argv[3];
report_actual_context_progress("window.extent.before"); const auto window=std::make_shared<Window>();need(window->hwnd!=nullptr,"Actual own CPU HWND unavailable");
 const auto probe=std::make_shared<Probe>();
 const auto actual_extent=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window->hwnd),
     "owned-camera-ui-context-cpu",[]{return std::uint64_t{1};});report_actual_context_progress("window.extent.after");
 runtime::BnwStockLuaClientExtentProvider provider;
 provider.current=[window,probe,actual_extent]{
  ui::currentness_diagnostic::Timing diagnostic_timing(ui::currentness_diagnostic::Lane::native_provider,ui::currentness_diagnostic::Phase::provider_body);
  ++probe->native_reads;
  report_native_provider_progress(probe->native_reads);
  if(probe->at_read==probe->native_reads){probe->at_read=0;auto delayed=std::move(probe->delayed);probe->delayed={};if(delayed)delayed();}
  auto once=std::move(probe->once);probe->once={};if(once)once();
  return actual_extent.current();
 };
report_actual_context_progress("lease.acquire.before"); const auto lease=ui::BnwNativeEarlyInputLease::acquire(save,game,canonical,
     reinterpret_cast<std::uintptr_t>(window->hwnd),std::move(provider));report_actual_context_progress("lease.acquire.after");
report_actual_context_progress("package.registration.before"); const auto package=ui::BnwNativePackageDiscoveryOwner::create(lease);
 package->register_native_dx9_primary_prefix();
 auto settings=ui::BnwNativeResourceBootSettingsOwner::create(package);
 package->register_native_dx9_texture_quality_branch(std::move(settings));package->register_native_common_high_quality_prefix();report_actual_context_progress("package.registration.after");
report_actual_context_progress("bootstrap.initial.before"); auto boot=ui::bootstrap_selected_bnw_preview_from_early_package(package,0);need(bool(boot.preview),"Actual selected preview failed");report_actual_context_progress("bootstrap.initial.after");
 auto preview=std::make_unique<ui::SelectedBnwReadOnlyPreview>(std::move(boot.preview));
 std::shared_ptr<ui::BnwOwnedCameraUiSourceInput> source;
 std::function<void()> on_copy;
 const std::function<void(std::shared_ptr<ui::BnwOwnedCameraUiSourceInput>)> renderer=RenderCopy(&on_copy,&source);
 on_copy=[&]{preview.reset();on_copy={};};
 // Genuine producer retains its actual parent before copying the target.
report_actual_context_progress("source.delivery_and_current.before"); preview->with_owned_camera_ui_source_input(renderer);
 need(!preview&&source,"Real source producer failed copy-before-callback parent retention");source->require_current();report_actual_context_progress("source.delivery_and_current.after");
report_actual_context_progress("saved_session.decode.before"); const auto decoded=saves::read_saved_session(save,saves::SavedSessionDecodeContext{});
 need(decoded.snapshot.pregame&&decoded.snapshot.map_plots&&decoded.snapshot.map_projection,"Actual save scene unavailable");report_actual_context_progress("saved_session.decode.after");
report_actual_context_progress("renderer.receipt.before"); const auto art=saves::observe_saved_map_team_art(*decoded.snapshot.pregame,*decoded.snapshot.map_plots);
 auto receipt=graphics::capture_bnw_renderer_scene_receipt(save,runtime::identify_file(save).sha256,decoded,art);
 need(receipt.has_value(),"Actual source renderer receipt unavailable");report_actual_context_progress("renderer.receipt.after");
report_actual_context_progress("projection.reconcile.before"); auto projection=*decoded.snapshot.map_projection;
 need(graphics::reconcile_bnw_renderer_scene_projection(projection,*receipt,source->input().session.state()),"Actual scene source not reconciled");report_actual_context_progress("projection.reconcile.after");
 // Exercise the NEW real named EARLY source lane on a fresh unregistered
 // package, with a genuine independently decoded receipt and load hook.
 {
report_actual_context_progress("named.package_and_receipt.before");  const auto fresh=ui::BnwNativePackageDiscoveryOwner::create(lease);
  auto named_receipt=graphics::capture_bnw_renderer_scene_receipt(save,runtime::identify_file(save).sha256,decoded,art);
  need(named_receipt.has_value(),"Named EARLY real source receipt unavailable");report_actual_context_progress("named.package_and_receipt.after");
  auto named_projection=*decoded.snapshot.map_projection;bool named_reached=false;
  const char* named_stage="pre-callback";report_actual_context_progress(named_stage);
  const ui::BnwLivePreviewRequest request{save,game,0,{},{}};
report_actual_context_progress("named.launch.before");  const auto outcome=ui::launch_selected_bnw_camera_preview_from_early_package_with_owned_ui_source(
      request,save,fresh,[&](std::shared_ptr<ui::BnwOwnedCameraUiSourceInput> named_source){
       named_reached=true;try {
       named_stage="source-binding";report_actual_context_progress(named_stage);need(named_source&&&named_source->input().session==&named_source->parent()->session(),
           "Named EARLY private input did not retain actual SAME session/parent");
       named_source->parent()->require_same_early_package(*fresh);
       named_stage="camera-table";report_actual_context_progress(named_stage);auto table=camera_table(canonical);
       named_stage="font-scope";report_actual_context_progress(named_stage);auto font=ui::BnwWorldViewFontSourceScope::create(std::move(table),"en_US");
       named_stage="pre-create";report_actual_context_progress(named_stage);const auto* same=&font->resources();
       named_stage="create";report_actual_context_progress(named_stage);auto context=ui::BnwPlotYieldRefreshOwnedUiContext::create(named_source,font,*named_receipt);
       named_stage="post-create";report_actual_context_progress(named_stage);need(&context->font_source()->resources()==same,"Named EARLY context replaced actual moved table");
       const auto current=lease->client_extent();const ui::ClientViewport actual_viewport{
           int(current.width),int(current.height),int(current.width),int(current.height),96};
       named_stage="begin_client";report_actual_context_progress(named_stage);need(bool(context->begin_client(actual_viewport,{})),"Named EARLY actual client capture failed");
       named_stage="begin_ui";report_actual_context_progress(named_stage);const auto ui=context->begin_ui(actual_viewport.framebuffer_width,actual_viewport.framebuffer_height);
       named_stage="accepts";report_actual_context_progress(named_stage);need(context->scene()->accepts(*context->host(),ui),"Named EARLY actual host/provider publication differs");
       named_stage="retire";report_actual_context_progress(named_stage);context->retire();
       named_stage="callback-returned";report_actual_context_progress(named_stage);
       }catch(const std::filesystem::filesystem_error& error){
        try {
        const auto code=error.code();const auto condition=code.default_error_condition();
        std::cerr<<"Named EARLY filesystem error: stage="<<named_stage
            <<" code="<<code.value()<<" category="<<code.category().name()
            <<" condition="<<condition.value()<<" condition_category="<<condition.category().name()
            <<" path1="<<error.path1()<<" path2="<<error.path2()<<'\n';
        }catch(...){}
        throw;
       }
      },&*named_receipt,[&](const ui::LiveBnwPreviewInput* input,graphics::BnwRendererSceneReceipt& actual){
       need(input!=nullptr,"Named EARLY load hook lost its real input");
report_actual_context_progress("named.load_reconcile.before");       const bool reconciled=graphics::reconcile_bnw_renderer_scene_projection(named_projection,actual,input->session.state());
       if(reconciled)*named_receipt=actual;report_actual_context_progress("named.load_reconcile.after");return reconciled;
      });report_actual_context_progress("named.launch.after");
  if(!outcome.rendered||!named_reached)
   std::cerr<<"Named EARLY owned-source outcome: rendered="<<outcome.rendered
       <<" named_reached="<<named_reached<<" stage="<<named_stage<<" blocker="
       <<(outcome.blocker?outcome.blocker->reason:"<none>")<<'\n';
  need(outcome.rendered&&named_reached,"Real named EARLY owned-source callback was not completed");
 }
report_actual_context_progress("negative.viewport.before"); const auto extent=lease->client_extent();
 const ui::ClientViewport viewport{int(extent.width),int(extent.height),int(extent.width),int(extent.height),96};report_actual_context_progress("negative.viewport.after");
 auto make_context=[&]{report_actual_context_progress("make_context.entry");auto table=camera_table(canonical);
report_actual_context_progress("make_context.table.after_font.before");  auto font=ui::BnwWorldViewFontSourceScope::create(std::move(table),"en_US");const auto* same_table=&font->resources();
report_actual_context_progress("make_context.font.after_create.before");  auto result=ui::BnwPlotYieldRefreshOwnedUiContext::create(source,font,*receipt);
report_actual_context_progress("make_context.create.after_resource_check.before");  need(&result->font_source()->resources()==same_table,"Owned Context substituted its actual moved resource table");report_actual_context_progress("make_context.return");return result;};
report_actual_context_progress("negative.no_op_ownership_aliases.entry"); {auto table=camera_table(canonical);auto font=ui::BnwWorldViewFontSourceScope::create(std::move(table),"en_US");
  auto source_alias=std::shared_ptr<ui::BnwOwnedCameraUiSourceInput>(source.get(),[](auto*){});
  auto font_alias=std::shared_ptr<ui::BnwWorldViewFontSourceScope>(font.get(),[](auto*){});
  const auto before=probe->native_reads;
  refused([&]{ui::BnwPlotYieldRefreshOwnedUiContext::create(source_alias,font,*receipt);},"No-op source alias granted ownership");
  refused([&]{ui::BnwPlotYieldRefreshOwnedUiContext::create(source,font_alias,*receipt);},"No-op font scope alias granted ownership");
  need(probe->native_reads==before,"Canonical owner rejection reached borrowed native callbacks");
 }report_actual_context_progress("negative.no_op_ownership_aliases.exit");
 auto capture=[&](const auto& context){report_actual_context_progress("capture.client.before");need(bool(context->begin_client(viewport,{})),"Actual client publication absent");
report_actual_context_progress("capture.client.after_ui.before");  auto frame=context->begin_ui(viewport.framebuffer_width,viewport.framebuffer_height);
report_actual_context_progress("capture.ui.after_accepts.before");  need(context->scene()->accepts(*context->host(),frame),"Actual host/provider publication differs");report_actual_context_progress("capture.return");return frame;};
 {
  auto context=make_context();auto frame=capture(context);auto child=context->yield_adapter();
  const auto reads=probe->native_reads;context.reset();
  refused([&]{child->require_current();},"Adapter kept Context facade alive after loss");
  refused([&]{child->advance({},frame);},"Facade-loss adapter entered sources");
  need(probe->native_reads==reads,"Facade-loss child invoked retired native sources");
 }
 // These are real retained Context callbacks, not synthetic gameplay effects.
 for(int lane=0;lane<4;++lane){
  auto context=make_context();auto frame=capture(context);auto child=context->yield_adapter();
  std::weak_ptr<ui::BnwPlotYieldRefreshOwnedUiContext> weak=context;
  probe->once=[&]{
   if(lane==0)context->retire();
   else if(lane==1)context.reset();
   else if(lane==2)refused([&]{context->begin_ui(viewport.framebuffer_width,viewport.framebuffer_height);},
       "Adapter callback reentered actual Context operation");
   else refused([&]{child->require_current();},"Adapter callback recursively read busy child");
  };
  refused([&]{child->require_current();},"Adapter accepted callback retirement/reentry");
  need(!probe->once,"Adapter skipped actual native currentness callback");
  if(lane==1)need(weak.expired(),"Adapter callback retained Context facade cycle");
  const auto reads=probe->native_reads;
  refused([&]{child->advance({},frame);},"Callback-retired adapter entered publication");
  need(probe->native_reads==reads,"Callback-retired adapter invoked further native source");
 }
 // Arm retirement in one real currentness callback, fire it at the next
 // borrowed native read. No assumed number of internal provider calls.
 for(int lane=0;lane<2;++lane){
  auto context=make_context();auto frame=capture(context);auto child=context->yield_adapter();
  std::weak_ptr<ui::BnwPlotYieldRefreshOwnedUiContext> weak=context;
  bool fired=false,rejected=false;
  probe->once=[&]{
   probe->at_read=probe->native_reads+1;
   probe->delayed=[&]{fired=true;if(lane==0)context->retire();else context.reset();};
  };
  for(int attempt=0;attempt<4&&!fired;++attempt){
   try{child->require_current();}catch(const std::exception&){rejected=true;}
  }
  need(fired&&rejected&&!probe->once&&!probe->delayed&&!probe->at_read,
       "Delayed adapter callback did not refuse actual retirement");
  if(lane==1)need(weak.expired(),"Delayed adapter callback retained facade cycle");
  const auto reads=probe->native_reads;
  refused([&]{child->advance({},frame);},"Delayed-retired adapter entered publication");
  need(probe->native_reads==reads,"Delayed-retired child reached further native callbacks");
 }
 {
  auto context=make_context();auto frame=capture(context);auto child=context->yield_adapter();
  const auto reads=probe->native_reads;bool refused_foreign=false;
  std::thread foreign([&]{try{child->advance({},frame);}catch(const std::exception&){refused_foreign=true;}});foreign.join();
  need(refused_foreign&&probe->native_reads==reads,"Foreign adapter entered native sources");
  refused([&]{child->require_current();},"Foreign adapter operation did not quarantine child");
 }
report_actual_context_progress("actual.city_selection_private_bundle_and_retirement.entry"); {
  auto context=make_context();auto frame=capture(context);
  need(context->screens().registry().find("CityList"),"Actual CityList route absent");
  need(context->dispatch(ui::UiOpenScreen{"CityList"},frame).status==ui::UiCommandStatus::navigation,"Actual CityList did not open");
  frame=capture(context);const auto* cities=frame.snapshot.template find<ui::CityScreenFrame>();
  need(cities&&!cities->cities.empty(),"Actual selected owner cities absent");
  const auto city=cities->cities.front().id;
  const ui::UiCommand open=ui::UiGameplayCommand::make("CityList",ui::WorldViewAction{ui::WorldViewActionKind::open_city,city});
  need(context->dispatch(open,frame).status==ui::UiCommandStatus::navigation,"Actual live CityList did not select CityView");
  frame=capture(context);cities=frame.snapshot.template find<ui::CityScreenFrame>();
  need(cities&&cities->selected_city_id==city&&context->host()->selected_city()==std::optional(std::pair{source->input().active_player_id,city}),
      "Actual CityView selection/provider handoff differs");
  const auto adapter=context->yield_adapter();auto stripped=frame;
  stripped.snapshot=ui::UiFrameSnapshot(frame.snapshot.frame(),frame.snapshot.active_player_id(),frame.snapshot.turn());
  const auto before=probe->native_reads;refused([&]{adapter->advance({},stripped);},"Public stripped payload was admitted");
  need(probe->native_reads==before,"Private issue rejection reached native callbacks");
   auto foreign=frame;foreign.owner=nullptr;
   refused([&]{adapter->advance({},foreign);},"Foreign issued owner was admitted");
   need(probe->native_reads==before,"Foreign issue rejection reached native callbacks");
  need(context->dispatch(ui::UiCloseScreen{"CityView"},frame).status==ui::UiCommandStatus::navigation,"Actual CityView did not close");
  frame=capture(context);need(!context->host()->selected_city(),"Actual closed CityView retained host selection");
  const auto host=context->host();const auto scene=context->scene();context->retire();
   const auto adapter_reads=probe->native_reads;
   refused([&]{adapter->require_current();},"Retired context adapter retained live source authority");
   refused([&]{adapter->advance({},frame);},"Retired context adapter entered publication");
   need(probe->native_reads==adapter_reads,"Retired adapter invoked native source callbacks");
  const auto retired_reads=probe->native_reads;const auto focused=host->screens().focused_screen();
  refused([&]{host->dispatch(ui::UiOpenScreen{"CityList"},frame);},"Retired retained host action reached borrowed controller");
  need(host->screens().focused_screen()==focused&&!host->selected_city(),"Retired action changed actual controller/selection");
  refused([&]{host->begin_frame(viewport.framebuffer_width,viewport.framebuffer_height);},"Retired retained host borrowed its sources");
  refused([&]{scene->owner().ui_provider()();},"Retired retained FrameOwner borrowed its sources");
  auto& retained_owner=const_cast<ui::BnwClientFrameOwner&>(scene->owner());
  refused([&]{retained_owner.unit_dirty_state();},"Retired retained Scene read unit dirty storage");
  refused([&]{retained_owner.take_unit_dirty_events();},"Retired retained Scene consumed unit dirty queue");
  refused([&]{retained_owner.take_city_dirty_events();},"Retired retained Scene consumed city dirty queue");
  refused([&]{host->unit_dirty_state();},"Retired retained Host read unit dirty storage");
  refused([&]{host->take_unit_dirty_events();},"Retired retained Host consumed unit dirty queue");
  need(probe->native_reads==retired_reads,"Retirement rejection reached borrowed native sources");
 }report_actual_context_progress("actual.city_selection_private_bundle_and_retirement.exit");
// Prepared lifetime regressions use the actual owned Context and canonical
// plot input. No successful native Dim operation or fabricated dirty event.
report_actual_context_progress("actual.dirty_api_stale_issued_frame.entry"); {
  auto context=make_context();capture(context);const auto host=context->host();const auto scene=context->scene();
  auto& owner=const_cast<ui::BnwClientFrameOwner&>(scene->owner());
  const auto issued=owner.current_frame();need(bool(issued),"Canonical plot input has no current publication");
  const auto before=owner.unit_dirty_state();
  const std::optional<std::int64_t> plot=issued->stamp.selection.plot?std::nullopt:std::optional<std::int64_t>{0};
  owner.select_plot(plot,issued);
  need(!owner.is_current(issued),"Actual canonical selection revision did not stale its issued Frame");
  const auto dirty=owner.unit_dirty_state();
  need(dirty.client_revision>before.client_revision&&dirty==host->unit_dirty_state(),"Live storage dirty state rejected a legitimate producer revision");
  need(dirty.events.empty(),"Canonical plot input fabricated native unit dirty events");
  need(owner.take_unit_dirty_events().empty()&&host->take_unit_dirty_events().empty(),"Empty real dirty bank fabricated transfer events");
  (void)owner.take_city_dirty_events();
 }report_actual_context_progress("actual.dirty_api_stale_issued_frame.exit");
report_actual_context_progress("negative.dirty_api_source_retirement.entry");
 for(int lane=0;lane!=5;++lane){
  auto context=make_context();capture(context);const auto host=context->host();const auto scene=context->scene();
  auto& owner=const_cast<ui::BnwClientFrameOwner&>(scene->owner());
  probe->once=[&]{context->retire();};
  refused([&]{switch(lane){
   case 0:(void)owner.unit_dirty_state();break;
   case 1:(void)owner.take_unit_dirty_events();break;
   case 2:(void)owner.take_city_dirty_events();break;
   case 3:(void)host->unit_dirty_state();break;
   default:(void)host->take_unit_dirty_events();break;
  }},"Dirty API published after actual source callback retired its Context");
  need(!probe->once,"Dirty API bypassed actual source-currentness callback");
 }
report_actual_context_progress("negative.dirty_api_source_retirement.exit");
report_actual_context_progress("negative.dirty_api_foreign_thread.entry"); {
  auto context=make_context();capture(context);const auto host=context->host();const auto scene=context->scene();
  auto& owner=const_cast<ui::BnwClientFrameOwner&>(scene->owner());const auto before=probe->native_reads;
  std::thread foreign([&]{
   refused([&]{owner.unit_dirty_state();},"Foreign Scene read unit dirty storage");
   refused([&]{owner.take_unit_dirty_events();},"Foreign Scene consumed unit dirty queue");
   refused([&]{owner.take_city_dirty_events();},"Foreign Scene consumed city dirty queue");
   refused([&]{host->unit_dirty_state();},"Foreign Host read unit dirty storage");
   refused([&]{host->take_unit_dirty_events();},"Foreign Host consumed unit dirty queue");
  });foreign.join();need(probe->native_reads==before,"Foreign dirty API reached native sources");
 }report_actual_context_progress("negative.dirty_api_foreign_thread.exit");
report_actual_context_progress("negative.child_last_context_release.entry"); {
  auto context=make_context();capture(context);const auto host=context->host();const auto scene=context->scene();
  std::weak_ptr<ui::BnwPlotYieldRefreshOwnedUiContext> weak=context;
  probe->once=[&]{context.reset();};
  // Direct retained child does not pin a Context facade. A callback releasing
  // it retires storage and the child must refuse rather than publish success.
  refused([&]{host->begin_frame(viewport.framebuffer_width,viewport.framebuffer_height);},
      "Direct child published after final Context facade callback release");
  need(weak.expired(),"Retained children formed a Context ownership cycle");
  const auto before=probe->native_reads;
  refused([&]{scene->owner().ui_provider()();},"Released Context retained FrameOwner authority");
  need(probe->native_reads==before,"Released Context rejection borrowed native sources");
 }report_actual_context_progress("negative.child_last_context_release.exit");
report_actual_context_progress("negative.public_operation_self_pin_release.entry"); {
  auto context=make_context();capture(context);const auto host=context->host();
  std::weak_ptr<ui::BnwPlotYieldRefreshOwnedUiContext> weak=context;
  probe->once=[&]{context.reset();};
  // The public Context operation has a real self/source pin through callbacks;
  // releasing its last external owner cannot tear down borrowed controllers.
  const auto completed=context->begin_ui(viewport.framebuffer_width,viewport.framebuffer_height);
  need(weak.expired(),"In-flight pin immortalized the Context facade");
  refused([&]{host->is_current_frame(completed);},"Completed retired in-flight frame remained admitted");
 }report_actual_context_progress("negative.public_operation_self_pin_release.exit");
report_actual_context_progress("negative.explicit_retirement_during_callback.entry"); {
  auto context=make_context();capture(context);
  probe->once=[&]{context->retire();};
  refused([&]{context->begin_ui(viewport.framebuffer_width,viewport.framebuffer_height);},
      "Explicit retirement during actual native callback published UI");
 }report_actual_context_progress("negative.explicit_retirement_during_callback.exit");
report_actual_context_progress("negative.foreign_thread_retirement.entry"); {
  auto context=make_context();capture(context);const auto before=probe->native_reads;bool rejected=false;
  std::thread foreign([&]{try{context->require_current();}catch(const std::exception&){rejected=true;}});foreign.join();
  need(rejected&&probe->native_reads==before,"Foreign thread entered borrowed native sources");
  refused([&]{context->require_current();},"Foreign call did not permanently close this UI storage epoch");
 }report_actual_context_progress("negative.foreign_thread_retirement.exit");
report_actual_context_progress("negative.getter_last_context_release.entry"); {
  auto context=make_context();capture(context);std::weak_ptr<ui::BnwPlotYieldRefreshOwnedUiContext> weak=context;
  probe->once=[&]{context.reset();};
  refused([&]{context->host();},"Getter returned member borrow after actual Context facade callback loss");
  need(weak.expired(),"Getter source validation pin immortalized Context facade");
 }report_actual_context_progress("negative.getter_last_context_release.exit");
report_actual_context_progress("negative.caught_context_reentry.entry"); {
  auto context=make_context();capture(context);bool inner_refused=false;
  probe->once=[&]{try{context->begin_ui(viewport.framebuffer_width,viewport.framebuffer_height);}catch(const std::exception&){inner_refused=true;}};
  refused([&]{context->begin_ui(viewport.framebuffer_width,viewport.framebuffer_height);},
      "Caught nested public context entry still admitted the outer UI operation");
  need(inner_refused,"Actual nested callback route was not reached");
  refused([&]{context->require_current();},"Caught context reentry did not permanently retire storage");
 }report_actual_context_progress("negative.caught_context_reentry.exit");
report_actual_context_progress("negative.borrowed_host_all_owners_release.entry"); {
  auto context=make_context();capture(context);auto host=context->host();auto scene=context->scene();
  std::weak_ptr<ui::BnwPlotYieldRefreshOwnedUiContext> weak=context;auto* raw=host.get();
  probe->once=[&]{context.reset();scene.reset();host.reset();};
  refused([&]{raw->begin_frame(viewport.framebuffer_width,viewport.framebuffer_height);},
      "Borrowed retained host queried freed storage after all actual owners were released");
  need(weak.expired()&&!host&&!scene,"Last host/context/Scene native callback loss did not occur");
 }report_actual_context_progress("negative.borrowed_host_all_owners_release.exit");
report_actual_context_progress("negative.borrowed_publication_all_owners_release.entry"); {
  auto context=make_context();auto frame=capture(context);auto host=context->host();auto scene=context->scene();
  const auto publication=scene->frame();const auto* raw=&scene->owner();
  std::weak_ptr<ui::BnwPlotYieldRefreshOwnedUiContext> weak=context;
  probe->once=[&]{context.reset();host.reset();scene.reset();};
  refused([&]{raw->is_current(publication);},
      "Borrowed publication owner queried freed storage after last child callback loss");
  need(weak.expired()&&!host&&!scene,"Last publication/context/Scene native callback loss did not occur");
  (void)frame;
 }report_actual_context_progress("negative.borrowed_publication_all_owners_release.exit");
report_actual_context_progress("negative.delayed_capture_last_child_release.entry"); {
  // Arm from the actual CityBanner capture's successful source resolution,
  // after FrameOwner installed ClearCapture. The existing observer supplies
  // no resource/owner authority; only the NEXT genuine Native query drops owners.
  auto context=make_context();capture(context);auto host=context->host();auto scene=context->scene();
  need(!source->input().frame_sources.selected_unit&&!host->selected_city(),"Delayed capture exact real source prefix differs");
  const auto font=context->font_source();const auto& resources=font->resources();
  auto* raw=scene.get();std::weak_ptr<ui::BnwClientSceneFrame> weak_scene=scene;
  std::weak_ptr<ui::BnwPlotYieldRefreshOwnedUiContext> weak=context;
  const auto before=probe->native_reads;bool armed=false,delayed=false;
  std::uint64_t armed_read=0,fired_read=0;
  need(!probe->once&&!probe->delayed&&!probe->at_read,"Delayed capture probe has a prior pending callback");
  assets::ScopedResourceResolveObserver observer(resources,
    [&](const assets::AssetKey& key,std::span<const assets::MountedResource> results,bool all){
     if(armed||all||key.original_name()!="Assets/UI/InGame/CityBannerManager.lua"||
        results.size()!=1||!std::holds_alternative<assets::AssetHandle>(results.front().asset))return;
     armed_read=probe->native_reads;
     probe->delayed=[&]{
      fired_read=probe->native_reads;delayed=true;
      need(armed&&fired_read==armed_read+1,"Delayed capture did not run on next genuine Native query");
      context.reset();host.reset();scene.reset();
      need(weak.expired()&&!weak_scene.expired(),"Capture child pin retained facade or lost actual Scene during callback");};
     probe->at_read=armed_read+1;armed=true;
    });
  refused([&]{raw->begin(viewport,{});},"Delayed capture accepted after facade loss inside guarded source");
  // Source.current completes its existing package post-lease and Host lease
  // queries, then storage rejects retirement. No later guarded source may borrow.
  need(!observer.failed()&&armed&&delayed&&armed_read>before&&fired_read==armed_read+1&&
      probe->native_reads==fired_read+2&&!probe->at_read&&!probe->delayed&&
      weak.expired()&&weak_scene.expired(),
      "Actual after-ClearCapture last-child destruction boundary was not reached/released");
 }report_actual_context_progress("negative.delayed_capture_last_child_release.exit");
 // Actual public Scene helper owns only in-flight children; action loss/throw
 // invalidates its retained real FrameOwner while refusing accepted effects.
 for(int lane=0;lane<2;++lane){report_actual_context_progress("negative.scene_dispatch_action_release.entry",lane);
  auto context=make_context();const auto frame=capture(context);auto host=context->host();auto scene=context->scene();
  auto* raw=scene.get();auto* actual_host=host.get();std::weak_ptr<ui::BnwClientSceneFrame> weak_scene=scene;
  std::weak_ptr<ui::LiveUiHost> weak_host=host;std::weak_ptr<ui::BnwPlotYieldRefreshOwnedUiContext> weak=context;
  bool action=false;
  refused([&]{raw->dispatch(*actual_host,frame,[&]()->ui::UiCommandResult{
   action=true;context.reset();scene.reset();host.reset();
   need(weak.expired()&&!weak_scene.expired()&&!weak_host.expired(),"Actual dispatch child lifetime/facade retirement differs");
   if(lane==1)throw std::runtime_error("Actual action throws after owner release");
   return {ui::UiCommandStatus::performed,"actual helper reached"};
  });},"Scene accepted uncertain action after last actual context/children loss");
  need(action&&weak.expired()&&weak_scene.expired()&&weak_host.expired(),"Actual dispatch throw/drop boundary not reached/released");
report_actual_context_progress("negative.scene_dispatch_action_release.exit",lane); }
 // Child pins precede arbitrary function-target copies as well as invocation.
 for(int lane=0;lane<2;++lane){report_actual_context_progress("negative.child_function_target_copy_release.entry",lane);
  auto context=make_context();const auto frame=capture(context);auto host=context->host();auto scene=context->scene();
  auto* raw=scene.get();auto* actual_host=host.get();std::weak_ptr<ui::BnwClientSceneFrame> weak_scene=scene;
  std::weak_ptr<ui::BnwPlotYieldRefreshOwnedUiContext> weak=context;
  bool invoked=false,copied=false;std::function<void()> on_copy;
  const std::function<ui::UiCommandResult()> action=ActionCopy(&on_copy,&invoked);
  const std::function<void(const ui::BnwClientFrame&)> compose=ComposeCopy(&on_copy,&invoked);
  on_copy=[&]{copied=true;on_copy={};context.reset();host.reset();scene.reset();
   need(weak.expired()&&!weak_scene.expired(),"Child function-target copy lost actual pin or retained Context facade");};
  refused([&]{if(lane==0)(void)raw->dispatch(*actual_host,frame,action);else(void)raw->present(compose);},
      "Actual child callback target copy accepted after last facade release");
  need(copied&&!invoked&&weak.expired()&&weak_scene.expired(),"Child callback target copy lifetime/refusal boundary differs");
report_actual_context_progress("negative.child_function_target_copy_release.exit",lane); }
 // Reset the actual Scene.frame_ through the real zero-viewport begin route
 // inside composition. No external strong BnwClientFrame masks the lifetime.
 // The compose reference remains readable, then both Scene and direct owned
 // FrameOwner presentation refuse after Context retirement.
 for(int lane=0;lane<2;++lane){report_actual_context_progress("negative.presentation_frame_reset_release.entry",lane);
  auto context=make_context();capture(context);auto host=context->host();auto scene=context->scene();
  auto* raw=scene.get();const auto* owner=&scene->owner();
  const auto expected=scene->frame()->stamp.identity;const auto plots=scene->frame()->world.map_projection->cells.size();
  std::weak_ptr<const ui::BnwClientFrame> weak_frame=scene->frame();
  std::weak_ptr<ui::BnwClientSceneFrame> weak_scene=scene;
  std::weak_ptr<ui::BnwPlotYieldRefreshOwnedUiContext> weak=context;bool composed=false,read_after_release=false;
  const std::function<void(const ui::BnwClientFrame&)> compose=[&](const auto& delivered){
   composed=true;need(!raw->begin({0,0,0,0,96},{}),"Actual minimized route unexpectedly published");
   need(!raw->frame(),"Actual Scene member frame was not reset during composition");
   context.reset();host.reset();scene.reset();
   need(weak.expired()&&!weak_scene.expired()&&!weak_frame.expired(),"Presentation retained Context or lost actual immutable frame/Scene pin");
   need(delivered.stamp.identity==expected&&delivered.world.map_projection&&
        delivered.world.map_projection->cells.size()==plots,"Compose reference expired after actual member/publication reset");
   read_after_release=true;
  };
  refused([&]{if(lane==0)(void)raw->present(compose);else(void)owner->present(raw->frame(),compose);},
      "Actual presentation was accepted after composition retired source context");
  need(composed&&read_after_release&&weak.expired()&&weak_scene.expired()&&weak_frame.expired(),
      "Actual Scene/FrameOwner compose reset/drop boundary not reached/released");
report_actual_context_progress("negative.presentation_frame_reset_release.exit",lane); }
report_actual_context_progress("negative.identical_checkpoint_restore.entry"); {
  auto context=make_context();capture(context);auto& session=source->input().session;const auto checkpoint=session.checkpoint();
  probe->once=[&]{session.restore(checkpoint);};
  refused([&]{context->begin_ui(viewport.framebuffer_width,viewport.framebuffer_height);},
      "Identical restore during callback re-admitted old source/allocation");
  refused([&]{source->require_current();},"Old private input accepted identical checkpoint restore");
 }report_actual_context_progress("negative.identical_checkpoint_restore.exit");
 // Each standalone public lane uses a FRESH real privately minted input.
 // The Native observer releases its last facade after the producer has returned;
 // retained Impl keeps actual borrowed parents/resources alive until refusal.
 for(int lane=0;lane<3;++lane){report_actual_context_progress("negative.standalone_source_facade_release.entry",lane);
report_actual_context_progress("standalone.package.before",lane);  const auto fresh=ui::BnwNativePackageDiscoveryOwner::create(lease);
  fresh->register_native_dx9_primary_prefix();auto settings=ui::BnwNativeResourceBootSettingsOwner::create(fresh);
  fresh->register_native_dx9_texture_quality_branch(std::move(settings));fresh->register_native_common_high_quality_prefix();report_actual_context_progress("standalone.package.after",lane);
report_actual_context_progress("standalone.bootstrap.before",lane);  auto boot=ui::bootstrap_selected_bnw_preview_from_early_package(fresh,0);need(bool(boot.preview),"Fresh standalone real source parent unavailable");report_actual_context_progress("standalone.bootstrap.after",lane);
  std::shared_ptr<ui::BnwOwnedCameraUiSourceInput> actual;
  auto wrapper=std::make_unique<ui::SelectedBnwReadOnlyPreview>(std::move(boot.preview));
report_actual_context_progress("standalone.source.before",lane);  wrapper->with_owned_camera_ui_source_input([&](auto input){actual=std::move(input);});report_actual_context_progress("standalone.source.after",lane);
  auto* borrowed=actual.get();std::weak_ptr<ui::BnwOwnedCameraUiSourceInput> weak=actual;
  const auto before=probe->native_reads;probe->once=[&]{actual.reset();wrapper.reset();};
  refused([&]{if(lane==0)borrowed->require_current();else if(lane==1)(void)borrowed->input();else(void)borrowed->parent();},
      "Standalone public source query returned after last facade callback loss");
  need(weak.expired()&&probe->native_reads>before,"Standalone last-facade real native callback lane was not reached");
report_actual_context_progress("negative.standalone_source_facade_release.exit",lane); }
// Controlled v6 cache data is admitted by codec into a NEW genuine EARLY
// Host before private UI/source creation. No live mutation or invented pending.
report_actual_context_progress("actual.plot_dirty_controlled_codec_positive.entry"); {
 const auto original_sha=runtime::identify_file(save).sha256;
 auto base_package=ui::BnwNativePackageDiscoveryOwner::create(lease);
 base_package->register_native_dx9_primary_prefix();
 auto base_settings=ui::BnwNativeResourceBootSettingsOwner::create(base_package);
 base_package->register_native_dx9_texture_quality_branch(std::move(base_settings));
 base_package->register_native_common_high_quality_prefix();
 auto base_boot=ui::bootstrap_selected_bnw_preview_from_early_package(base_package,0);
 need(bool(base_boot.preview),"Controlled fixture genuine baseline Host absent");
 std::shared_ptr<ui::SelectedBnwPreviewHost> baseline(std::move(base_boot.preview.host));
 const auto actor=baseline->active_player_id();const auto& city_roster=baseline->session().state().cities.players.at(actor).cities;
 need(!city_roster.empty(),"Controlled fixture actual city absent");
 const auto& chosen=city_roster.begin()->second;
 const auto chosen_id=city_roster.begin()->first;
 auto world=baseline->session().state().world;
 need(world.map_plots&&world.map_projection&&world.map_header.grid.width>0&&world.map_header.grid.height>0&&
      chosen.x>=0&&chosen.y>=0&&chosen.x<world.map_header.grid.width&&chosen.y<world.map_header.grid.height,
      "Controlled fixture real city-center map absent");
 const auto plot=static_cast<std::int64_t>(chosen.y*world.map_header.grid.width+chosen.x);
 need(plot>=0&&static_cast<std::uint64_t>(plot)<world.map_plots->plots.size(),"Controlled city-center index outside map");
 auto& cell=world.map_projection->cells.at(static_cast<std::size_t>(plot));
 need(cell.cached_yields.has_value(),"Controlled fixture cached six-shorts absent");
 std::vector<std::int64_t>* raw=nullptr;
 for(auto& row:world.map_plots->plots.at(static_cast<std::size_t>(plot)).arrays)
  if(row.name&&std::string_view(row.name)=="yields")raw=&row.values;
 need(raw&&raw->size()==6,"Controlled fixture actual raw six-shorts absent");
 for(std::int64_t yield=0;yield<6;++yield){
  const auto old=raw->at(static_cast<std::size_t>(yield));
  const auto value=old==INT16_MIN?old+1:old-1;
  raw->at(static_cast<std::size_t>(yield))=value;cell.cached_yields->at(yield)=value;
 }
 // Preserve real import-clock/transient owners and every original source ID.
 // Legacy two-section imports cannot grant the same genuine UI source.
 auto document=baseline->export_import_clock_checkpoint().document;
 const auto id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_map_plots);
 std::erase_if(document.sections,[&](const auto& row){return row.id==id;});
 document.sections.push_back(runtime::make_bnw_map_plot_checkpoint(world,
     baseline->session().source().snapshot,baseline->identity(),{}));
 const auto unique=std::chrono::steady_clock::now().time_since_epoch().count();
 const auto path=std::filesystem::temp_directory_path()/
     ("bnw-plot-ui-bridge-cache-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(unique)+".CivisSave");
 saves::write_civis_save(path,document); // Exclusive CREATE_NEW; never original.
 struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code e;std::filesystem::remove(path,e);}} cleanup{path};
 runtime::BnwStockLuaClientExtentProvider fresh_provider;
 fresh_provider.current=[window,probe,actual_extent]{
  ++probe->native_reads;
  if(probe->at_read==probe->native_reads){probe->at_read=0;auto fn=std::move(probe->delayed);probe->delayed={};if(fn)fn();}
  auto fn=std::move(probe->once);probe->once={};if(fn)fn();return actual_extent.current();
 };
 const auto fresh_lease=ui::BnwNativeEarlyInputLease::acquire(path,game,canonical,
     reinterpret_cast<std::uintptr_t>(window->hwnd),std::move(fresh_provider));
 auto package=ui::BnwNativePackageDiscoveryOwner::create(fresh_lease);
 package->register_native_dx9_primary_prefix();auto settings=ui::BnwNativeResourceBootSettingsOwner::create(package);
 package->register_native_dx9_texture_quality_branch(std::move(settings));package->register_native_common_high_quality_prefix();
 auto bootstrap=ui::bootstrap_selected_bnw_preview_from_early_package(package,actor);
 need(bool(bootstrap.preview),"Controlled codec NEW actual Host import failed");
 auto wrapper=std::make_unique<ui::SelectedBnwReadOnlyPreview>(std::move(bootstrap.preview));
 std::shared_ptr<ui::BnwOwnedCameraUiSourceInput> controlled;
 wrapper->with_owned_camera_ui_source_input([&](auto actual){controlled=std::move(actual);});
 need(controlled&&controlled->parent()!=baseline,"Controlled fixture reused original Host authority");
 const auto& restored=controlled->input().session.source();
 const auto team_art=saves::observe_saved_map_team_art(*restored.snapshot.pregame,*restored.snapshot.map_plots);
 auto actual_receipt=graphics::capture_bnw_renderer_scene_receipt(path,runtime::identify_file(path).sha256,restored,team_art);
 need(actual_receipt.has_value(),"Controlled actual renderer-source receipt absent");
 auto actual_projection=*restored.snapshot.map_projection;
 need(graphics::reconcile_bnw_renderer_scene_projection(actual_projection,*actual_receipt,controlled->input().session.state()),
      "Controlled actual source projection mismatch");
 auto font=ui::BnwWorldViewFontSourceScope::create(camera_table(canonical),"en_US");
 auto context=ui::BnwPlotYieldRefreshOwnedUiContext::create(controlled,font,*actual_receipt);
 auto frame=capture(context);
 need(context->dispatch(ui::UiOpenScreen{"CityList"},frame).status==ui::UiCommandStatus::navigation,
      "Controlled real CityList navigation absent");
 frame=capture(context);
 const ui::UiCommand open=ui::UiGameplayCommand::make("CityList",ui::WorldViewAction{ui::WorldViewActionKind::open_city,chosen_id});
 need(context->dispatch(open,frame).status==ui::UiCommandStatus::navigation,"Controlled real city selection absent");
 frame=capture(context);const auto old_publication=context->scene()->owner().current_frame();
 need(bool(old_publication),"Controlled genuine pre-prefix frame absent");
 const auto before=controlled->input().session.read_state_lease();const auto generation=controlled->input().session.state_generation();
 const auto occurrence=before->city_dirty_next_occurrence;const auto revision=before->ui_source_revision;
 const auto rng=before->random.gameplay.state();
 const auto rules=runtime::BnwNormalPlotYieldRules::load(controlled->parent()->database(),controlled->parent()->identity().rules_sha256);
 auto pending=controlled->input().session.begin_bnw_plot_normal_yield_refresh(plot,rules);
 const auto progress=pending->progress();
 need(progress.plot_writes>0&&progress.phase==runtime::BnwPlotYieldRefreshPhase::city_screen_predicate,
      "Controlled actual selected city did not reach genuine changed predicate");
 need(!context->scene()->owner().is_current(old_publication)&&before->ui_source_revision>revision,
      "Actual prefix stores did not stale old frame");
 const auto adapter=context->yield_adapter();bool mutated=false;
 // Borrowed caller frame is destroyed/replaced by a real currentness callback
 // AFTER entry. The adapter must use its retained copy throughout.
 probe->once=[&]{mutated=true;frame=ui::LiveUiFrame{ui::UiFrameSnapshot{0,std::nullopt,std::nullopt}};};
 const auto result=adapter->advance(pending,frame);
 need(mutated&&!probe->once&&result.dirty_deliveries>0&&result.refreshed_ui&&
      result.progress.phase==runtime::BnwPlotYieldRefreshPhase::symbols,
      "Actual CityScreen advance failed or manufactured symbols completion");
 need(context->scene()->accepts(*context->host(),*result.refreshed_ui)&&
      controlled->input().session.read_state_lease()==before&&controlled->input().session.state_generation()==generation&&
      before->city_dirty_next_occurrence>occurrence&&before->random.gameplay.state()==rng&&
      before->city_dirty_events.size()>=result.dirty_deliveries,
      "Actual consumption lost factual events/allocation/RNG/current publication");
 need(!context->scene()->accepts(*context->host(),frame),"Callback-replaced caller frame acquired authority");
 need(!ui::BnwPlotYieldRefreshUiAdapter::symbols_listener_ready()&&!controlled->input().session.readiness().can_continue(),
      "Controlled dirty publication granted renderer/full-turn continuation");
 need(runtime::identify_file(save).sha256==original_sha,"Controlled fixture modified original save");
 context->retire();
}report_actual_context_progress("actual.plot_dirty_controlled_codec_positive.exit");
report_actual_context_progress("all_scenarios.before_existing_PASS_output"); std::cout<<"PASS actual privately minted EARLY CPU UI source/context/CityView/provider retention gates; no GPU/native VM/plot-symbol/full-refresh claim\n";
report_actual_context_progress("all_scenarios.after_existing_PASS_output");}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
