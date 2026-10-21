#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include <iostream>
#include <functional>
#include "civ5/ui/bnw_manual_cycle_key_input.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
namespace {
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
template<class F> void refused_exact(F&& f,std::string_view expected,const char* why){
 bool refused=false;try{f();}catch(const std::exception&e){refused=std::string_view(e.what())==expected;}
 need(refused,why);
}
struct CopyState {bool active=false;unsigned copies=0;std::function<void()> effect;};
struct SourceCopy {
 std::shared_ptr<CopyState> state;
 explicit SourceCopy(std::shared_ptr<CopyState> s):state(std::move(s)){}
 SourceCopy(const SourceCopy& other):state(other.state){
  if(state->active){++state->copies;const auto effect=state->effect;if(effect)effect();}
 }
 std::optional<std::int64_t> operator()()const{return std::nullopt;}
};
ui::UiContextRegistry registry(){
 return ui::UiContextRegistry::from_documents(content::read_xml_metadata(R"(<Context>
 <LuaContext ID="WorldView" FileName="WorldView"/>
 <LuaContext ID="CycleModal" FileName="UI/InGame/Popups/CycleModal" Hidden="True"/>
 <LuaContext ID="CityList" FileName="UI/InGame/WorldView/CityList" Hidden="True"/>
 </Context>)"),content::read_xml_metadata("<Context/>"));
}
void key_intent_checks(){
 ui::BnwManualCycleKeyInput keys;using Filter=runtime::BnwUnitCycleFilter;
 need(!ui::BnwManualCycleKeyInput::camera_up(true,true,false),"Selected W also panned camera");
 need(ui::BnwManualCycleKeyInput::camera_up(false,true,false),"Legacy camera W binding changed");
 need(ui::BnwManualCycleKeyInput::camera_up(true,true,true),"Selected arrow-up camera binding lost");
 need(keys.key(0x57,true,false,true,false,false,false)==Filter::all,"W fresh intent absent");
 need(!keys.key(0x57,true,false,true,false,false,false),"Duplicate down repeated manual intent");
 need(!keys.key(0x57,false,false,true,false,false,false),"Key up produced manual intent");
 need(!keys.key(0x57,true,true,true,false,false,false),"Native repeat bit produced intent");keys.cancel();
 need(!keys.key(0x57,true,false,false,false,false,false),"Modal/focused UI produced manual intent");
 need(!keys.key(0x57,true,false,true,false,false,false),"Held focused key replayed after modal close");keys.cancel();
 for(unsigned modifier=0;modifier<3;++modifier){
  need(!keys.key(0xbf,true,false,true,modifier==0,modifier==1,modifier==2),"Modified slash produced intent");keys.cancel();
 }
 need(keys.key(0xbf,true,false,true,false,false,false)==Filter::workers,"Fresh slash worker intent absent");
 keys.cancel();
 for(const auto code:{0xbeu,0xbcu,0x0du,0x65u})
  need(!keys.key(code,true,false,true,false,false,false),"Conflicting/unproved control was routed as cycle");
}
struct Window {
 HWND hwnd=CreateWindowExW(0,L"STATIC",L"Selected scene owned-parent CPU gate",WS_OVERLAPPEDWINDOW,
     0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 ~Window(){if(hwnd)DestroyWindow(hwnd);}
};
}
int main(int argc,char**argv){try{
 key_intent_checks();
 need(argc==4,"Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS");
 const std::filesystem::path save=argv[1],game=argv[2],canonical=argv[3];
 Window window;need(window.hwnd!=nullptr,"Actual CPU test window absent");
 auto lease=ui::BnwNativeEarlyInputLease::acquire(save,game,canonical,
     reinterpret_cast<std::uintptr_t>(window.hwnd),runtime::bnw_stock_lua_win32_client_extent(
         reinterpret_cast<std::uintptr_t>(window.hwnd),"selected-scene-cpu-window",[]{return std::uint64_t{1};}));
 auto package=ui::BnwNativePackageDiscoveryOwner::create(lease);
 package->register_native_dx9_primary_prefix();
 auto settings=ui::BnwNativeResourceBootSettingsOwner::create(package);
 package->register_native_dx9_texture_quality_branch(std::move(settings));
 package->register_native_common_high_quality_prefix();
 auto boot=ui::bootstrap_selected_bnw_preview_from_early_package(package,0);
 need(static_cast<bool>(boot.preview),"Actual genuine selected host unavailable");
 auto preview=std::make_unique<ui::SelectedBnwReadOnlyPreview>(std::move(boot.preview));
 const auto decoded=saves::read_saved_session(save,saves::SavedSessionDecodeContext{});
 need(decoded.snapshot.pregame&&decoded.snapshot.map_plots&&decoded.snapshot.map_projection,
     "Actual saved map source unavailable");
 const auto art=saves::observe_saved_map_team_art(*decoded.snapshot.pregame,*decoded.snapshot.map_plots);
 auto receipt=graphics::capture_bnw_renderer_scene_receipt(save,runtime::identify_file(save).sha256,decoded,art);
 need(receipt.has_value()&&!receipt->projection_reconciled,"Actual original source-scene receipt absent");
 auto projection=*decoded.snapshot.map_projection;
 std::weak_ptr<const ui::SelectedBnwPreviewHost> weak_parent;
 std::unique_ptr<ui::BnwClientSceneFrame> retained_scene;
 std::optional<ui::LiveSessionUiSources> retained_sources;
 ui::ClientViewport viewport{};
 ui::UiScreenController screens(registry());
 bool reached=false;
 preview->with_owned_host_camera_input([&](std::shared_ptr<const ui::SelectedBnwPreviewHost> parent,
                                         const ui::LiveBnwPreviewInput*input){
  reached=true;need(parent&&input&&&parent->session()==&input->session,"Actual owned host/session substituted");
  weak_parent=parent;parent->require_same_early_package(*package);
  need(graphics::reconcile_bnw_renderer_scene_projection(projection,*receipt,input->session.state()),
      "Actual 960-cell/art/city scene reconciliation failed");
  need(receipt->restored_payload_sha256==parent->identity().payload_sha256&&
       runtime::identify_bytes(parent->session().source().source.bytes).sha256==receipt->restored_payload_sha256,
       "Actual raw restored payload differs from selected host; cannot exchange tags");
  const auto extent=lease->client_extent();viewport={static_cast<int>(extent.width),static_cast<int>(extent.height),
      static_cast<int>(extent.width),static_cast<int>(extent.height),96};
  auto actual=std::make_unique<ui::BnwClientSceneFrame>(parent,*receipt,input->frame_sources);
  auto actual_ui=actual->create_selected_ui_host(screens,actual->ui_provider(),nullptr);
  need(&actual_ui->screens()==&screens,"Scene substituted renderer shared screen controller");
  refused_exact([&]{(void)actual->create_selected_ui_host(screens,actual->ui_provider(),nullptr);},
      "Actual selected client UI can bind only once","Scene bound a second actual receiver");
  const auto frame=actual->begin(viewport,{});
  need(frame&&actual->ready_to_draw()&&actual->current()&&
       &actual->owner().current_frame()->ui==&frame->ui&&
       frame->stamp.identity.session==parent->identity().payload_sha256&&
       frame->stamp.identity.player==parent->active_player_id(),
       "Actual selected scene did not publish ONE current payload-tagged frame");
  ui::BnwClientSceneFrame legacy(input->session,*receipt,input->active_player_id,input->frame_sources);
  const auto old_frame=legacy.begin(viewport,{});
  need(old_frame&&legacy.current()&&old_frame->stamp.identity.session==receipt->scene_file_sha256,
       "Legacy scene/file-SHA constructor changed");
  need(frame->stamp.identity.session!=old_frame->stamp.identity.session,
       "Actual fixture does not distinguish file and restored payload identities");
  // Input through the actual scene and SAME controller must survive begin().
  actual->select_plot(0,frame);
  auto manual_frame=actual->begin(viewport,{});
  auto ui_frame=actual_ui->begin_frame(viewport.client_width,viewport.client_height);
  const ui::UiCommand command{ui::UiGameplayCommand::make("WorldView",ui::BnwManualCycleSelectionCommand{})};
  const auto moves=runtime::bind_bnw_update_moves_original_import(parent->session().source(),parent->identity());
  need(parent->activation_owner().busy_state.has_value(),"Actual import busy owner absent");
  const auto expected=runtime::observe_bnw_game_unit_cycle(parent->session().state(),moves,
      *parent->activation_owner().busy_state,parent->identity(),parent->active_player_id(),std::nullopt,
      runtime::BnwUnitCycleDirection::forward,runtime::BnwUnitCycleFilter::all);
  const auto dispatched=actual->dispatch(*actual_ui,ui_frame,command);
  need(dispatched.status==ui::UiCommandStatus::navigation,"Actual scene typed manual dispatch refused");
  manual_frame=actual->begin(viewport,{});
  const auto selected=expected.next?std::optional{expected.next->second}:std::nullopt;
  need(manual_frame->stamp.selection.unit==selected&&manual_frame->stamp.selection.plot==std::optional<std::int64_t>{0},
      "Scene begin reset canonical manual unit/plot selection");
  need(actual->dispatch(*actual_ui,ui_frame,command).status==ui::UiCommandStatus::invalid,
      "Scene reused retired input frame");
  const auto repeated=actual->begin(viewport,{});
  need(repeated->stamp.selection==manual_frame->stamp.selection,
      "Repeated scene begin reset actual client selection");
  ui_frame=actual_ui->begin_frame(viewport.client_width,viewport.client_height);
  need(actual->dispatch(*actual_ui,ui_frame,ui::UiOpenScreen{"CycleModal"}).status==ui::UiCommandStatus::navigation,
      "Scene shared modal did not open");
  manual_frame=actual->begin(viewport,{});ui_frame=actual_ui->begin_frame(viewport.client_width,viewport.client_height);
  need(actual->dispatch(*actual_ui,ui_frame,command).status==ui::UiCommandStatus::invalid&&actual->current(),
      "Scene typed manual command bypassed SAME modal ownership");
  need(actual->dispatch(*actual_ui,ui_frame,ui::UiCloseScreen{"CycleModal"}).status==ui::UiCommandStatus::navigation,
      "Scene shared modal did not close");
  (void)actual->begin(viewport,{});ui_frame=actual_ui->begin_frame(viewport.client_width,viewport.client_height);
  need(ui::BnwManualCycleKeyInput::world_frame(*actual,*actual_ui,ui_frame),
      "Actual un-focused displayed frame cannot receive manual keys");
  need(actual->dispatch(*actual_ui,ui_frame,ui::UiOpenScreen{"CityList"}).status==ui::UiCommandStatus::navigation,
      "Actual nonmodal CityList did not open");
  (void)actual->begin(viewport,{});ui_frame=actual_ui->begin_frame(viewport.client_width,viewport.client_height);
  need(actual->accepts(*actual_ui,ui_frame)&&actual_ui->screens().focused_screen()==std::optional<std::string>{"CityList"}&&
      !actual_ui->screens().registry().find("CityList")->modal,
      "Actual SAME controller nonmodal focus fixture absent");
  ui::BnwManualCycleKeyInput focused_keys;
  need(!ui::BnwManualCycleKeyInput::world_frame(*actual,*actual_ui,ui_frame)&&
      !focused_keys.key(0x57,true,false,ui::BnwManualCycleKeyInput::world_frame(*actual,*actual_ui,ui_frame),false,false,false),
      "Actual focused NONmodal receiver allowed manual key intent");
  need(actual->dispatch(*actual_ui,ui_frame,ui::UiCloseScreen{"CityList"}).status==ui::UiCommandStatus::navigation,
      "Actual focused nonmodal CityList did not close");
  (void)actual->begin(viewport,{});ui_frame=actual_ui->begin_frame(viewport.client_width,viewport.client_height);
  need(ui::BnwManualCycleKeyInput::world_frame(*actual,*actual_ui,ui_frame)&&
      !focused_keys.key(0x57,true,false,true,false,false,false),
      "Held focused key replayed when actual nonmodal receiver closed");
  if(!expected.next)std::cout<<"Scene fixture changed-unit branch UNOBSERVED; nullable selection retained\n";
  auto state=std::make_shared<CopyState>();ui::LiveSessionUiSources probed=input->frame_sources;
  probed.selected_unit=SourceCopy(state);
  state->active=true;
  refused_exact([&]{ui::BnwClientSceneFrame bad({},*receipt,probed);},
      "Selected renderer parent absent","Missing parent entered source-copy/owner construction");
  need(state->copies==0,"Missing parent copied callable targets");
  auto wrong=*receipt;wrong.restored_payload_sha256="different-payload-negative";
  refused_exact([&]{ui::BnwClientSceneFrame bad(parent,wrong,probed);},
      "Selected renderer payload differs from actual host","Wrong payload reached source-copy/owner construction");
  wrong=*receipt;wrong.active_player=static_cast<std::uint64_t>(parent->active_player_id()+1);
  refused_exact([&]{ui::BnwClientSceneFrame bad(parent,wrong,probed);},
      "Selected renderer player differs from actual host","Wrong selected player reached source-copy/owner construction");
  wrong=*receipt;wrong.projection_reconciled=false;
  refused_exact([&]{ui::BnwClientSceneFrame bad(parent,wrong,probed);},
      "Selected renderer source projection not reconciled","Unreconciled source reached callback/owner");
  need(state->copies==0,"Rejected source/player/projection copied callable targets");
  state->effect=[] {throw std::runtime_error("source-copy-observer-throw");};
  refused_exact([&]{ui::BnwClientSceneFrame bad(parent,*receipt,probed);},
      "source-copy-observer-throw","Uncertain callable copy produced a scene owner");
  state->effect={};state->active=false;
  need(actual->current(),"Throwing diagnostic source copy changed actual scene publication");
  // The original receipt storage can legally be released by a callable copy.
  // A genuine parent and plain receipt must already be pinned before that copy.
  // There is one canonical client per Session. Retire the first before
  // constructing the next source-copy/lifetime probe.
  const auto retired_provider=actual->ui_provider();actual.reset();actual_ui.reset();
  refused_exact([&]{(void)retired_provider();},"BNW client UI owner retired",
      "Scene destruction left its provider usable");
  auto aliased_receipt=std::make_optional(*receipt);
  state->effect=[&]{aliased_receipt.reset();state->active=false;};state->active=true;
  auto pinned_receipt=std::make_unique<ui::BnwClientSceneFrame>(parent,*aliased_receipt,probed);
  auto pinned_ui=pinned_receipt->create_selected_ui_host(screens,pinned_receipt->ui_provider(),nullptr);
  need(!aliased_receipt&&pinned_receipt->begin(viewport,{})&&pinned_receipt->current(),
      "Source callback invalidated borrowed receipt before ownership pin");
  state->effect={};
  // A real identical checkpoint restore during callable copy invalidates the
  // captured native state generation BEFORE FrameOwner construction.
  const auto checkpoint=input->session.checkpoint();
  state->effect=[&]{state->active=false;input->session.restore(checkpoint);};state->active=true;
  refused_exact([&]{ui::BnwClientSceneFrame bad(parent,*receipt,probed);},
      "Selected renderer state replaced during source copy","Identical restore/ABA escaped source-copy fence");
  state->effect={};
  need(!pinned_receipt->current(),"Identical restore retained an old live scene");
  pinned_receipt.reset();pinned_ui.reset();
  // After explicit restore, old constructor-owned read bridges remain stale.
  // This retention-only scene therefore excludes those old sources: genuine
  // scene, source map/visibility and active player are still the real state.
  // No title/portrait/native Label or fabricated publication is supplied.
  retained_sources.emplace();
  retained_scene=std::make_unique<ui::BnwClientSceneFrame>(parent,*receipt,*retained_sources);
  auto retained_ui=retained_scene->create_selected_ui_host(screens,retained_scene->ui_provider(),nullptr);
  need(&retained_ui->screens()==&screens,"Rebuilt scene substituted shared controller");
  need(retained_scene->begin(viewport,{})&&retained_scene->current(),
      "Explicit scene owner rebuild failed on identical source state");
 });
 need(reached&&retained_scene&&!weak_parent.expired(),"Actual owned parent handoff skipped");
 preview.reset();
 need(!weak_parent.expired()&&retained_scene->current()&&
      retained_scene->frame()->stamp.identity.session==receipt->restored_payload_sha256,
      "Actual scene did not retain parent across final preview facade release");
 retained_scene.reset();retained_sources.reset();
 need(weak_parent.expired(),"Scene retained-parent ownership cycle or alias remains");
 std::cout<<"PASS actual CPU selected-host source/player/payload, source-copy/restore/lifetime and legacy identity gates; no GPU/native Label claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
